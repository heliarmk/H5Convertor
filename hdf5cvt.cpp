#include "hdf5cvt.h"

HDF5convertor::HDF5convertor(QObject *parent, QString datasetpath, QString outputdir,QString type, QString filetype, bool isLog)
    : QObject(parent)
    , _dataset_name(datasetpath)
    , _storeDir(outputdir)
    , _filetype(filetype)
    , _type(type)
    , _isLog(isLog)
{
    _currentTime = QDateTime::currentDateTime();

}

bool HDF5convertor::readH5File()
{
#ifdef Q_OS_WIN32
    hFilePtr = new H5::H5File(_dataset_name.toStdString().c_str(), H5F_ACC_RDONLY);
#else
    H5::H5File *h5_hFilePtr = new H5::H5File(dataset_name.toUtf8().data(), H5F_ACC_RDONLY);
#endif

    // first find the name of the dataset. This opens all of them to see which ones have 3 dimensions
    // if there are multiple datasets. Give the option to the user which one to choose.
    hsize_t amDataSets = hFilePtr->getNumObjs();
    //        qDebug()<<"Amount datasets is: "<<amDataSets;
    std::vector<H5std_string> goodSets;
    QStringList items;  // only used if more than one valid dataset present

    bool timepresent = false;
    bool stiparamspresent = false;
    
    std::string timestring;
    std::string stiparamstring;

    for (hsize_t i = 0; i < amDataSets; i++)
    {
        qDebug() << "begin";
        std::string theName = hFilePtr->getObjnameByIdx(i);
        if (QString::fromStdString(theName).contains("time"))
        {
            timepresent = true;
            timestring = theName;
        }
        if (QString::fromStdString(theName).contains("stiParam"))
        {
            stiparamspresent = true;
            stiparamstring = theName;
        }
        qDebug() << "Dataset: " << QString::fromStdString(theName);
        H5::DataSet dumbset = hFilePtr->openDataSet(theName);
        H5::DataSpace dumbspace = dumbset.getSpace();
        int dumbrank = dumbspace.getSimpleExtentNdims();
        if (dumbrank == 3)
        {
            // only requirement is that they have 3 dimensions
            goodSets.push_back(theName);
            items << QString::fromStdString(theName);
        }

        dumbspace.close();
        dumbset.close();
    }
    std::string finalDataset;
    if (goodSets.size() == 0)
    {
        qDebug() << "No valid 3D dataset found";
        //return cv::Mat();
        return false;
    }
    else if (goodSets.size() == 1)
    {
        finalDataset = goodSets[0];
    }
    else
    {
        bool ok;
        QString item = QInputDialog::getItem(NULL, "Which dataset to open",
            "Options:", items, 0, false, &ok);
        if (ok && !item.isEmpty())
        {
            finalDataset = item.toStdString();
            //qDebug()<<"Selected "<<item;
        }
        else
        {
            //return cv::Mat();
            return false;
        }

    }

    // now open the selected one
    _dataset = hFilePtr->openDataSet(finalDataset);
    _dataSpace = _dataset.getSpace();

    int ndims = _dataSpace.getSimpleExtentDims(_dims, NULL);

    //告知总共Frame数目
    emit(totalFrameNum(_dims[0]));

    if (ndims != 3) qDebug() << "Rank is not 3: " << ndims;
    /*qDebug() << "rank " << ndims << ", dimensions " <<
    (unsigned long)(_dims[0]) << " x " <<
    (unsigned long)(_dims[1]) << " x " <<
    (unsigned long)(_dims[2]);*/



    H5T_class_t _dataclass = _dataset.getTypeClass();
    if (_dataclass == H5T_INTEGER)
    {
        bool hasFormat = _dataset.attrExists("PIXFORMAT");
        if (hasFormat)
        {
            H5::Attribute _attr_pix = _dataset.openAttribute("PIXFORMAT");
            std::string strbuf;
            H5::StrType _strdatatype(H5::PredType::C_S1, 10);
            _attr_pix.read(_strdatatype, strbuf);
            _dataFormat = QString::fromStdString(strbuf);
        }
        //            qDebug()<<"Attribute contents: "<<dataformat;


        H5::IntType _intype = _dataset.getIntType();
        size_t size = _intype.getSize();


        if (size == 1)
        {
            if (_dataFormat == "RGB8")
            {
                h5_readType = H5::PredType::NATIVE_UCHAR;
                _bufferFrame = cv::Mat(_dims[1], _dims[2] / 3., CV_8UC3);
                //dataformat="RGB8";
            }
            else if (_dataFormat == "BayerGB")
            {
                h5_readType = H5::PredType::NATIVE_UCHAR;
                _bufferFrame = cv::Mat(_dims[1], _dims[2], CV_8U);
                if (!hasFormat)
                {
                    _dataFormat = "BayerGB";
                }
            }
            else
            {
                h5_readType = H5::PredType::NATIVE_UCHAR;
                _bufferFrame = cv::Mat(_dims[1], _dims[2] / 4., CV_8UC4);
            }
        }
        else if (size == 2)
        {
            h5_readType = H5::PredType::NATIVE_UINT16;
            _bufferFrame = cv::Mat(_dims[1], _dims[2], CV_16U);
            if (!hasFormat)
            {
                _dataFormat = "MONO14"; // not knowing if it is 12 or 14, 14 is the safer choice for displaying will check attribute later.
            }
        }
        else
        {
            qDebug() << "Integer size not yet handled: " << size;
            return false;
        }

    }
    else if (_dataclass == H5T_FLOAT)
    {
        _dataFormat = "FLOAT";
        h5_readType = H5::PredType::NATIVE_FLOAT;
        _bufferFrame = cv::Mat(_dims[1], _dims[2], CV_32F);
    }
    else if (_dataclass == H5T_COMPOUND)
    {
        //typically a complex number => no meaningfull way to show this so exit
        qDebug() << "Data set has compound type - will exit";
        return false;
    }
    else
    {
        qDebug() << "Data set has unknown type - will exit";
        return false;
    }

    if (timepresent)
    {
        timestamps.clear();
        H5::DataSet timeset = hFilePtr->openDataSet(timestring);
        H5::DataSpace timespace = timeset.getSpace();
        hsize_t timedim[1];
        int ndims = timespace.getSimpleExtentDims(timedim, NULL);
        if (ndims != 1) qDebug() << "Rank is not 1: " << ndims;

        cv::Mat timeMat = cv::Mat(1, timedim[0], CV_64F);
        timeset.read(timeMat.data, H5::PredType::NATIVE_DOUBLE);

        for (uint i = 0; i < timedim[0]; i++) {
            timestamps.push_back(timeMat.at<double>(0, i));
        }
    }
    if (stiparamspresent)
    {
        stiParams.clear();
        H5::DataSet stiparamset = hFilePtr->openDataSet(stiparamstring);
        H5::DataSpace stiparamspace = stiparamset.getSpace();
        hsize_t stiparamdim[1];
        int ndims = stiparamspace.getSimpleExtentDims(stiparamdim, NULL);
        if (ndims != 1) qDebug() << "Rank is not 1: " << ndims;

        cv::Mat stiparamMat = cv::Mat(1, stiparamdim[0], CV_64F);
        stiparamset.read(stiparamMat.data, H5::PredType::NATIVE_INT32);

        if (stiparamdim[0] % 5 != 0)
        {
            qDebug() << "Error: the stimulus params number is wrong";
            return false;
        }
        else
        {
            for (uint i = 0; i < stiparamdim[0]; i+=5)
            {
                ParaConfig tmp;
                tmp.dutyCycle = stiparamMat.at<int>(0, i);
                tmp.frequency = stiparamMat.at<int>(0, i + 1);
                tmp.periodCount = stiparamMat.at<int>(0, i + 2);
                tmp.stimulusCount = stiparamMat.at<int>(0, i + 3);
                tmp.direction = stiparamMat.at<int>(0, i + 4);
                stiParams.push_back(tmp);
            }
        }
    }
    return true;
}

