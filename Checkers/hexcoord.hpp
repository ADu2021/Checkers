#ifndef HEXCOORD_HPP
#define HEXCOORD_HPP

class HexCoord{
public:
    HexCoord(int _x = 0, int _y = 0, int _z = 0) : x(_x), y(_y), z(_z) {}

    static HexCoord dir(int); // 0<=x<=5

    HexCoord operator + (HexCoord other) { return HexCoord(x+other.x, y+other.y, z+other.z); }
    bool operator == (HexCoord other) {return ((x==other.x) && (y==other.y) && (z==other.z));}
    bool operator != (HexCoord other) {return !((*this)==other);}

    int x;
    int y;
    int z;
};

inline bool operator < (HexCoord a, HexCoord other)
{
    if(a.x != other.x) return a.x < other.x;
    if(a.y != other.y) return a.y < other.y;
    return a.z < other.z;
}

inline HexCoord HexCoord::dir(int x)
{
    int dx[6] = {1,0,-1,-1,0,1};
    int dy[6] = {-1,-1,0,1,1,0};
    int dz[6] = {0,1,1,0,-1,-1};
    return HexCoord(dx[x], dy[x], dz[x]);
}

#endif // HEXCOORD_HPP
