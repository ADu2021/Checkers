#ifndef SERVERTHREAD_H
#define SERVERTHREAD_H

#include <QThread>
#include <QTcpServer>

#include "mytcpsocket.hpp"


class ServerThread: public QThread{
    Q_OBJECT
    typedef QThread super;
public:
    ServerThread(QObject *parent = nullptr, int _port = 57856);

protected:
    void run() override;

private:
    QTcpServer* server = nullptr; // delete ?
    MyTcpSocket* _tcpSocket = nullptr; // delete ? // (TODO): and same thing in socketthread.h
    int port;

signals:
    // init
    void initialized(); // for HostWindow TODO
    void connected(); // for HostWindow TODO
    void sendSocket(QTcpSocket* _socket); // for _tcpSocket

    // gaming
    void incomingData(DataSet dataSet); // [read] [up] with mainWindow
    void readyPull(); // [read] [up] with mainWindow : indicate that a pull can be done
    void pullData(); // [read] [down] with tcp socket :
    void pushData(DataSet dataSet); // [write] [down] with tcp socket

    // ends
    void disconnected();
    void socketDestroyed();


public slots:
    // init
    void startListening();
    void stopListening();
    void processNewConnection();
    void processSocketReady();

    //gaming
    void getData(); // [read] [down] with tcp socket
    void sendData(DataSet); // [write] [up] with mainWindow

    //ending
    void processDisconnect();

};

#endif // SERVERTHREAD_H
