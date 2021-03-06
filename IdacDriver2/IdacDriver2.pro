# -------------------------------------------------
# Project created by QtCreator 2009-04-17T11:33:42
# -------------------------------------------------
QT -= gui
CONFIG += warn_on \
    staticlib \
    create_prl \
    debug_and_release
TEMPLATE = lib
INCLUDEPATH += . \
    .. \
    ../Core \
    ../IdacDriver
HEADERS += IdacDriver2.h \
    IdacDriver2ReqIds.h \
    IdacDriver2Constants.h \
    IdacDriver2Es.h
SOURCES += IdacDriver2.cpp \
	IdacDriver2Firmware.cpp \
	IdacDriver2Es.cpp
win32:INCLUDEPATH += ../extern/win32
unix:INCLUDEPATH += ../extern/libusb/include

#CONFIG(debug, debug|release):DESTDIR = ../debug
#else:DESTDIR = ../release
