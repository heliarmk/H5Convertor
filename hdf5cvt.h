#ifndef HDF52IMG_H
#define HDF52IMG_H
#define H5_BUILT_AS_DYNAMIC_LIB

#include <H5Cpp.h>
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <QInputDialog>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QString>
#include <QDebug>
#include <QDateTime>
#include <QObject>

using namespace H5;

class HDF5convertor : public QObject
{
    Q_OBJECT
public:
    explicit HDF5convertor(QObject *parent = 0, QString datasetpath = "", QString outputdir = "",QString type = "", QString filetype = "", bool isLog = false);

signals:
    void totalFrameNum(qulonglong count);
    void finishedFrameCount(qulonglong count);
    void finished();

public slots:
    bool cvtH5();

private slots:
    bool convertFile();
    bool readH5File();
    bool closeH5File();

private:
    QString _dataset_name;
    QString _storeDir;
    QString _filetype;
    QString _type;
    bool _isLog;
    QString _outputFilePath;
    QDateTime _currentTime;
    DataSpace _dataSpace;
    QString _dataFormat;
    DataSet _dataset;
    DataType h5_readType;
    H5File *hFilePtr;
    hsize_t _dims[3];

    cv::Mat _bufferFrame;
    std::vector<double> timestamps;
    struct ParaConfig
    {
        qint32 dutyCycle;
        qint32 frequency;
        qint32 periodCount;
        qint32 stimulusCount;
        qint32 stimulusInterval;
        qint32 direction;
    };
    std::vector<ParaConfig> stiParams;
    cv::VideoWriter *_writerPtr;

    int _codec;		//视频编码模式
    double _fps;	//视频播放fps
    //log File
    QFile *_logFilePtr;
};

#endif // HDF52IMG_H
