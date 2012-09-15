#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef NOANDROID
#include <android/log.h>
#endif

extern void yyerror(const char *);
extern int yylex();

struct citania {
  int cnt;
  char *tag;
  struct varianty *l;
  struct citania *n;
};

struct varianty {
  int cnt;
  char *tag;
  struct casti *l;
  struct varianty *n;
};

struct casti {
  int cnt;
  char k[20];
  int h, v;
  char range;
  struct casti *n;
};

union node_type {
  struct citania *citania;
  struct varianty *varianty;
  struct casti *casti;
  char* id;
  int num;
};

struct strbuf {
  char *buf;
  int alen, i;
};

#define YYSTYPE union node_type

void Process(struct citania *l);
void scan_string(char *s);
void free_scan_string();
int yyparse(void);

void InitBuf(struct strbuf *q);
void FreeBuf(struct strbuf *q);
void Rst(struct strbuf *q); 
void Prn(struct strbuf *q, char *fmt, ...); 
void Cpy(struct strbuf *q, const void *buf, int len); 

extern struct AAssetManager *amgr;
