QT +=  core gui qml quick quickcontrols2

CONFIG += c++17
CONFIG += qtquickcompiler
CONFIG += qmltypes

QML_IMPORT_NAME = TIR
QML_IMPORT_MAJOR_VERSION = 1

include(Network/Network.pri)
include(Models/Models.pri)

SOURCES += \
        main.cpp \
        tirbackend.cpp

RESOURCES += $$PWD/QML/qml.qrc \
    resources/resources.qrc

unix|win32: LIBS += -lpcap

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    Common.h \
    tirbackend.h
