libs += -lm

main:fparser.o fparser_platform.o frp_bison.tab.o lex.frp_bison.o

test.timeline:test.timeline.o fparser.o fparser_platform.o frp_bison.tab.o lex.frp_bison.o
	cc -o $@ $+ $(libs)
test.memcheck:test.memcheck.o fparser.o fparser_platform.o frp_bison.tab.o lex.frp_bison.o
	cc -o $@ $+ $(libs)
test.normal:test.normal.o fparser.o fparser_platform.o frp_bison.tab.o lex.frp_bison.o
	cc -o $@ $+ $(libs)
test.timeline.o:test.c fparser.h
	cc -o $@ -c test.c -DTIMELINE_TEST
test.memcheck.o:test.c fparser.h
	cc -o $@ -c test.c -DMEMORY_LEAK_TEST
test.normal.o:test.c fparser.h
	cc -o $@ -c test.c -DNORMAL_TEST

fparser.o:fparser.c fparser.h fparser_platform.h
	cc -c fparser.c
fparser_platform.o:fparser_platform.c fparser.h fparser_platform.h
	cc -c fparser_platform.c
frp_bison.tab.o:frp_bison.tab.c fparser.h fparser_platform.h
	cc -c frp_bison.tab.c
lex.frp_bison.o:lex.frp_bison.c fparser.h frp_bison.tab.h
	cc -c lex.frp_bison.c
frp_bison.tab.c frp_bison.tab.h:frp_bison.y
	bison -d --name-prefix=frp_bison frp_bison.y
lex.frp_bison.c:frp_flex.l
	flex --prefix=frp_bison frp_flex.l
clean:
	rm -f *.o frp_bison.tab.c frp_bison.tab.h lex.frp_bison.c test.normal test.memcheck test.timeline
