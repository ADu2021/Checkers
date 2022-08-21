# 网络对战跳棋小游戏

杜一阳 2021011778 计11 duyy21@mails.tsinghua.edu.cn

2022 THUCST summer term

[TOC]

## 总体结构

本项目使用QT框架（C++，版本6.3.1，采用LGPLv3开源协议）实现了中国跳棋的游戏逻辑、网络联机功能以及相应的图形界面。

## 图形界面（GUI）

### 窗口设计

- 主窗口中部及右侧为棋盘，左侧从上至下分别为剩余走子时间、回合数、双方阵营棋子颜色以及当前操作方信息。左上角选项卡中有“Host a game”（作为Server）、“Join a game”（作为Client）、“Start”（开始游戏）和“Surrender”（投降）。

- Join窗口中从上至下分别有Host IP输入框（默认localhost），连接按钮，取消按钮，消息label。

- Host窗口中从上至下分别有消息label（含本机IP），连接按钮，取消按钮。

- Join、Host窗口阻塞主窗口。

<img title="" src="https://i.imgur.com/B18kr4A.png" alt="" width="401">

<img title="" src="https://i.imgur.com/68yqaKk.png" alt="" width="154">        <img title="" src="https://i.imgur.com/EzzzT3C.png" alt="" width="155">

### 绘制方法

棋盘的绘制主要通过继承QLabel类并重写覆盖其paintEvent和mouseReleaseEvent函数实现。其中，paintEvent用于将label绘制为棋子（或一个空位）的形状，mouseReleaseEvent用于emit Clicked()信号供主窗口处理逻辑。

```cpp
class DrawLabel : public QLabel {
    ...
signals:
    void Clicked(DrawLabel* dl);
public:
    void paintEvent(QPaintEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
};
```

中国跳棋的棋盘有121个位置，为了绘制这121个位置，在utils.hpp中实现了calcPos()函数。

```cpp
inline QList<QPoint> calcPos
    (const QPoint& start, const int& width, const int& step);
```

该函数以起始点（棋盘最上方的那个棋子的位置）、绘制宽度、步长（相邻两个棋子中心的距离）为参数。计算方式为逐行通过该行的棋子数量计算出最左侧的棋子所在位置，再通过步长遍历计算该行其余棋子的位置。~~这比打一个121大小的表要优雅一些。~~

类似地还实现了calcCoord()函数用于计算棋子的*坐标*（*坐标*是在游戏逻辑中用到的，后面会介绍，不同于此处用于绘制的*位置*）。

## 通信

### 通信协议

通信协议规定，游戏逻辑部分所需要传输的信息都封装为一个DataSet类。DataSet类中只有两个（私有）变量成员：指向数据的char指针data与表示数据长度的lenData。此外还实现了几种构造函数、析构函数、拷贝（移动）构造（赋值）等函数。

```cpp
class DataSet {
    ...
    char* data;
    int lenData;
};
```

协议规定以Qt的tcpSocket类的read()与write()函数作为底层的读写函数，并且所有传输的数据均为char类型，**包括表示长度的数据在内。** 这意味着长度最多能表示127位，但因为中国跳棋游戏过于简单，实际需要中最多只需要表示2位，所以为了简化读写逻辑在传输过程中也使用char类型表示数据长度。

DataSet的发送方式为：（首先转换为大尾端）发送一个char的len表示后边跟随的数据长度，然后发送len个char（即DataSet.data）中的数据。

DataSet的接受方式为：先接收一个char发送一个char的len表示后边跟随的数据长度，然后接收len个char（即DataSet.data）中的数据。（然后从大尾端转换）

### 通信实现

通信的实现主要在继承了QTcpSocket的MyTcpSocket类，并通过信号槽机制实现了其功能。

```cpp
class MyTcpSocket : public QTcpSocket {
    ...
    QTcpSocket *socket;
    char* buffer;
    int bufferCnt = 0;
signals:
    void ready(); // init
    void readyPull(); // read
    void pullData(); // read
    void incomingData(DataSet); // read
slots:
    void setSocket(QTcpSocket*); // init
    void processReadyRead(); // read
    void processPullData(); // read
    void pushData(DataSet); // write
};
```

初始化：由于Tcp协议Server端需要在建立连接后才能返回一个QTcpSocket，因此MyTcpSocket作为一个适配器需要有`setSocket()`函数在构造后获得需要管理的socket。这就意味着在此之前进行读写操作是不安全的，因此借助Qt的信号槽机制，设计了在完成setSocket之后才发送`ready()`信号，保证了读写操作的安全。

