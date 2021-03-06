#include "clienthandler.h"

#include <thread>
#include "utilities.h"
#include <memory>
#include "debugwindow.h"
#include "clientmanager.h"
#include "songmanager.h"
#include "songqueue.h"
#include "audiomanager.h"

ClientHandler::ClientHandler(SOCKET sock, u_long ip) {
    info.userId = sock;
    info.ip = ip;
    olapWrap.client = this;
    std::thread clientThread(&ClientHandler::init, this);
    clientThread.detach();
}

void ClientHandler::init() {
    wsaBuf.buf = buffer;
    wsaBuf.len = sizeof(buffer);
    receive(info.userId, wsaBuf, &olapWrap.olap, receiveRoutine);
    while (true) {
        SleepEx(INFINITE, TRUE);
    }
}

void CALLBACK ClientHandler::receiveRoutine(DWORD errCode, DWORD recvBytes, LPOVERLAPPED olap, DWORD flags) {
    if (errCode != 0) {
        DebugWindow::get()->logd("Client::receiveRoutine failed.");
    }

    ClientHandler *client = reinterpret_cast<ClientHandlerOlap *>(olap)->client;
    client->handleReceive(recvBytes);
}

void ClientHandler::handleReceive(int recvBytes) {
    if (recvBytes == 0) {
        DebugWindow::get()->logd(QString("Client disconnected: ") + QString(info.username) + itoq(info.userId));
        ClientManager::get().removeClient(info.userId);
    } else {
        parse(recvBytes);
    }
}

void ClientHandler::parse(int recvBytes) {
    char *tempBuffer = buffer;
    int offset = 0;
    while (recvBytes) {
        tempBuffer += offset;
        offset = 0;
        PktIds *pktId = reinterpret_cast<PktIds *>(tempBuffer);
        switch (*pktId) {
        case PktIds::JOIN:
            {
                Join *join = reinterpret_cast<Join *>(tempBuffer);
                DebugWindow::get()->logd(QString("THIS PERSON JOINED: ") + join->username);

                UserId idPkt;
                idPkt.id = info.userId;
                WSABUF wsaBuf;
                wsaBuf.buf = reinterpret_cast<char *>(&idPkt);
                wsaBuf.len = sizeof(idPkt);
                sendTCP(info.userId, wsaBuf);

                memcpy(info.username, join->username, strlen(join->username) + 1);
                ClientManager::get().addClient(this);

                SongManager::get().sendSongList();
                SongQueue::get().sendSongQueue();
            }
            offset += sizeof(Join);
            break;
        case PktIds::SONG_REQUEST:
            {
                SongRequest *request = reinterpret_cast<SongRequest *>(tempBuffer);
                SongQueue::get().addSong(SongManager::get().at(request->songId));
            }
            offset += sizeof(SongRequest);
            break;
        case PktIds::DOWNLOAD_REQUEST:
            {
                DownloadRequest *request = reinterpret_cast<DownloadRequest *>(tempBuffer);
                QByteArray data = audioManager::get().loadSong(SongManager::get().at(request->songId).getDir());
                char temp[sizeof(PktIds::DOWNLOAD) + sizeof(int) + sizeof(int)];
                PktIds pktId = PktIds::DOWNLOAD;
                int len = data.size();
                memcpy(temp, &pktId, sizeof(PktIds::DOWNLOAD));
                memcpy(temp + sizeof(PktIds::DOWNLOAD), &request->songId, sizeof(int));
                memcpy(temp + sizeof(PktIds::DOWNLOAD) + sizeof(int), &len, sizeof(int));
                WSABUF wsaBuf;
                wsaBuf.buf = temp;
                wsaBuf.len = sizeof(temp);

                sendTCP(info.userId, wsaBuf);



                DebugWindow::get()->logd(QString("len: ") + itoq(len));

                wsaBuf.buf = data.data();
                wsaBuf.len = data.size();
                sendTCP(info.userId, wsaBuf);
            }
            offset += sizeof(DownloadRequest);
            break;
        case PktIds::UPLOAD:
            {
                Upload *upload = reinterpret_cast<Upload *>(tempBuffer);
                QByteArray data;
                QString audioDir(QString(upload->title) + ".wav");
                Song song(SongManager::get().genId(), upload->title, upload->artist, upload->album, audioDir);
                int len = upload->len;
                while (len) {
                    wsaBuf.buf = buffer;
                    if (len > sizeof(buffer))
                        wsaBuf.len = sizeof(buffer);
                    else
                        wsaBuf.len = len;
                    int ret = recvTCP(info.userId, wsaBuf);
                    data.append(wsaBuf.buf, ret);
                    len -= ret;
                }
                QFile file(audioDir);
                if (file.open(QIODevice::ReadWrite)) {
                    file.write(data);
                }
                file.close();
                recvBytes = 0;
                SongManager::get().addSong(song);

            }
            break;
        }
        recvBytes -= offset;
    }

    wsaBuf.buf = buffer;
    wsaBuf.len = sizeof(buffer);
    receive(info.userId, wsaBuf, &olapWrap.olap, receiveRoutine);
}


