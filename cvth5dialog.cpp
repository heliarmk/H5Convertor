#include "cvth5dialog.h"
#include "ui_cvth5dialog.h"

cvtH5Dialog::cvtH5Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::cvtH5Dialog)
{
    ui->setupUi(this);

    _fileType = ui->imageTypeComboBox->currentText();
    _type = "image";
    _isLog = false;
    ui->progressBar->setValue(0);
    setWindowIcon(QIcon("logo.ico"));
}

cvtH5Dialog::~cvtH5Dialog()
{
    delete ui;
}

void cvtH5Dialog::closeEvent(QCloseEvent *event)
{

    
    QDialog::closeEvent(event);
}

void cvtH5Dialog::on_h5FilePathToolButton_clicked()
{
    _datasetPath = QFileDialog::getOpenFileName(NULL, QString::fromLocal8Bit("选择H5文件"), "", QString::fromLocal8Bit("HDF5 File (*.h5)"));
    ui->h5FilePathLineEdit->setText(_datasetPath);
}

void cvtH5Dialog::on_outputDirToolButton_clicked()
{
    QFileInfo fileInfo(_datasetPath);
    _outputDir = QFileDialog::getExistingDirectory(NULL,QString::fromLocal8Bit("选择储存图片路径"), fileInfo.path());
    ui->outputDirLineEdit->setText(_outputDir);
}

void cvtH5Dialog::on_imageTypeComboBox_currentIndexChanged(const QString &arg1)
{
    _fileType = arg1;
}

void cvtH5Dialog::setProgressBarRange(qulonglong max)
{
    ui->progressBar->setRange(0, max);
}

void cvtH5Dialog::updateProgressBar(qulonglong value)
{
    ui->progressBar->setValue(value);
}

void cvtH5Dialog::on_imgRadioButton_clicked()
{
    ui->imageTypeComboBox->setEnabled(true);
    ui->isLogCheckBox->setEnabled(false);
    _type="image";

}

void cvtH5Dialog::on_videoRadioButton_clicked()
{
    ui->imageTypeComboBox->setEnabled(false);
    ui->isLogCheckBox->setEnabled(true);
    _type="video";
    _fileType = "avi";
}

void cvtH5Dialog::on_beginButton_clicked()
{

    if(ui->isLogCheckBox->isChecked())
    {
        _isLog = true;
    }

    _converterPtr = new HDF5convertor(NULL,_datasetPath,_outputDir,_type,_fileType, _isLog);
    _cvtThreadPtr = new QThread();

    connect(_converterPtr, SIGNAL(totalFrameNum(qulonglong)), this, SLOT(setProgressBarRange(qulonglong)));
    connect(_converterPtr, SIGNAL(finishedFrameCount(qulonglong)), this, SLOT(updateProgressBar(qulonglong)));
    connect(_cvtThreadPtr, SIGNAL(started()), _converterPtr, SLOT(cvtH5()));
    connect(_converterPtr, SIGNAL(finished()), _cvtThreadPtr, SLOT(quit()));
    connect(_converterPtr, SIGNAL(finished()), this, SLOT(reset()));
    connect(_cvtThreadPtr, SIGNAL(finished()), _cvtThreadPtr, SLOT(deleteLater()));

    _converterPtr->moveToThread(_cvtThreadPtr);

    _cvtThreadPtr->start();

    ui->beginButton->setEnabled(false);
}

void cvtH5Dialog::reset()
{
    ui->beginButton->setEnabled(true);
    ui->progressBar->reset();
}
