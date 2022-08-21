#ifndef HOSTWINDOW_HPP
#define HOSTWINDOW_HPP

#include <QPushButton>
#include <QLabel>
#include <QDebug>

#include "serverthread.h"
#include "utils.hpp"

class HostWindow : public QWidget
{
    Q_OBJECT
    typedef QWidget super;
public:
    HostWindow(ServerThread* _serverThread, int& _status, bool& _isHost, QWidget *parent = nullptr, Qt::WindowFlags f = Qt::Dialog);
private:
    QLabel* labelInfo;
    QPushButton* btnOK;
    QPushButton* btnCancel;

    int& statusMainWindow;
    bool& isHost;

    ServerThread* serverThread;
    bool threadInitialized = false;

signals:
    void createHostWindow();
    void startListen();
    void stopListen();
    void connected();
private slots:
    void processInit();
    void processConnect();
    void processButtonOk();
    void processButtonCancel();
};

inline HostWindow::HostWindow(ServerThread* _serverThread, int& _status, bool& _isHost, QWidget *parent, Qt::WindowFlags f)
    : super(parent, f), statusMainWindow(_status), isHost(_isHost), serverThread(_serverThread)
{
    setWindowModality(Qt::WindowModal);
    setWindowTitle("Host a game");

    // init objects it contains
    labelInfo = new QLabel("Press OK to start hosting.\nHost IP: "+myIp(), this);
    labelInfo->setGeometry(10, 30, 180, 60);

    btnOK = new QPushButton("OK", this);
    btnOK->setGeometry(60, 110, 80, 30);

    btnCancel = new QPushButton("Cancel", this);
    btnCancel->setGeometry(60, 160, 80, 30);


    // start serverThread
    serverThread->start();

    // connect slots
    connect(btnOK, &QPushButton::clicked, this, &HostWindow::processButtonOk);
    connect(btnCancel, &QPushButton::clicked, this, &HostWindow::processButtonCancel);
    connect(serverThread, &ServerThread::initialized, this, &HostWindow::processInit);
    connect(serverThread, &ServerThread::connected, this, &HostWindow::processConnect);
    connect(this, &HostWindow::startListen, serverThread, &ServerThread::startListening);
    connect(this, &HostWindow::stopListen, serverThread, &ServerThread::stopListening);
}

inline void HostWindow::processButtonOk()
{
    qDebug() << "HostWindow::processButton()";

    if(!threadInitialized || statusMainWindow != -1) // if the thread isn't initialized or connection is already established
        return;
    qDebug() << "HostWindow emit startListen";
    emit startListen();
    labelInfo->setText("Listening :\n    Waiting for connection...");
    hide();
}

inline void HostWindow::processButtonCancel()
{
    qDebug() << "HostWindow::processButtonCancel";
    if(statusMainWindow == -1)
    {
        labelInfo->setText("Press OK to start hosting.\nHost IP: "+myIp());
        emit stopListen();
    }
    hide();
}

inline void HostWindow::processInit(){
    threadInitialized = true;
}

inline void HostWindow::processConnect(){
    qDebug() << "HostWindow::processConnect()";
    labelInfo->setText("Connected!");
    this->hide();
    isHost = true;
    emit connected();
}

#endif // HOSTWINDOW_HPP
