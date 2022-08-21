#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenu>
#include <QAction>
#include <QMap>
#include <QMessageBox>
#include <QTimer>

#include "drawlabel.hpp"
#include "inclcdnumber.hpp"
#include "joinwindow.hpp"
#include "hostwindow.hpp"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
signals:
    void pullData();
    void pushData(DataSet dataSet);

    void disconnectTcp();

public slots:
    void processDlClick(DrawLabel* dl); //  click on the board
    void processHost(); // click "host" qaction
    void processJoin(); // click "join" qaction
    void processStart();
    void processSurrender();

    void processInitConnect(int result); // handle SocketThread::initConnectResult

    void processConnected() ;
    void processConnectFail(int);
    void processDisconnected();

    void processReadyPull();
    void processIncomingData(DataSet dataSet); // handle incoming data from thread
    void processTimerUpdate();

    void processSocketDestroyed();

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    static const int boardSize = 121;
    QList<DrawLabel*> dls; // which is supposed to have 121 elements.
    QMap< HexCoord, DrawLabel* > map;


    QMenu *menuConnect;
    QMenu *menuPlay;
    QAction *actionHost;
    QAction *actionJoin;
    QAction *actionStart;
    QAction *actionSurrender;

    HostWindow *hostWindow = nullptr;
    JoinWindow *joinWindow = nullptr;
    QMessageBox *endGameInfo = nullptr;
    QMessageBox *tempInfo = nullptr;

    bool isServer = false; // listening or hosting
    bool isClient = false; // connecting to host or being client
    // assert(~(isServer & isClient))

    IncLCDNumber *lcdTurn;

    QLabel *labelInfo;
    QLabel *labelColor;
    QLabel *labelTimeLeft;
    QLabel *labelTurn;

    DrawLabel *selected = nullptr;
    DrawLabel *opSelected = nullptr;
    bool isHost = false;
    bool isFirst = false;
    int status = -1;
    void setStatus(int x);

    /* MainWindow Status Code:
     * -2: Game ends.
     * -1: Not connected.
     * 0: Connected, but the game isn't started yet.
     * 1: Game starts, waiting for commands.
     * 2: Waiting the opponent(network communication) i.e. Opponent's Turn
     * 3: Checker selected, waiting for destination command (for the first time).
     * 4: Checker alrealy jumped, but it can jump another time.
     * */
    int turn = 1;

    QTimer *timer = nullptr;
    IncLCDNumber *timerLcd = nullptr;
    int timeoutCnt = 0;

    ServerThread serverThread;
    SocketThread socketThread;

    /* Control Code:
     * 0-120 position(id) of the dl
     * 121: start
     * 122: playerTimeout (process : switch turn)
     * 123: endTurn (process : show on the board & status = 1)
     * 124: src defeat (have next byte) (process : show why(by code below) & me=victory)
     *   124-0: player surrender
     *   124-1: 3 timeout -> defeat
     *   124-2: rules defeat (not enough chess outside home)
     * 125: src win (me=defeat)                                Note that the game logic is only pended by myself, op just receive msg.
     * 126:
     * 127: bye-bye (process : end thread, delete buffer, ...)
     */

    void startTurn();
    bool endTurnChk();
    void Iwin(QString = "");
    void Ilose(QString = "");
    void draw(QString = "");
    void endGame();

    Ui::MainWindow *ui;

    static const int LostConnectTimeSec = 60;

};
#endif // MAINWINDOW_H
