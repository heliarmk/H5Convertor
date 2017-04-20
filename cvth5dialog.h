#ifndef cvtH5DialOG_H
#define cvtH5DialOG_H

#include <QDialog>
#include <QString>
#include <QMessageBox>
#include <QThread>

#include "hdf5cvt.h"

namespace Ui {
class cvtH5Dialog;
}

class cvtH5Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit cvtH5Dialog(QWidget *parent = 0);
    ~cvtH5Dialog();
    
protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void setProgressBarRange(qulonglong max);
    void updateProgressBar(qulonglong value);
    void reset();

    void on_h5FilePathToolButton_clicked();

    void on_outputDirToolButton_clicked();

    void on_beginButton_clicked();

    void on_imageTypeComboBox_currentIndexChanged(const QString &arg1);

    void on_imgRadioButton_clicked();

    void on_videoRadioButton_clicked();

private:
    QString _datasetPath;
    QString _outputDir;
    QString _outputFileName;
    QString _fileType;
    QString _type;
    bool _isLog;

    HDF5convertor *_converterPtr;
    QThread *_cvtThreadPtr;

    Ui::cvtH5Dialog *ui;
};

#endif // cvtH5DialOG_H
