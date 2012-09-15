CFLAGS+=-O0 -ggdb -DNOANDROID
LDFLAGS+=-O0 -ggdb -DNOANDROID

#all: pismo db_bin_gen
all: pismo

#pismo: pismo.o parser.tab.o scanner.o common.o db_mysql.o
#	$(CC) $(LDFLAGS) $^ -o $@ -lmysqlclient

pismo: pismo.o parser.tab.o scanner.o common.o db_bin.o
	$(CC) $(LDFLAGS) $^ -o $@

db_bin_gen: db_bin_gen.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ -lmysqlclient

%.o: %.c
	$(CC) -Wall -W -c -I$(OPIEDIR)/usr/include/mysql $(CFLAGS) $<

pismo.c: parser.tab.h

scanner.c: scanner.l parser.tab.c parser.tab.h
	flex -8 -s -o $@ $<

parser.tab.c: parser.tab.h

parser.tab.h: parser.y
	bison -v -d $<

clean:
	rm -f parser.tab.* scanner.c parser *.o parser.output pismo db_bin_gen

