#ifndef JOINWINDOW_HPP
#define JOINWINDOW_HPP

#include <QWidget>
#include <QPlainTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QDebug>
#include <QMainWindow>
#include <QThread>

#include "socketthread.h"


class JoinWindow : public QWidget
{
    Q_OBJECT
    typedef QWidget super;
public:
    JoinWindow(SocketThread* _socketThread, int& _status, QWidget *parent = nullptr, Qt::WindowFlags f = Qt::Dialog);
private:
    QLabel *labelHostIp;
    QPlainTextEdit *inputText;
    QPushButton *buttonConnect;
    QPushButton *buttonCancel;
    QLabel *labelWarning;

    SocketThread *socketThread;
    int& statusMainWindow;

    bool processing = false;

private slots:
    void processButtonConnect();
    void processButtonCancel();
    void processInitialized();
    void processConnectResult(int result);
signals:
    void initConnect(QString addr);
    void startConnecting();
    void stopConnecting(int code = 0);
    void connected();
};

inline JoinWindow::JoinWindow(SocketThread* _socketThread, int& _status, QWidget *parent, Qt::WindowFlags f)
    : super(parent, f), statusMainWindow(_status), socketThread(_socketThread) // there was a stupid segmentation fault due to not initiating the pointer socketThread...
{
    setWindowModality(Qt::WindowModal);
    setWindowTitle("Join a game");

    // init objects it contains
    labelHostIp = new QLabel("Host IP:", this);
    labelHostIp->setGeometry(20,40,labelHostIp->geometry().width(), labelHostIp->geometry().height());

    inputText = new QPlainTextEdit("127.0.0.1", this);
    inputText->setGeometry(75, 40, inputText->geometry().width()+5, inputText->geometry().height());

    buttonConnect = new QPushButton("Connect", this);
    buttonConnect->setGeometry(50, 90, buttonConnect->geometry().width(), buttonConnect->geometry().height()); // 100,30

    buttonCancel = new QPushButton("Cancel", this);
    buttonCancel->setGeometry(50, 130, buttonConnect->geometry().width(), buttonConnect->geometry().height());

    labelWarning = new QLabel("Please enter the host IP \n this is another line.", this);
    labelWarning->setGeometry(20,170,160,50);

    // connect slots
    connect(buttonConnect, &QPushButton::clicked, this, &JoinWindow::processButtonConnect);
    connect(buttonCancel, &QPushButton::clicked, this, &JoinWindow::processButtonCancel);
    connect(this, &JoinWindow::initConnect, socketThread, &SocketThread::initConnect);
    connect(socketThread, &SocketThread::initialized, this, &JoinWindow::processInitialized);
    connect(socketThread, &SocketThread::initConnectResult, this, &JoinWindow::processConnectResult);
}

inline void JoinWindow::processButtonConnect() {
    // todo : do not process button click when tcp connection is already established
    // to avoid multiple click
    if(processing || statusMainWindow != -1)
        return;
    processing = true;
    emit startConnecting();
    labelWarning->setText("Connecting...");

    qDebug() << "JoinWindow::processButtonConnect()..";
    qDebug() << "Original thread: " << QThread::currentThreadId();

    if(!socketThread->isRunning())
        socketThread->start();
    emit initConnect(inputText->toPlainText());
}
inline void JoinWindow::processButtonCancel() {
    hide();
}
inline void JoinWindow::processInitialized() {
    qDebug() << "JoinWindow::processInitialized()..";
    emit initConnect(inputText->toPlainText());
}

inline void JoinWindow::processConnectResult(int result){
    /* Result code:
     * 0 : success
     * 1 : ip addr not valid
     * 2 : init connect timeout
     */
    switch(result){
    case 0:
        labelWarning->setText("Connected Successfully!");
        this->hide();
        emit connected();
        break;
    case 1:
        labelWarning->setText("Error : \ninvalid ip address");
        emit stopConnecting(1);
        break;
    case 2:
        labelWarning->setText("Error : \ninitiate connection timeout");
        emit stopConnecting(2);
        break;
    }
    processing = false;
}

#endif // JOINWINDOW_HPP
