TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        fparser.c \
        fparser_platform.c \
        frp_bison.tab.c \
        lex.frp_bison.c \
        main.c

HEADERS += \
    fparser.h \
    fparser_platform.h \
    frp_bison.tab.h

DISTFILES += \
    file_format_define.txt \
    frp_bison.y \
    frp_flex.l \
    lrc.txt \
