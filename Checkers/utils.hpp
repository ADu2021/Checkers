#ifndef UTILS_HPP
#define UTILS_HPP

#include <QPoint>
#include <QString>
#include <QList>
#include <QtMath>
#include <QHostInfo>
#include <QHostAddress>
#include <regex>
#include <ctime>

#include "hexcoord.hpp"
#include "drawlabel.hpp"

inline int maxx(int x, int y) {return x > y ? x : y;}
inline int abss(int x) {return x < 0 ? -x : x;}
inline bool boolRand() { return rand()%2; }

inline QString QPointToQString(QPoint cor, int type = 0){
    QString ss;
    ss += ss.number(cor.x(),10);
    ss += ",";
    ss += ss.number(cor.y(),10);

    if(type == 1)
        ss = "(" + ss + ")";
    return ss;
}

inline QList<QPoint> calcPos(const QPoint& start, const int& width, const int& step)
{
    // note : calculate process using double, but result using int.
    static const int len = 17;
    static const int cnt[len] = {1, 2, 3, 4, 13, 12, 11, 10, 9, 10, 11, 12, 13, 4, 3, 2, 1};
    static const double ratio = qSqrt(3) / 2.0;
    double dy = step*ratio;
    double mid = start.x() + (width/2.0);
    QList<QPoint> ret;
    ret.append(start);
    double leftC; // the center of the left side circle.
    double nowX;
    double nowY = start.y() + dy;
    for(int i = 1; i < len; i++)
    {
        leftC = mid - ((cnt[i]-1)/2.0)*step;
        nowX = leftC - (width/2.0);
        for(int j = 0; j < cnt[i]; j++)
        {
            ret.append(QPoint(int(nowX), int(nowY)));
            nowX += step;
        }
        nowY += dy;
    }
    return ret;
}

inline QList<HexCoord> calcCoord()
{
    static const int len = 17;
    static const int cnt[len] = {1, 2, 3, 4, 13, 12, 11, 10, 9, 10, 11, 12, 13, 4, 3, 2, 1};
    static HexCoord start(-4, -4, 8);
    HexCoord midl = start, left;
    QList<HexCoord> ret;
    ret.append(start);
    for(int i = 1; i < len; i++)
    {
        if(i&1)
            midl = midl + HexCoord::dir(4); // downLeft
        else
            midl = midl + HexCoord::dir(5); // downRight

        left = midl;
        for(int j = 0, steps = (cnt[i]-1)/2; j < steps; j++)
            left = left + HexCoord::dir(3); // left
        for(int j = 0; j < cnt[i]; j++)
        {
            ret.append(left);
            left = left + HexCoord::dir(0); // right
        }
    }

    return ret;
}

inline int validMove(const QMap<HexCoord,DrawLabel*>& map, DrawLabel* src, DrawLabel* dst, bool allowWalk, bool allowJump)
{
    //return true; // test
    // Reture Value: 0-false, 1-walkOK, 2-jumpOK
    // Assume that src and dst are not the same point
    // TODO: handle the src===dst case outside this file(maybe in gamelogic)
    HexCoord from = src->getCoord();
    HexCoord to = dst->getCoord();

    qDebug() << "src: " << from.x << ' ' << from.y << ' ' << from.z;
    qDebug() << "dst: " << to.x << ' ' << to.y << ' ' << to.z;

    // chk if HexCoord to is inside the map
    //if(!map.contains(from) || !map.contains(to))
        //return 0;

    // chk if this move a walk or jump(on the same line)
    int diffCnt = 0;
    if(from.x != to.x) diffCnt++;
    if(from.y != to.y) diffCnt++;
    if(from.z != to.z) diffCnt++;
    qDebug() << "diffCnt: " << diffCnt;
    if(diffCnt == 3) return 0;


    int dist = maxx(abs(from.x-to.x), abs(from.y-to.y));
    qDebug() << "dist: " << dist;

    if(dist == 1) // walk
        return ((map[to]->getStatus() == 0) && allowWalk)  ? 1 : 0; // empty
    else if(dist == 2)
    {
        HexCoord mid((from.x+to.x)>>1, (from.y+to.y)>>1, (from.z+to.z)>>1);
        return ((map[to]->getStatus() == 0) && (map[mid]->getStatus() != 0) && allowJump) ? 2 : 0; // dst empty and there is a 'bridge' to jump
    }
    return 0; // too far
}

inline bool hasNextMove(const QMap<HexCoord,DrawLabel*>& map, DrawLabel* now)
{
    DrawLabel *nxt;
    HexCoord nxtC;
    for(int i = 0; i < 6; i++)
    {
        nxtC = now->getCoord() + HexCoord::dir(i) + HexCoord::dir(i);
        if(!map.contains(nxtC))
            continue;
        nxt = map[nxtC];
        if(validMove(map,now,nxt,0,1))
            return true;
    }
    return false;
}

inline bool isIp(QString addr)
{
    static std::regex re(R"(^((25[0-5]|(2[0-4]|1\d|[1-9]|)\d)(\.(?!$)|$)){4}$)");
    return std::regex_match(addr.toStdString(), re);
}

inline QString myIp()
{
    QList<QHostAddress> addrs = QHostInfo::fromName(QHostInfo::localHostName()).addresses();
    QString strAddr("");
    for(int i = 0; i < addrs.length(); i++)
        if(isIp(addrs[i].toString()))
            strAddr = strAddr + addrs[i].toString() + "\n";
    return strAddr;
}

#endif // UTILS_HPP
