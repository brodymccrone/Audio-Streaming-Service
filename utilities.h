#ifndef UTILITIES_H
#define UTILITIES_H

#include <winsock2.h>
#include "routinestructs.h"
#include <QString>
#include <QWidget>

enum ErrorType {
    CREATE_SOCKET,
    RECEIVE,
    BIND_SOCKET,
    SEND,
    JOIN_MULTICAST,
    TRAVERSE_MULTIPLE,
    DISABLE_LOOPBACK,
    SET_REUSABLE,
    SEND_TCP_OLAP,
    SEND_TCP,
    ACCEPT_CONNECTION,
    CONNECT_SOCKET,
    SEND_UDP,
    RECV_TCP
};

typedef void (*Routine)(DWORD, DWORD, LPWSAOVERLAPPED, DWORD);

void printError(const char *msg, int errCode);
void startup();
SOCKET createSocket(int type);
SOCKET acceptConnection(SOCKET sock);
void sendUDP(SOCKET sock, WSABUF& wsaBuf, struct sockaddr_in addr);
bool connectSocket(SOCKET sock, struct sockaddr_in addr);
void sendTCP(SOCKET sock, WSABUF& wsaBuf);
int recvTCP(SOCKET sock, WSABUF& wsaBuf);
void bindSocket(SOCKET sock, struct sockaddr_in addr);
std::pair<SOCKET, struct sockaddr_in> acceptConnectionGetAddr(SOCKET sock);
struct sockaddr_in createAddress(u_long ip, int port);
void receive(SOCKET sock, WSABUF wsaBuf, LPWSAOVERLAPPED olap, Routine callback);
void multicast(SOCKET sock, LPWSABUF wsaBuf, struct sockaddr_in& addr, LPWSAOVERLAPPED overlapped, Routine callback);
void joinMulticast(SOCKET sock);
void traverseMultiple(SOCKET sock);
void disableLoopback(SOCKET sock);
QString itoq(int);
QString addNull(const char *txt, int len);
void handleError(int errCode, const char *msg, ErrorType err);
void showWarningMessage(QWidget *parent, QString title, QString msg);

#endif // UTILITIES_H
