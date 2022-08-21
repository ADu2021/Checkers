#include <QDebug>
#include "socketthread.h"
#include "utils.hpp"

SocketThread::SocketThread(QObject *parent, int _port) : super(parent), port(_port) {
    moveToThread(this); // someone says this is ugly
}

void SocketThread::run(){

    qDebug() << "SocketThread::run()..";
    _tcpSocket = new MyTcpSocket();

    connect(_tcpSocket, &MyTcpSocket::stateChanged, this, &SocketThread::processStateChanged);

    emit initialized();
    exec();
}

void SocketThread::processStateChanged(MyTcpSocket::SocketState state) {
    switch(state){
    case MyTcpSocket::UnconnectedState:
        qDebug() << "TcpSocket UnconnectedState";
        emit initConnectResult(2); // timeout
        break;
    case MyTcpSocket::ConnectedState:
        qDebug() << "TcpSocket ConnectedState";
        connect(this, &SocketThread::pullData, _tcpSocket, &MyTcpSocket::pullData); // [down] read
        connect(_tcpSocket, &MyTcpSocket::incomingData, this, &SocketThread::incomingData); // [up] read
        connect(_tcpSocket, &MyTcpSocket::readyPull, this, &SocketThread::readyPull); // [up] read
        connect(this, &SocketThread::pushData, _tcpSocket, &MyTcpSocket::pushData); // [down] write
        connect(_tcpSocket, &MyTcpSocket::disconnected, this, &SocketThread::disconnected);
        connect(_tcpSocket, &MyTcpSocket::destroyed, this, &SocketThread::socketDestroyed);

        emit initConnectResult(0); // success
        break;
    default:
        break;
    }
}

void SocketThread::initConnect(QString addr) {

    if(!isIp(addr))
    {
        emit initConnectResult(1);
        return;
    }

    qDebug() << "SocketThread::initConnect(QString addr) " << addr;

    qDebug() << "Thread in SocketThread::initConnect : " << currentThreadId();

    _tcpSocket->connectToHost(addr, port);

    qDebug() << "after connect initiated";
}

void SocketThread::sendData(DataSet dataSet){
    _tcpSocket->pushData(dataSet);
}

void SocketThread::getData(){
    emit pullData();
}
void SocketThread::processDisconnect() {
    if(_tcpSocket == nullptr)
        return;
    _tcpSocket->close();
    _tcpSocket->deleteLater();
    _tcpSocket = nullptr;
}

