QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets multimedia serialport

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
  #  Audio.cpp \
  #  Audioinput.cpp \
    audioengine.cpp \
    httpmanager.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
  #  Audio.h \
  #  Audioinput.h \
    audioengine.h \
    cusdr_queue.h \
    httpmanager.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

TRANSLATIONS += \
    HAMster_en_US.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/..//release/ -ldigihamlib
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD//debug/ -ldigihamlib
else:unix: LIBS += -L$$PWD/../build-digihamlib-Qt6_6_1-Debug/ -ldigihamlib

INCLUDEPATH += $$PWD/../build-digihamlib-Qt6_6_1-Debug
DEPENDPATH += $$PWD/../build-digihamlib-Qt6_6_1-Debug
