%{
#include "common.h"
#include <stdio.h>
#include <string.h>

//#define YYDEBUG 1

#define NEW(new, type, next) { new.type = (struct type *)malloc(sizeof(struct type)); new.type->n = next; }

%}

%token OR COMMENT REGEXP ID NUM DASH DOT COMMA SEMICOLON

%%

all: citania  { Process($1.citania); }

citania:  citanie SEMICOLON citania { NEW($$, citania, $3.citania); $$.citania->l = $1.varianty; }
    | citanie                       { NEW($$, citania, NULL); $$.citania->l = $1.varianty; }
;

citanie: varianta OR citanie   { $$ = $1; $$.varianty->n = $3.varianty; }
    | varianta                 { $$ = $1; }  
;

varianta: ID casti             { NEW($$, varianty, NULL); $$.varianty->l = $2.casti; strncpy($$.varianty->l->k, $1.id, sizeof($$.varianty->l->k)); }
    |                          { NEW($$, varianty, NULL); $$.varianty->l = NULL; }
;

casti: suradnice DOT casti     { $$ = $1; $$.casti->n = $3.casti; $$.casti->range = 0; }
    | suradnice DASH casti     { $$ = $1; $$.casti->n = $3.casti; $$.casti->range = 1; } 
    | suradnice                { $$ = $1; $$.casti->range = 0; } 
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
  printf("error: %s\n", s);
//  exit(1);
}

