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

TRANSLATIONS += \
    i18n/tr_en.ts \
    i18n/tr_ru.ts \
    i18n/tr_de.ts

RESOURCES += \
    i18n/lang.qrc

QMAKE_CXXFLAGS += -Ofast -Wall -Wpedantic

DEFINES *= \
    QT_USE_QSTRINGBUILDER
