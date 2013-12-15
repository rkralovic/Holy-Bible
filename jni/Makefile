CFLAGS+=-O0 -ggdb -DNOANDROID
LDFLAGS+=-O0 -ggdb -DNOANDROID

all: pismo pismo.cgi db_bin_gen pismo.mobi

pismo.cgi: pismo.o parser.tab.o scanner.o common.o db_mysql.o
	$(CC) $(LDFLAGS) $^ -o $@ -lmysqlclient

pismo: pismo.o parser.tab.o scanner.o common.o db_bin.o
	$(CC) $(LDFLAGS) $^ -o $@

db_bin_gen: db_bin_gen.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ -lmysqlclient

db_bin_to_ebook: db_bin_to_ebook.o db_bin.o parser.tab.o scanner.o common.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ -lmysqlclient

pismo.mobi: db_bin_to_ebook ebook.css
	./db_bin_to_ebook >/tmp/pismo.html
	ebook-convert /tmp/pismo.html ./pismo.mobi \
          --max-toc-links 500 \
          --level1-toc '//*[@class="kniha"]|//h:h1' \
          --level2-toc '//*[@class="kap"]' \
          --extra-css ./ebook.css

%.o: %.c
	$(CC) -Wall -W -c -I$(OPIEDIR)/usr/include/mysql $(CFLAGS) $<

pismo.c: parser.tab.h

scanner.c: scanner.l parser.tab.c parser.tab.h
	flex -8 -s -o $@ $<

parser.tab.c: parser.tab.h

parser.tab.h: parser.y
	bison -v -d $<

clean:
	rm -f parser.tab.* scanner.c parser *.o parser.output pismo db_bin_gen pismo.cgi

