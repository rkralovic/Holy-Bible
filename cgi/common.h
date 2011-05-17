#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern void yyerror(const char *);
extern int yylex();

struct citania {
  struct varianty *l;
  struct citania *n;
};

struct varianty {
  struct casti *l;
  struct varianty *n;
};

struct casti {
  char k[20];
  int h, v;
  char range;
  struct casti *n;
};

union node_type {
  struct citania *citania;
  struct varianty *varianty;
  struct casti *casti;
  char id[20];
  int num;
};

struct strbuf {
  char buf[65536];
  int i;
};

#define YYSTYPE union node_type

void Process(struct citania *l);
void scan_string(char *s);
void free_scan_string();
int yyparse(void);


void Rst(struct strbuf *q); 
void Prn(struct strbuf *q, char *fmt, ...); 
