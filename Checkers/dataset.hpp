#ifndef DATASET_HPP
#define DATASET_HPP

#include <QDebug>

inline void charncpy(char *dest, const char *src, int len){
    for(int i = 0; i < len; i++)
        dest[i] = src[i];
    return;
}

class DataSet{

    char* data;
    int lenData;

public:
    DataSet(const char* _data, int _len)
        : data(new char[_len+2]), lenData(_len)
    {
        for(int i= 0; i < lenData; i++)
            qDebug() << "org:" << int(_data[i]);

        charncpy(data, _data, _len);

        qDebug() << "DataSet Construct";
        for(int i = 0; i < lenData; i++)
            qDebug() << "now:" <<int(data[i]);
    }
    DataSet(int inp) : data(new char[3]), lenData(1) { data[0] = inp; }
    DataSet(int inpx, int inpy) : data(new char[4]), lenData(2) { data[0] = inpx; data[1] = inpy;
                                                                qDebug() << "Construct DataSet" << int(data[0]) << int(data[1]);}
    DataSet(const DataSet& other) : DataSet(other.data, other.lenData) {qDebug() << "DataSet Copy Construct";        for(int i = 0; i < lenData; i++)
            qDebug() << int(data[i]);}
    DataSet(DataSet&& other)
        : data(other.data), lenData(other.lenData)
    {
        qDebug() << "DataSet Move Construct";
        other.data = nullptr;
        other.lenData = 0;
    }
    DataSet& operator = (const DataSet& other){
        if(other.data == nullptr) return *this;
        if(data != nullptr) delete[] data;
        qDebug() << "DataSet Copy Assignment";
        data = new char[other.lenData+2];
        charncpy(data, other.data, other.lenData);
        lenData = other.lenData;
        return *this;
    }
    DataSet& operator = (DataSet&& other){
        if(other.data == nullptr) return *this;
        if(data != nullptr) delete[] data;
        qDebug() << "DataSet Move Assignment";
        data = other.data;
        other.data = nullptr;
        lenData = other.lenData;
        other.lenData = 0;
        return *this;
    }
    ~DataSet(){
        if(data != nullptr)
            delete[] data;
    }
    char* getData() {return data;}
    int getLenData() {return lenData;}

};

#endif // DATASET_HPP
