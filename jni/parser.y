%{
#include "common.h"
#include <stdio.h>
#include <string.h>

//#define YYDEBUG 1

#define NEW(new, type, next) { new.type = (struct type *)malloc(sizeof(struct type)); new.type->n = next; new.type->cnt = 1; }
#define CPY(dst, src) { dst = src; dst->cnt++; }

void free_casti(struct casti *c) {
  c->cnt--;
  if (!c->cnt) {
    if (c->n) free_casti(c->n);
    free(c);
  }
}

void free_varianty(struct varianty *c) {
  c->cnt--;
  if (!c->cnt) {
    if (c->l) free_casti(c->l);
    if (c->n) free_varianty(c->n);
    free(c);
  }
}

void free_citania(struct citania *c) {
  c->cnt--;
  if (!c->cnt) {
    if (c->l) free_varianty(c->l);
    if (c->n) free_citania(c->n);
    free(c);
  }
}

static YYSTYPE citaniaMerge(YYSTYPE x0, YYSTYPE x1) { x0.citania->cnt++; return x0; };

%}

%token OR COMMENT REGEXP ID NUM DASH DOT COMMA SEMICOLON ERROR DESC
%token END 0
%glr-parser
%destructor { free($$.casti); } suradnice casti;
%destructor { free($$.varianty); } varianta citanie;
%destructor { free($$.citania); } citania;
%destructor { free($$.id); } REGEXP ID DESC;

%%

all: citania { Process($1.citania); }

citania:  citanie citania { NEW($$, citania, $2.citania); CPY($$.citania->l, $1.varianty); } %dprec 3 %merge <citaniaMerge>
    | citanie             { NEW($$, citania, NULL); CPY($$.citania->l, $1.varianty); } %dprec 2
;

citanie: varianta OR citanie   { CPY($$.varianty, $1.varianty); CPY($$.varianty->n, $3.varianty); }
    | varianta oddelovac       { CPY($$.varianty, $1.varianty); }
    | error oddelovac          { NEW($$, varianty, NULL); $$.varianty->l = NULL; }
;

oddelovac: SEMICOLON | END
;

varianta: ID casti        { NEW($$, varianty, NULL); CPY($$.varianty->l, $2.casti); strncpy($$.varianty->l->k, $1.id, sizeof($$.varianty->l->k)); }
;

casti: suradnice DOT casti      { CPY($$.casti, $1.casti); CPY($$.casti->n, $3.casti); $$.casti->range = 0; }
    | suradnice DASH casti      { CPY($$.casti, $1.casti); CPY($$.casti->n, $3.casti); $$.casti->range = 1; }
    | suradnice SEMICOLON casti { CPY($$.casti, $1.casti); CPY($$.casti->n, $3.casti); $$.casti->range = 0; }
    | suradnice                 { CPY($$.casti, $1.casti); $$.casti->range = 0; }
;

suradnice: hlava COMMA vers   { NEW($$, casti, NULL); $$.casti->k[0] = 0; $$.casti->h = $1.num; $$.casti->v = $3.num; }
    | vers                    { NEW($$, casti, NULL); $$.casti->k[0] = 0; $$.casti->h = -1; $$.casti->v = $1.num; }
;

hlava:  NUM          { $$.num = $1.num; }
;

vers: NUM castversa  { $$.num = $1.num; }
    | castversa  { }
;

castversa: | ID | REGEXP | ID REGEXP
;

%%

void yyerror(const char *s) {
//  printf("error: %s\n", s);
  __android_log_print(ANDROID_LOG_INFO, "svpismo", "error %s\n", s);
//  exit(1);
}

