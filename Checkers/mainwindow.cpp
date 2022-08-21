#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "hexcoord.hpp"
#include "utils.hpp"

#include <QDebug>
#include <QChar>
#include <QHostInfo>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , actionHost(new QAction(tr("&Host a game")))
    , actionJoin(new QAction(tr("&Join a game")))
    , actionStart(new QAction(tr("&Start")))
    , actionSurrender(new QAction(tr("&Surrender")))
{
    ui->setupUi(this);
    srand(time(0));
    setFixedSize(668,595);
    setWindowTitle("Chinese Checkers");

    // connect QMenu and QAction
    menuConnect = menuBar()->addMenu(tr("&Connect"));
    menuConnect->addAction(actionHost);
    menuConnect->addAction(actionJoin);
    menuPlay = menuBar()->addMenu(tr("&Play"));
    menuPlay->addAction(actionStart);
    menuPlay->addAction(actionSurrender);

    // initiate the board
    QList<QPoint> pos = calcPos(QPoint(400,50), 30, 35);
    QList<HexCoord> cor = calcCoord();
    //QList<Q>
    DrawLabel* dl;
    for(int i = 0, len = pos.size(); i < len; i++)
    {
        dl = new DrawLabel(this, cor[i], i);
        map.insert(cor[i], dl);
        dl->move(pos[i]);
        dl->show();
        if(cor[i].z <= -5)
            dl->setStatus(1);
        else if(cor[i].z >= 5)
            dl->setStatus(2);
        else
            dl->setStatus(0);

        qDebug() << dl->getCoord().x << ' ' << dl->getCoord().y << ' ' << dl->getCoord().z << Qt::endl;

        dls.append(dl);
    }

    // Init lcd number
    lcdTurn = new IncLCDNumber(this);
    lcdTurn->move(15,100);
    lcdTurn->display(1);
    timerLcd = new IncLCDNumber(this);
    timerLcd->move(15,50);
    timerLcd->display(0);
    timerLcd->setStep(-1);

    // init timer
    timer = new QTimer(this);
    timer->setTimerType(Qt::PreciseTimer);

    // init label
    labelInfo = new QLabel("Not connected.", this);
    labelInfo->setGeometry(QRect(30,450,300,300));
    labelInfo->setAlignment(Qt::AlignTop|Qt::AlignLeft);
    QFont font = labelInfo->font();
    font.setFamily("Times");
    font.setPointSize(15);
    font.setBold(true);
    labelInfo->setFont(font);

    labelColor = new QLabel("", this);
    labelColor->setGeometry(QRect(30, 250, 170, 45));
    labelColor->setFont(font);

    labelTimeLeft = new QLabel("Time Left", this);
    labelTimeLeft->move(125,50);
    labelTimeLeft->setFont(font);

    labelTurn = new QLabel("Turn Count", this);
    labelTurn->setGeometry(QRect(125, 100, labelTurn->geometry().width()+20, labelTurn->geometry().height()));
    labelTurn->setFont(font);

    // connect signals and slots:
    for(int i = 0, len = dls.size(); i < len; i++)
    {
        dl = dls[i];
        connect(dl, &DrawLabel::Clicked, this, &MainWindow::processDlClick);
    }
    connect(actionHost, &QAction::triggered, this, &MainWindow::processHost);
    connect(actionJoin, &QAction::triggered, this, &MainWindow::processJoin);
    connect(actionStart, &QAction::triggered, this, &MainWindow::processStart);
    QObject::connect(actionSurrender, &QAction::triggered, this, &MainWindow::processSurrender);
    connect(&serverThread, &ServerThread::incomingData, this, &MainWindow::processIncomingData);
    connect(&serverThread, &ServerThread::readyPull, this, &MainWindow::processReadyPull);
    connect(&socketThread, &SocketThread::incomingData, this, &MainWindow::processIncomingData);
    connect(&socketThread, &SocketThread::readyPull, this, &MainWindow::processReadyPull);
    connect(this, &MainWindow::pullData, &serverThread, &ServerThread::pullData);
    connect(this, &MainWindow::pullData, &socketThread, &SocketThread::pullData);
    connect(this, &MainWindow::disconnectTcp, &serverThread, &ServerThread::processDisconnect);
    connect(this, &MainWindow::disconnectTcp, &socketThread, &SocketThread::processDisconnect);
    connect(timer, &QTimer::timeout, this, &MainWindow::processTimerUpdate);
    connect(&serverThread, &ServerThread::socketDestroyed, this, &MainWindow::processSocketDestroyed);
    connect(&socketThread, &SocketThread::socketDestroyed, this, &MainWindow::processSocketDestroyed);
    connect(&socketThread, &SocketThread::disconnected, this, &MainWindow::processDisconnected);
    connect(&serverThread, &ServerThread::disconnected, this, &MainWindow::processDisconnected);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::processDlClick(DrawLabel* dl){
    /* MainWindow Status Code:
     * -2: Game ends.
     * -1: Not connected.
     * 0: Connected, but the game isn't started yet.
     * 1: Game starts, waiting for commands.
     * 2: Waiting the opponent(network communication) i.e. Opponent's Turn
     * 3: Checker selected, waiting for destination command (for the first time).
     * 4: Checker alrealy jumped, but it can jump another time.
     * */

    qDebug() << status;

    if(status < 0 || status == 0 || status == 2)
    {

        return;
    }
    else if(status == 1)
    {
        if(dl->getStatus() == 0)
            return;
        if(isFirst != (dl->getStatus()&1))
            return;

        selected = dl;

        dl->setStatus(dl->getStatus()+2);
        setStatus(3);
        return;
    }
    else if(status == 3)
    {
        if(dl->getCoord() == selected->getCoord())
        {
            dl->setStatus(dl->getStatus()-2);
            setStatus(1);
            return;
        }
        int valid = validMove(map, selected, dl, 1, 1);
        if(valid == 1)
        {
            dl->setStatus(selected->getStatus()-2);
            selected->setStatus(0);
            setStatus(2);

            emit pushData(DataSet(selected->getId(), dl->getId()));
            if(endTurnChk()){
                // Warning : if these emit's order is broken, it will cause logic confusion.
                emit pushData(DataSet(123));
            }

            return;
        }
        if(valid == 2)
        {
            if(hasNextMove(map,dl))
            {
                dl->setStatus(selected->getStatus());
                selected->setStatus(0);
                setStatus(4);

                emit pushData(DataSet(selected->getId(), dl->getId()));

                qDebug () << "status3->4 Id:" << selected->getId() << dl->getId();
                selected = dl;
                return;
            }
            else
            {
                dl->setStatus(selected->getStatus()); // TODO : selected->getStatus() - 2;
                selected->setStatus(0);
                dl->setStatus(dl->getStatus()-2);
                setStatus(2);

                emit pushData(DataSet(selected->getId(), dl->getId()));
                if(endTurnChk()){
                    // Warning : if these emit's order is broken, it will cause logic confusion.
                    emit pushData(DataSet(123));
                }

                return;
            }
        }
        return;
    }
    else if(status == 4)
    {
        if(dl->getCoord() == selected->getCoord())
        {
            dl->setStatus(dl->getStatus()-2);
            setStatus(2);

            if(endTurnChk()){
                // Warning : if these emit's order is broken, it will cause logic confusion.
                emit pushData(DataSet(123));
            }
            return;
        }
        int valid = validMove(map, selected, dl, 0, 1);
        if(valid == 2)
        {
            if(hasNextMove(map,dl))
            {
                dl->setStatus(selected->getStatus());
                selected->setStatus(0);
                setStatus(4);

                emit pushData(DataSet(selected->getId(), dl->getId()));
                selected = dl;
                return;
            }
            else
            {
                dl->setStatus(selected->getStatus()); // TODO : selected->getStatus() - 2;
                selected->setStatus(0);
                dl->setStatus(dl->getStatus()-2);
                setStatus(2);

                emit pushData(DataSet(selected->getId(), dl->getId()));
                if(endTurnChk()){
                    // Warning : if these emit's order is broken, it will cause logic confusion.
                    emit pushData(DataSet(123));
                }
                return;
            }
        }
    }



}
void MainWindow::processHost() {

    qDebug() << "Start Hosting...";
    if(isClient)
    {
        if(tempInfo != nullptr)
        {
            tempInfo->hide();
            delete tempInfo;
        }
        tempInfo = new QMessageBox(QMessageBox::NoIcon
                                      , "You can't be client and server at the same time."
                                      , "You're connecting to a host currently, so you can't be the host."
                                      , QMessageBox::Ok
                                      , this);
        tempInfo->show();
        return;
    }
    if(hostWindow == nullptr)
    {
        hostWindow = new HostWindow(&serverThread, status, isHost, this);
        hostWindow->setGeometry(this->geometry().x()+300,this->geometry().y()+200,200,250);
        hostWindow->show();

        connect(hostWindow, &HostWindow::connected, this, &MainWindow::processConnected);
        connect(hostWindow, &HostWindow::startListen, this, [=](){labelInfo->setText("Listening:\nWaiting for connection..."); isServer = true;});
        connect(hostWindow, &HostWindow::stopListen, this, [=](){labelInfo->setText("Not connected."); isServer = false;});

    }
    else
    {
        hostWindow->show();
    }
}

void MainWindow::processJoin(){
    qDebug() << "Start Joining...";
    if(isServer)
    {
        if(tempInfo != nullptr)
        {
            tempInfo->hide();
            delete tempInfo;
        }
        tempInfo = new QMessageBox(QMessageBox::NoIcon
                                      , "You can't be client and server at the same time."
                                      , "You're hosting currently, so you can't be the client."
                                      , QMessageBox::Ok
                                      , this);
        tempInfo->show();
        return;
    }

    if(joinWindow == nullptr)
    {
        joinWindow = new JoinWindow(&socketThread, status, this);
        joinWindow->setGeometry(this->geometry().x()+300,this->geometry().y()+200,200,250);
        joinWindow->show();

        connect(joinWindow, &JoinWindow::connected, this, &MainWindow::processConnected);
        connect(joinWindow, &JoinWindow::stopConnecting, this, &MainWindow::processConnectFail);
        connect(joinWindow, &JoinWindow::startConnecting, this, [=]() {isClient = true;} );
    }
    else
    {
        joinWindow->show();
    }

}

void MainWindow::processInitConnect(int result){
    /* Result code:
     * 0 : success
     * 1 : ip addr not valid
     * 2 : init connect timeout
     */

    switch(result){
    case 0:
        break;
    case 1:
        qDebug() << "InitConnect Failed : ip addr not valid";
        isClient = false;
        break;
    case 2:
        qDebug() << "InitConnect Failed : timeout";
        isClient = false;
        break;
    default:
        qDebug() << "processInitConnect Error : Unknown result code";
    }

}

void MainWindow::processConnected() {
    if(isHost)
        connect(this, &MainWindow::pushData, &serverThread, &ServerThread::sendData);
    else
        connect(this, &MainWindow::pushData, &socketThread, &SocketThread::sendData);
    setStatus(0);
}
void MainWindow::processConnectFail(int code) {
    isClient = false;
}

void MainWindow::processDisconnected() {
    qDebug() << "MainWindow::processDisconnected()";
    if(status < 0)
        return;
    setStatus(-2);
    draw("Draw: Connection Lost.");
}

void MainWindow::processReadyPull(){
    // TODO : if ...
    emit pullData();
}

void MainWindow::processIncomingData(DataSet dataSet){
    qDebug() << "MainWindow::processIncomingData" << int(dataSet.getLenData());
    /* Control Code:
     * 0-120 position(id) of the dl
     * 121: start
     *   121-0 : src isFirst   dst isSecond
     *   121-1 : src isSecond  dst isFirst
     * 122: playerTimeout (process : switch turn)
     * 123: normalEndTurn (process : show on the board & status = 1)
     * 124: src defeat (have next byte) (process : show why(by code below) & me=victory)
     *   124-0: player surrender
     *   124-1: 3 timeout -> defeat
     *   124-2: rules defeat (not enough chess outside home)
     * 125: src win (me=defeat)                                Note that the game logic is only pended by myself, op just receive msg.
     * 126:    (reserved)
     * 127: bye-bye (process : end thread, delete buffer, ...)
     */
    // TODO : implement me
    if(dataSet.getLenData() == 0)
        return;
    int first, second = -1;
    if(dataSet.getLenData() >= 2) second = dataSet.getData()[1];
    first = dataSet.getData()[0];

    qDebug() << "MainWindow::processIncomingData receive " << first << ' ' << second;
    if(first <= 120){
        dls[second]->setStatus(dls[first]->getStatus());
        dls[first]->setStatus(0);
        opSelected = dls[second];
   }  else if(first == 121){
        // asssert(status == 0)
        isFirst = second;
        if(isFirst)
        {
            setStatus(1);
            labelColor->setText("You = RED\nOpponent = BLUE");
            timerLcd->display(20);
            timer->stop();
            timer->start(1000);
        }
        else
        {
            setStatus(2);
            labelColor->setText("You = BLUE\nOpponent = RED");
            timer->start(LostConnectTimeSec*1000);
        }
    } else if(first == 122){
        if(opSelected != nullptr && opSelected->getStatus() > 2)
            opSelected->setStatus(opSelected->getStatus()-2);
        startTurn();
    } else if(first == 123){
        if(opSelected != nullptr && opSelected->getStatus() > 2)
            opSelected->setStatus(opSelected->getStatus()-2);
        startTurn();
    } else if(first == 124){
        switch(second){
        case 0:
            // surrender
            Iwin("The opponent is afraid of you.");
            break;
        case 1:
            //playerTimeoutDefeat
            Iwin("The oppoent is sleeping?");
            break;
        case 2:
            // TODO rules defeat
            Iwin("The opponent moves so slow.");
            break;
        }
        endGame();
    } else if(first == 125){
        Ilose("The opponent achieved victory...");
        endGame();
    } else if(first == 127){
        // ? is this right?
        endGame();
    }
}

void MainWindow::processStart(){
    qDebug() << "MainWindow::processStart(): " << status << ' ' <<isHost;
    if(status != 0)
    {
        if(tempInfo != nullptr)
        {
            tempInfo->hide();
            delete tempInfo;
        }
        tempInfo = new QMessageBox(QMessageBox::NoIcon
                                       , "Start Game"
                                       , "You can't start the game now."
                                       , QMessageBox::Ok
                                       , this);
        tempInfo->show();
        return;
    }
    isFirst = boolRand();
    if(isFirst){
        setStatus(1);
        timerLcd->display(20);
        timer->start(1000);
        labelColor->setText("You = RED\nOpponent = BLUE");
    }
    else{
        setStatus(2);
        timer->stop();
        timer->start(LostConnectTimeSec*1000);
        labelColor->setText("You = BLUE\nOpponent = RED");
    }

    //send msg to op
    qDebug() << "send init msg";
    char* buf = new char[2];
    buf[0] = 121; buf[1] = 1-isFirst;
    DataSet ret(buf, 2);
    emit pushData(ret);
    delete[] buf;
}

bool MainWindow::endTurnChk(){
    // note : even if player timeout , this function should be called.
    // timer lose
    // timer upd
    int homeFirst = 0;
    int awayFirst = 0;
    int homeSecond = 0;
    int awaySecond = 0;
    static const int MaxLeftTurn[3] = {20, 25, 30}; // to be considered
    static const int MaxLeft[3] = {5,2,0};

    timer->stop();
    timer->start(LostConnectTimeSec*1000);

    for(int i = 0; i < 10; i++)
    {
        if(dls[i]->getStatus() == 1)
            ++awayFirst;
        else if(dls[i]->getStatus() == 2)
            ++homeSecond;
    }
    for(int i = 111; i < 121; i++)
    {
        if(dls[i]->getStatus() == 1)
            ++homeFirst;
        else if(dls[i]->getStatus() == 2)
            ++awaySecond;
    }
    qDebug() << "homeFirst:" << homeFirst << "awayFirst:" << awayFirst;
    qDebug() << "homeSecond:" << homeSecond << "awaySecond:" << awaySecond;

    // important
    if(!isFirst){ // processSecond
        std::swap(homeFirst, homeSecond);
        std::swap(awayFirst, awaySecond);
    }

    if(awayFirst == 10){
        Iwin("You're so fast.");
        emit pushData(DataSet(125));
        endGame();
        return false;
    }
    for(int i = 0; i < 3; i++)
        if(turn == MaxLeftTurn[i] && homeFirst > MaxLeft[i]){
            Ilose("You're so slow.");
            emit pushData(DataSet(124,2));
            endGame();
            return false;
        }

    if(timeoutCnt == 3){
        Ilose("Defeat because of 3 timeout.");
        emit pushData(DataSet(124,1));
        endGame();
        return false;
    }



    if(!isFirst)
    {
        ++turn; // at the end of the function
        lcdTurn->Increase();
    }
    setStatus(2);
    return true; // return true means continue to play normally
}

void MainWindow::processSurrender(){
    qDebug() << "MainWindow::processSurrender()";
    if(status <= 0 || status == 2) // note : you can only surrender in your turn, when game is running.
    {
        if(tempInfo != nullptr)
        {
            tempInfo->hide();
            delete tempInfo;
        }
        tempInfo = new QMessageBox(QMessageBox::NoIcon
                                      , "Surrender"
                                      , "You can only surrender in your turn, when game is running."
                                      , QMessageBox::Ok
                                      , this);
        tempInfo->show();
        return;
    }
    Ilose("You surrendered.");
    emit pushData(DataSet(124,0));
    endGame();
}

void MainWindow::Iwin(QString info){
    endGameInfo = new QMessageBox(QMessageBox::NoIcon
                                  , "Victory"
                                  , "Congratulations, you win!\n" + info
                                  , QMessageBox::Ok
                                  , this);
    endGameInfo->show();
}
void MainWindow::Ilose(QString info){
    endGameInfo = new QMessageBox(QMessageBox::NoIcon
                                  , "Defeat"
                                  , "You lose.\nHow can you miss that?\n" + info
                                  , QMessageBox::Ok
                                  , this);
    endGameInfo->show();
}
void MainWindow::draw(QString info){
    endGameInfo = new QMessageBox(QMessageBox::NoIcon
                                  , "Draw"
                                  , info
                                  , QMessageBox::Ok
                                  , this);
    endGameInfo->show();
}
void MainWindow::endGame(){
    setStatus(-2);
    timer->stop();
    timerLcd->display(0);

}
void MainWindow::processSocketDestroyed() {
    emit disconnectTcp();
    if(serverThread.isRunning())
        serverThread.exit();
    if(socketThread.isRunning())
        serverThread.exit();
}
void MainWindow::startTurn() {
    if(isFirst)
    {
        ++turn;
        lcdTurn->Increase();
    }
    setStatus(1);
    timerLcd->display(20);
    timer->stop();
    timer->start(1000);


}
void MainWindow::processTimerUpdate(){
    if(status == 2){ // lost connection
        processDisconnected();
        endGame();
    }
    timerLcd->Increase();
    if(timerLcd->value() > 0)
        return;
    qDebug() << "middle of timerUpdate";
    if(status == 1 || status == 3 || status == 4) {// playerTimeOut
        timeoutCnt++;
        if(selected != nullptr && selected->getStatus() > 2)
            selected->setStatus(selected->getStatus()-2);
        if(endTurnChk())
            emit pushData(DataSet(122));
    }
}

void MainWindow::setStatus(int x)
{
    status = x;
    switch(x){
    case -2:
        labelInfo->setText("Game Ends.");
        break;
    case -1:
        labelInfo->setText("Not connected");
        break;
    case 0:
        labelInfo->setText("Connected.\nUse Play->Start tab to start the game.");
        break;
    case 1:
        if(isFirst)
            labelInfo->setText("Your Turn:\nMove the RED checker.");
        else
            labelInfo->setText("Your Turn:\nMove the BLUE checker.");
        break;
    case 2:
        labelInfo->setText("Opponent's Turn.");
        break;
    default:
        break;
    }
}
