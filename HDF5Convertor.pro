#-------------------------------------------------
#
# Project created by QtCreator 2017-04-05T14:27:00
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = HDF5Convertor
TEMPLATE = app


SOURCES += main.cpp\
        cvth5dialog.cpp \
        hdf5cvt.cpp

HEADERS  += cvth5dialog.h \
        hdf5cvt.h

FORMS    += cvth5dialog.ui

#Opencv lib path
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../dev/opencv/opencv3.2/build/x64/vc14/lib/ -lopencv_world320
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../dev/opencv/opencv3.2/build/x64/vc14/lib/ -lopencv_world320d

INCLUDEPATH += $$PWD/../../../../dev/opencv/opencv3.2/build/include
DEPENDPATH += $$PWD/../../../../dev/opencv/opencv3.2/build/include

#hdf5 lib path
win32: LIBS += -L$$PWD/../../../../dev/hdf5/1.10.0-patch1/lib/ -lhdf5 \
                                                               -lhdf5_cpp \
                                                               -lszip \
                                                               -lzlib
INCLUDEPATH += $$PWD/../../../../dev/hdf5/1.10.0-patch1/include
DEPENDPATH += $$PWD/../../../../dev/hdf5/1.10.0-patch1/include
