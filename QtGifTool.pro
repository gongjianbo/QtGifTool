QT += core gui widgets concurrent

CONFIG += c++11 utf8_source

DESTDIR = $$PWD/bin

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

INCLUDEPATH += $$PWD/GifLib_VS2019x64/include
LIBS += $$PWD/GifLib_VS2019x64/lib/GifLib.lib

#INCLUDEPATH += "E:/Program Files (x86)/Visual Leak Detector/include"
#DEPENDPATH += "E:/Program Files (x86)/Visual Leak Detector/include"
#LIBS += "E:/Program Files (x86)/Visual Leak Detector/lib/Win64/vld.lib"
