#include "serverthread.h"

ServerThread::ServerThread(QObject *parent, int _port) : super(parent), port(_port) {
    moveToThread(this);
}

void ServerThread::run(){
    qDebug() << "ServerThread::run()..";
    _tcpSocket = new MyTcpSocket();
    server = new QTcpServer();

    //connect signals and slots:
    connect(server, &QTcpServer::newConnection, this, &ServerThread::processNewConnection);
    connect(this, &ServerThread::sendSocket, _tcpSocket, &MyTcpSocket::setSocket);
    connect(_tcpSocket, &MyTcpSocket::ready, this, &ServerThread::processSocketReady);

    emit initialized();
    exec();
}

void ServerThread::startListening(){
    if(!server->isListening())
        server->listen(QHostAddress::Any, port);
}

void ServerThread::stopListening(){
    if(server->isListening())
        server->close();
}

void ServerThread::processNewConnection(){
        emit sendSocket(server->nextPendingConnection());
}

void ServerThread::processSocketReady(){
    connect(this, &ServerThread::pullData, _tcpSocket, &MyTcpSocket::pullData); // [down] read
    connect(_tcpSocket, &MyTcpSocket::incomingData, this, &ServerThread::incomingData); // [up] read
    connect(_tcpSocket, &MyTcpSocket::readyPull, this, &ServerThread::readyPull); // [up] read
    connect(this, &ServerThread::pushData, _tcpSocket, &MyTcpSocket::pushData); // [down] write
    connect(_tcpSocket, &MyTcpSocket::disconnected, this, &ServerThread::disconnected);
    connect(_tcpSocket, &MyTcpSocket::destroyed, this, &ServerThread::socketDestroyed);
    emit connected();
}

void ServerThread::getData(){
    emit pullData();
}
void ServerThread::sendData(DataSet dataSet){
    emit pushData(dataSet);
}
void ServerThread::processDisconnect() {
    if(_tcpSocket != nullptr){
        _tcpSocket->close();
        _tcpSocket->deleteLater();
        _tcpSocket = nullptr;
    }
    if(server != nullptr){
        server->close();
        server->deleteLater();
        server = nullptr;
    }

}