bool HDF5convertor::closeH5File()
{
    try {
        hFilePtr->close();
        delete hFilePtr;
    } catch (Exception e) {
        qDebug()<<"Error occured: "<<e.getCDetailMsg();
        return false;
    }

    return true;
}

bool HDF5convertor::convertFile()
{
    //init stimulus text
    QString _stiParamsModel = "DutyCycle: %1, Frequency: %2, PeriodCount: %3, StiCount: %4, Direction: %5";
    int fontFace = cv::FONT_HERSHEY_COMPLEX_SMALL;
    double fontScale = 1.8;
    int thickness = 1;
    int baseline = 0;
    cv::Size textSize = cv::getTextSize(_stiParamsModel.toStdString(), fontFace, fontScale, thickness, &baseline);
    int margin = textSize.height * 2;
    //int margin = 0;
    baseline += thickness;

    cv::Size _outputSize(_bufferFrame.cols, _bufferFrame.rows + margin);
    // center the text in the margin
    cv::Point textOrg((_outputSize.width - textSize.width) / 2, _bufferFrame.rows + (margin + textSize.height) / 2 );
    cv::Point circleOrg(margin, _bufferFrame.rows + margin / 2);

    if (_type == "video")
    {
        QString _outputFileName = _dataset_name.mid(0, _dataset_name.lastIndexOf("."));
        _outputFileName = _outputFileName.mid(_outputFileName.lastIndexOf("/") + 1);

        _outputFilePath = _storeDir +  "/" + _outputFileName + "_processedTime_" 
            + _currentTime.toString("yyyy.MM.dd_hh-mm") + "." + _filetype;

        if (_isLog)
        {
            QString logFilename = _outputFilePath.mid(0, _outputFilePath.lastIndexOf("."));
            logFilename = logFilename + "_logFile.txt";
            _logFilePtr = new QFile(logFilename);
        }

        //_codec = CV_FOURCC('X', '2', '6', '4');
        //_codec = CV_FOURCC('D', 'I', 'V', 'X');
        _codec = CV_FOURCC('X', 'V', 'I', 'D');
        //_codec = CV_FOURCC('M', 'J', 'P', 'G');
        //_codec = CV_FOURCC('P', 'I', 'M', '1');

        _fps = 20.0;

        _writerPtr = new cv::VideoWriter(_outputFilePath.toStdString(), _codec, _fps, _outputSize);

        if (!_writerPtr->isOpened())
        {
            qDebug() << "VideoWriter is not ready";
            return false;
        }

        for (hsize_t frameIdx = 0; frameIdx < _dims[0]; frameIdx++)
        {
            if (_dataFormat == "BayerGB")
            {
                hsize_t dimsSlab[3] = { 1, _dims[1], _dims[2] };
                hsize_t offset[3] = { frameIdx,0,0 };
                _dataSpace.selectHyperslab(H5S_SELECT_SET, dimsSlab, offset);
            }

            hsize_t readDims[2] = { _dims[1],_dims[2] };
            H5::DataSpace memspace(2, readDims);

            _dataset.read(_bufferFrame.data, h5_readType, memspace, _dataSpace);
            _dataSpace.selectNone();

            cv::Mat _outputFrame = cv::Mat::zeros(_outputSize, _bufferFrame.type());
            
            //copy _bufferFrame into the correct place of _outputFrame
            _bufferFrame.copyTo(_outputFrame(cv::Rect(0, 0, _bufferFrame.cols, _bufferFrame.rows)));

            //convert color type from bayerGB to RGB
            cv::cvtColor(_outputFrame, _outputFrame, CV_BayerGB2RGB);

            //TODO check the dim is match
            //read Sti params and putText
            ParaConfig tmp = stiParams[frameIdx];
            QString _stiParamsText = _stiParamsModel.arg(tmp.dutyCycle).arg(tmp.frequency).arg(tmp.periodCount).arg(tmp.stimulusCount).arg(tmp.direction);
            cv::putText(_outputFrame, _stiParamsText.toStdString(), textOrg, fontFace, fontScale, cv::Scalar::all(255), thickness, CV_AA);
            
            //when the periodCount is larger than 0, it means the frame is stimulated
            if (tmp.periodCount > 0 || tmp.frequency > 0 || tmp.dutyCycle > 0)
            {
                cv::circle(_outputFrame, circleOrg, textSize.height / 2, cv::Scalar(0, 0, 255), -1);

                if( _isLog)
                {
                    if (_logFilePtr->open(QFile::Append | QFile::Text))
                    {
                        int sec = frameIdx / _fps;
                        int mius = sec / 60;
                        sec = sec % 60;

                        QTextStream out(_logFilePtr);
                        out.setFieldWidth(2);
                        out.setFieldAlignment(QTextStream::AlignLeft);
                        out.setPadChar(' ');
                        out << " Video Time(sec): " << mius << ":" << sec << " " << _stiParamsText << "\r\n";
                        _logFilePtr->close();
                    }
                }
            }
            //write the frame
            _writerPtr->write(_outputFrame);

            emit(finishedFrameCount(frameIdx + 1));
        }
    }
    else if (_type == "image")
    {
        for (hsize_t frameIdx = 0; frameIdx < _dims[0]; frameIdx++)
        {
            if (_dataFormat == "BayerGB")
            {
                hsize_t dimsSlab[3] = { 1, _dims[1], _dims[2] };
                hsize_t offset[3] = { frameIdx,0,0 };
                _dataSpace.selectHyperslab(H5S_SELECT_SET, dimsSlab, offset);
            }

            hsize_t readDims[2] = { _dims[1],_dims[2] };
            H5::DataSpace memspace(2, readDims);

            _dataset.read(_bufferFrame.data, h5_readType, memspace, _dataSpace);
            _dataSpace.selectNone();
            cv::Mat _outputFrame = cv::Mat::zeros(_outputSize, _bufferFrame.type());
            //copy _bufferFrame into the correct place of _outputFrame
            _bufferFrame.copyTo(_outputFrame(cv::Rect(0, 0, _bufferFrame.cols, _bufferFrame.rows)));

            //convert color type from bayerGB to RGB
            cv::cvtColor(_outputFrame, _outputFrame, CV_BayerGB2RGB);

            //read Sti params and putText
            ParaConfig tmp = stiParams[frameIdx];
            QString _stiParamsText = _stiParamsModel.arg(tmp.dutyCycle).arg(tmp.frequency).arg(tmp.periodCount).arg(tmp.stimulusCount).arg(tmp.direction);
            cv::putText(_outputFrame, _stiParamsText.toStdString(), textOrg, fontFace, fontScale, cv::Scalar::all(255), thickness, CV_AA);

            //when the periodCount is larger than 0, it means the frame is stimulated
            if (tmp.periodCount > 0 || tmp.frequency > 0 || tmp.dutyCycle > 0)
            {
                cv::circle(_outputFrame, circleOrg, textSize.height / 2, cv::Scalar(0, 0, 255), -1);
            }

            //get each frame output path
            _outputFilePath = _storeDir +"/" + QString::number(frameIdx) + "." + _filetype;

            //write the image
            cv::imwrite(_outputFilePath.toStdString(), _outputFrame);

            emit(finishedFrameCount(frameIdx + 1));
        }
    }
    else
    {
        qDebug() << "The type is invalid.";
        _writerPtr->release();
        return false;
    }

    _writerPtr->release();
    return true;
}


bool HDF5convertor::cvtH5()
{
    if(!readH5File())
    {
        return false;
    }

    if(!convertFile())
    {
        return false;
    }

    if(!closeH5File())
    {
        return false;
    }

    emit(finished());
    return true;
}
