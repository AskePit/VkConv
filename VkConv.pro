QT += core network gui widgets

CONFIG += c++11

TARGET = VkConv
#CONFIG += console
#CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    downloader.cpp \
    vkconvwizard.cpp

HEADERS += \
    downloader.h \
    vkconvwizard.h

FORMS +=

#QMAKE_CXXFLAGS += -O2 -Wall
#QMAKE_LFLAGS_RELEASE += -static -static-libgcc
