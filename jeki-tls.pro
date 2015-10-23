QT += core network
QT -= gui

TARGET = jeki-tls
CONFIG += console
CONFIG += c++11
CONFIG -= app_bundle

# Set this to your botan lib location
BOTAN_LIB = /home/diorahman/Experiments/jeki/deps/botan/current/linux
INCLUDEPATH += $${BOTAN_LIB}/include/botan-1.11
LIBS += -L$${BOTAN_LIB}/lib -lbotan-1.11

TEMPLATE = app

SOURCES += main.cpp \
    jekiqsocket.cpp

HEADERS += \
    jekiqsocket.h \
    credentials.h

