#ifndef INCLCDNUMBER_HPP
#define INCLCDNUMBER_HPP

#include <QLCDNumber>

class IncLCDNumber : public QLCDNumber{
    Q_OBJECT
public:
    IncLCDNumber(QWidget* parent = nullptr) : QLCDNumber(parent) {}
    void setStep(int x) { step = x; }
private:
    int step = 1;
    void increase(){
        this->display(this->value()+step);
    }
public slots:
    void Increase() { increase(); }
};

#endif // INCLCDNUMBER_HPP
