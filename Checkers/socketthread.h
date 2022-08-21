#ifndef SOCKETTHREAD_H
#define SOCKETTHREAD_H

#include <QThread>
#include <QTcpSocket>

#include "mytcpsocket.hpp"


class SocketThread: public QThread{
    Q_OBJECT
    typedef QThread super;
public:
    SocketThread(QObject *parent = nullptr, int _port = 57856);

protected:
    void run() override;

private:
    MyTcpSocket* _tcpSocket = nullptr;
    int port;
    unsigned int TIMEOUTsec = 60;

signals:
    // init
    void initialized();
    void initConnectResult(int code);
    /* Result code:
     * 0 : success
     * 1 : ip addr not valid
     * 2 : init connect timeout
     */

    // gaming
    void incomingData(DataSet dataSet); // [read] [up] with mainWindow
    void readyPull(); // [read] [up] with mainWindow : indicate that a pull can be done
    void pullData(); // [read] [down] with tcp socket :
    void pushData(DataSet dataSet); // [write] [down] with tcp socket

    // ends
    void disconnected();
    void socketDestroyed();

public slots:
    //init
    void processStateChanged(MyTcpSocket::SocketState);
    void initConnect(QString addr);

    //gaming
    void sendData(DataSet dataSet); // [write] call _tcpSocket->pushData()
    void getData(); // [read]

    //ending
    void processDisconnect();

};

#endif // SOCKETTHREAD_H
