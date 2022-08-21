#ifndef MYTCPSOCKET_HPP
#define MYTCPSOCKET_HPP

#include <QTcpSocket>
#include <QtEndian>
#include <QDebug>
#include <QThread>

#include "dataset.hpp"

class MyTcpSocket : public QTcpSocket{
    Q_OBJECT;
    typedef QTcpSocket super;
    super *socket;
public:
    MyTcpSocket(super* _socket = nullptr) : super() {
        if(_socket == nullptr) socket = this;

        buffer = new char[BufferSize];
        cpyBuffer = new char[BufferSize];

        connect(this, &MyTcpSocket::timeout, this, [](){qDebug() << "MyTcpSocket : timeout";});
        connect(this, &MyTcpSocket::writeFail, this, [](){qDebug() << "MyTcpSocket : writeFail";});
        connect(socket, &QTcpSocket::readyRead, this, &MyTcpSocket::processReadyRead);
        connect(this, &MyTcpSocket::pullData, this, &MyTcpSocket::processPullData);
    }
    ~MyTcpSocket(){
        delete[] buffer;
        delete[] cpyBuffer;
    }
signals:
    void incomingData(DataSet dataSet); // Player timeout should be included in this case, not the one below, and other side should send a playerTimeout message.
    void timeout(); // TODO : process these
    void writeFail(); // TODO : process these
    void ready(); // initialization complete
    void readyPull(); // a DataSet is ready to be pulled
    void pullData(); // tells me to pull and i give you a incoming Data
public slots:
    void setSocket(super *_socket) {socket = _socket; emit ready(); connect(socket, &QTcpSocket::readyRead, this, &MyTcpSocket::processReadyRead);}
    void processReadyRead(){
        qDebug() << "process readyRead signal";
        int bufferLeft = BufferSize - bufferCnt;
        int bytesRead = socket->read(buffer+bufferCnt, bufferLeft);
        qToBigEndian<char>(buffer+bufferCnt, bytesRead, buffer+bufferCnt);

        //debug
        for(int i = bufferCnt; i < bufferCnt+bytesRead; i++)
            qDebug() << "received a " << int(buffer[i]) << " on thread " << QThread::currentThreadId();

        bufferCnt += bytesRead;

        if(bufferCnt > 0 && bufferCnt > int(buffer[0]))
        {
            qDebug() << "emit readyPull()";
            emit readyPull();
        }
    }
    void processPullData(){
        if(bufferCnt <= 0 || bufferCnt <= int(buffer[0]))
            return;
        qDebug() << "pulling data on thread " << QThread::currentThreadId();
        int readCnt = int(buffer[0]);
        bufferCnt -= readCnt+1;
        DataSet ret(buffer+1, readCnt);
        charncpy(cpyBuffer, buffer+readCnt+1, bufferCnt);
        charncpy(buffer, cpyBuffer, bufferCnt);

        //debug
        qDebug() << "bufferCnt = " << bufferCnt;
        for(int i = 0; i < bufferCnt; i++)
            qDebug() << "buffer[" << i << "] = " << int(buffer[i]);

        emit incomingData(ret);
        if(bufferCnt > 0 && bufferCnt > int(buffer[0]))
        {
            qDebug() << "emit readyPull()";
            emit readyPull();
        }
    }
    void pushData(DataSet dataSet){
        qDebug() << "pushing data: " << int(dataSet.getLenData());
        qDebug() << "The data is :";
        for(int i = 0; i < int(dataSet.getLenData()); i++)
            qDebug() << int(dataSet.getData()[i]);
        char len = dataSet.getLenData();
        qToBigEndian<char>(&len, 1, &len);
        static const int Patience = 5;
        int noWrittenPatience = Patience;
        int written;

        written = socket->write(&len, 1);
        while(!written)
        {
            if(noWrittenPatience <= 0)
            {
                emit writeFail();
                return;
            }
            noWrittenPatience--;
            written = socket->write(&len, 1);
        }
        qFromBigEndian<char>(&len, 1, &len);

        int pos = 0;
        char* writeBuffer = new char[len+2];
        qToBigEndian<char>(dataSet.getData(), len, writeBuffer);
        while(pos < len)
        {
            written = socket->write(writeBuffer+pos, len-pos);
            qDebug() << written << " bytes written:";
            for(int i = pos; i < pos+written; i++)
                qDebug() << int(writeBuffer[i]);
            pos += written;
            if(written <= 0)
            {
                noWrittenPatience--;
                if(noWrittenPatience <= 0)
                {
                    emit writeFail();
                    return;
                }
            }
            else
                noWrittenPatience = Patience;
        }
        delete[] writeBuffer;
        qDebug() << "push data ends.";
    }
private:
    static const int TIMEOUTsec = 40; // seconds
    static const int BufferSize = 128;
    char* buffer;
    char* cpyBuffer;
    int bufferCnt = 0;
};

#endif // MYTCPSOCKET_HPP
