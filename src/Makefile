
CC=cc
AR=ar

PROJECT_DIR=./..
OUTPUT_DIR=$(PROJECT_DIR)/obj
INCLUDE_DIR=$(PROJECT_DIR)/include

all:$(OUTPUT_DIR)/libfparser.a $(INCLUDE_DIR)/fparser_public.h

$(OUTPUT_DIR)/libfparser.a:$(OUTPUT_DIR)/fparser.o $(OUTPUT_DIR)/fparser_platform.o $(OUTPUT_DIR)/frp_bison.tab.o $(OUTPUT_DIR)/lex.frp_bison.o
	ar -r $@ $+

$(INCLUDE_DIR)/fparser_public.h:fparser_platform.h fparser_public.h
	cp $+ $(INCLUDE_DIR)/


$(OUTPUT_DIR)/fparser.o:fparser.c fparser.h fparser_platform.h fparser_public.h
	cc -o $@ -c fparser.c
$(OUTPUT_DIR)/fparser_platform.o:fparser_platform.c fparser.h fparser_platform.h fparser_public.h
	cc -o $@ -c fparser_platform.c
$(OUTPUT_DIR)/frp_bison.tab.o:frp_bison.tab.c fparser.h fparser_platform.h fparser_public.h
	cc -o $@ -c frp_bison.tab.c
$(OUTPUT_DIR)/lex.frp_bison.o:lex.frp_bison.c fparser.h frp_bison.tab.h
	cc -o $@ -c lex.frp_bison.c
frp_bison.tab.c frp_bison.tab.h:bison_flex/frp_bison.y
	-bison -d --name-prefix=frp_bison $+
lex.frp_bison.c:bison_flex/frp_flex.l
	-flex --prefix=frp_bison $+
fparser.h:fparser_public.h

clean:
	rm -f frp_bison.tab.c frp_bison.tab.h lex.frp_bison.c