读：由于后面的设计里MyTcpSocket是在子线程中执行的，因此需要异步地发送信号，借助Qt的事件循环将DataSet传递给主线程。具体实现如下：

- 当socket发送`readyRead()`信号时，意味着至少有一个char可以读进来，这时候连接的槽函数`processReadyRead()`将这些char读进来并添加在buffer后面。由于通信协议保证了第一个char是消息长度，我们可以由此判断出在此次read操作后是否有一个完整的DataSet等待程序处理，如果是，则触发`readyPull()`信号给相应的线程管理类。

- 当线程管理类接到主线程的读取数据信号后，向MyTcpSocket发送`pullData()`信号，进而触发连接的槽函数`processPullData()`，将数据以信号`incomingData(DataSet)`的形式返回。

写：只需触发`pushData(DataSet)`槽函数，就可按照通信协议发送数据。

### 多线程

为了避免读写数据对主线程的阻塞造成窗体僵死等情况，将IO放到了子线程中完成。通过继承QThread来实现。~~似乎有人说继承的方法不是Qt实现多线程的最佳方式？Whatever.~~

#### 服务器子线程（ServerThread）

这里主要介绍其信号-槽机制的设计：

```cpp
class ServerThread: public QThread {
    ...
    QTcpServer* server;
    MyTcpSocket* _tcpSocket;
signals:
    // init
    void initialized(); // for HostWindow
    void connected(); // for HostWindow
    void sendSocket(QTcpSocket* _socket); // for _tcpSocket
    // gaming
    void incomingData(DataSet dataSet); // [read] [up] with mainWindow
    void readyPull(); // [read] [up] with mainWindow
    void pullData(); // [read] [down] with tcp socket
    void pushData(DataSet dataSet); // [write] [down] with tcp socket
    // end
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
    //end
    void processDisconnect();
};
```

初始化：`initialized()`与信号用于告知Host窗口线程已初始化，此后可以调用`startListening()`槽告知server开始监听，并在等待连接的过程中调用`stopListening()`停止监听。在接到server的`newConnection()`信号后，槽函数`processNewConnection()`将接受连接，并将得到的socket通过`sendSocket()`信号给到作为Adapter的MyTcpSocket。在MyTcpSocket接收完socket之后，其发送的`ready()`信号将被连接的`processSocketReady()`接收，并在连接信号槽后向Host窗口发出`connected()`信号，标志着连接的建立。

游戏中：`incomingData()`，`readyPull()`与`pullData()`信号用于转发MainWindow和socket之间对于读操作的信号，`pushData()`信号则用于转发写操作的信号。

结束时：`disconnected()`信号用于告知Host窗口与MainWindow连接断开，这既可能发生于连接建立失败，又可能发生于连接建立后的意外断开。`socketDestroyed()`信号转发socket的`destroyed()`信号，便于线程安全结束。

#### 客户端子线程（SocketThread）

客户端子线程与服务器子线程在IO等方面完全一致~~（这里如果用继承一个共同的基类来优化一下复用性更好）~~，但由于客户端没有监听以及新产生socket的过程，其初始化（init）的信号-槽机制有所不同，下面只介绍这部分。

```cpp
class SocketThread: public QThread {
    ...
    MyTcpSocket* _tcpSocket = nullptr;
signals:
    void initialized();
    void initConnectResult(int);
public slots:
    void processStateChanged(MyTcpSocket::SocketState);
    void initConnect(QString);
};
```

`initConnect()` 槽函数用于向socket传递host IP供发起连接。由于连接过程可能耗时，并且Qt文档说同步等待连接的函数 *“Randomly fails on Windows”*  ，所以采用异步的方式处理结果。在接收到socket状态变化时执行连接的槽函数`processStateChanged()`，并通过`initConnectResult()`信号传递连接结果。

## 游戏逻辑

### 状态机

游戏逻辑采用状态机实现，具体状态及转化如下：

- -2 ：游戏已结束

- -1：未连接
  
  - 作为服务器或客户端成功连接时，转0

- 0：已连接，但游戏未开始
  
  - 点击“Start”开始，随机先后手：
    
    - 若先手，转1
    
    - 若后手，转2
  
  - 接收到对方发送的游戏开始消息和先后手信息：
    
    - 若先手，转1
    
    - 若后手，转2

- 1：回合开始，等待指定棋子
  
  - 点击己方棋子，转3
  
  - 点击“Surrender”投降，转-2
  
  - 己方玩家超时：
    
    - 若小于三次，转2
    
    - 若大于三次，转-2

