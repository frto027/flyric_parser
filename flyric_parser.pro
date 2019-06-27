TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        fparser.c \
        fparser_platform.c \
        frp_bison.tab.c \
        lex.frp_bison.c \
        test.c

HEADERS += \
    fparser.h \
    fparser_platform.h \
    frp_bison.tab.h \
    fparser_public.h

DISTFILES += \
    file_format_define.txt \
    frp_bison.y \
    frp_flex.l \
    lrc.txt \
