TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        fparser.c \
        fparser_platform.c \
        main.c

HEADERS += \
    fparser.h \
    fparser_platform.h

DISTFILES += \
    file_format_define.txt \
    lrc.txt \
