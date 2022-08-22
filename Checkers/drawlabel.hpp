#ifndef DRAWLABEL_HPP
#define DRAWLABEL_HPP

#include <QObject>
#include <QLabel>
#include <QImage>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QRectF>

#include "hexcoord.hpp"

class DrawLabel : public QLabel
{
    Q_OBJECT
    typedef QLabel super;

signals:
    void Clicked(DrawLabel* dl);

public:
    DrawLabel(QWidget* parent = nullptr, HexCoord coord = HexCoord(0,0,0), int Id = 0) : super(parent), _parent(parent), // the parent of DrawLabel should only be MainWindow.
        cor(coord), id(Id),
        im(":/image/res/whiteButton.png"),
        imr(":/image/res/redButton.png"),
        imb(":/image/res/blueButton.png"),
        imrFx(":/image/res/shiningRed.png"),
        imbFx(":/image/res/shiningBlue.png")
        {}

    void paintEvent(QPaintEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    static const int len = 30;

    int getStatus() const {return status;}
    void setStatus(int x) {status = x; this->update();}
    int getId() const {return id;}

    HexCoord getCoord() const {return cor;}

private:
    HexCoord cor;
    int id;

    int status = 0;
    /* DrawLabel status code:
     * 0 : Emplt Slot
     * 1 : Red Normal
     * 2 : Blue Normal
     * 3 : Red Selected
     * 4 : Blue Selected
     */

    QImage im;
    QImage imr;
    QImage imb;
    QImage imrFx;
    QImage imbFx;

    QWidget* _parent;
};

inline void DrawLabel::paintEvent(QPaintEvent *event){
    super::paintEvent(event);
    QPainter *painter = new QPainter(this);
    QRectF target(0,0,len,len);
    if(status == 0)
        painter->drawImage(target, im);
    else if(status == 1)
        painter->drawImage(target, imr);
    else if(status == 2)
        painter->drawImage(target, imb);
    else if(status == 3)
        painter->drawImage(target, imrFx);
    else if(status == 4)
        painter->drawImage(target, imbFx);
    painter->end();
}

inline void DrawLabel::mouseReleaseEvent(QMouseEvent *event){
    // note that the logic of the game should not be processed by DrawLabel (here).
    if(event->button() == Qt::LeftButton)
        emit Clicked(this);

}

#endif // DRAWLABEL_HPP