- 2：对方回合
  
  - 仅接收到对方移动消息时，仍为2
  
  - 接收到对方回合结束or玩家超时（小于三次）消息时，转1
  
  - 接收到对方玩家胜利/失败消息时，转-2
  
  - TCP连接关闭或60s未发送消息时，转-2（平局：失去连接）

- 3：已指定棋子（首次）
  
  - 点击距离为1的空位时：
    
    - 若游戏未结束，转2
    
    - 若游戏结束（胜利or犯规判负），转-2
  
  - 点击距离为2的空位时：
    
    - 若还能再跳，转4
    
    - 若不能再跳且游戏未结束，转2
    
    - 若不能再跳且游戏结束（胜利or犯规判负），转-2
  
  - 点击“Surrender”投降，转-2
  
  - 己方玩家超时：
    
    - 若小于三次，转2
    
    - 若大于三次，转-2

- 4：已指定棋子（非首次，即已经跳了一步且还可再跳）
  
  - 点击距离为2的空位时：
    
    - 若还能再跳，转4
    
    - 若不能再跳且游戏未结束，转2
    
    - 若不能再跳且游戏结束（胜利or犯规判负），转-2
  
  - 点击“Surrender”投降，转-2
  
  - 己方玩家超时：
    
    - 若小于三次，转2
    
    - 若大于三次，转-2

当状态为0,1,3,4时，无论发生何种状态转换，都会向对方发送相应信息。

### MainWindow的信号-槽机制

信号`pullData()`和`pushData()`用于收发信息。共有四组槽函数：“process click”组用于处理对于棋盘&选项卡的点击，“connection”组用于接受服务器或客户端的连接信号，“process message”用于游戏中接受异步处理的读入信息，“timer”用于处理计时器的信号（己方回合用于每秒更新显示并判断超时，对方回合用于判断连接超时）。

```cpp
class MainWindow : public QMainWindow {
    ...
signals:
    void pullData();
    void pushData(DataSet dataSet);
public slots:
    // process click
    void processDlClick(DrawLabel* dl); //  click on the board
    void processHost(); // click "host" qaction
    void processJoin(); // click "join" qaction
    void processStart();
    void processSurrender();
    // connection
    void processInitConnect(int result); // handle SocketThread::initConnectResult
    void processConnected() ;
    void processConnectFail(int);
    void processDisconnected();
    // process message
    void processReadyPull();
    void processIncomingData(DataSet dataSet); // handle incoming data from thread
    // process timer
    void processTimerUpdate();
};
```

### 棋盘坐标设计：六边形坐标

<img title="" src="https://img2018.cnblogs.com/blog/1523767/201812/1523767-20181206114048551-2115714491.png" alt="" width="400">

中国跳棋的逻辑需要处理“相邻一个/两个距离的位置”。这当然可以通过检查两个棋子位置之间的距离来实现，但这种方法需要依赖棋子之间的绘制距离，而这个距离又是在一个小范围浮动的，且容易受到布局、绘制的影响，~~不够优雅~~。于是我采用[六边形坐标](https://www.cnblogs.com/heweiwei/p/10075896.html)的方式来处理这部分的逻辑。初始化棋盘时，用类似计算棋子绘制位置的方式计算其六边形坐标。

### IP地址：获取与判断

通过`std::regex`类判断是否为合法的IPv4地址，用于服务端获取本机地址和客户输入检查，正则表达式为：

```regex
^((25[0-5]|(2[0-4]|1\d|[1-9]|)\d)(\.(?!$)|$)){4}$
```

通过`QHostInfo::fromName(QHostInfo::localHostName()).addresses()`获取本机地址，并通过上述正则表达式提取出符合IPv4地址的部分。（否则会有一些::1开头的地址）

## 参考资料

- 小学期课程&课件

- [Qt Documentation](https://doc.qt.io/qt-6/)

- [Validating IPv4 addresses with regexp - Stack Overflow](https://stackoverflow.com/questions/5284147/validating-ipv4-addresses-with-regexp)

- [解决cmake的find_package问题 - CSDN](https://blog.csdn.net/giggle5/article/details/124612145)

- [一文理解QThread多线程 - CSDN](https://blog.csdn.net/weixin_40774605/article/details/109259653)

- [How to Create Executable & Installer (.Exe) File for QT Project - YouTube](https://www.youtube.com/watch?v=hCXAgB6y8eA)

- [六边形地图Cube coordinates理解  - cnblogs](https://www.cnblogs.com/heweiwei/p/10075896.html)
