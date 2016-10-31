QT += core network gui widgets

CONFIG += c++11

TARGET = VkConv

TEMPLATE = app

SOURCES += main.cpp \
    downloader.cpp \
    vkconvwizard.cpp

HEADERS += \
    downloader.h \
    vkconvwizard.h

FORMS +=
