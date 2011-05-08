%option noyywrap

%{
#include "common.h"
#include "parser.tab.h"

YY_BUFFER_STATE buf;

void scan_string(char *s) {
  buf = yy_scan_string(s);
  yy_switch_to_buffer(buf);
}

void free_scan_string() {
  yy_delete_buffer(buf);
}
       
%}

extern YYSTYPE yylval;
ALNUM  [^\n -.,;"'\\<>()*]
ALPHA  [^\n -.,;"'\\0-9<>()*]
IDLIST 1[sS]am|2[sS]am|1[kK]r|2[kK]r|1[kK]rn|2[kK]rn|1[kK]or|2[kK]or|1[sS]ol|2[sS]ol|1[tT]im|2[tT]im|1[pP]t|2[pP]t|1[jJ]n|2[jJ]n|3[jJ]n|1[mM]ach|2[mM]ach|[a-zA-Z]{ALPHA}+

%%
\         // skip spaces
\([^)]*\)  // skip parentheses
\/al\.[^/]*\/  return OR;
\/kr\.[^/]*\/  return OR;
\/[^/]*\/  // skip comments // strncpy(yylval.id, yytext, sizeof(yylval.id)); return COMMENT;
\<[^>]*\>    strncpy(yylval.id, yytext, sizeof(yylval.id)); return REGEXP;
{IDLIST}  strncpy(yylval.id, yytext, sizeof(yylval.id)); return ID;
[0-9]+    sscanf(yytext, "%d", &yylval.num); return NUM;
\-        return DASH;
\xe2\x80\x93  return DASH;
\+        return DASH;
\.        return DOT;
\,        return COMMA;
\;        return SEMICOLON;
\n        //
{ALPHA}+    strncpy(yylval.id, yytext, sizeof(yylval.id)); return ID;
.         yyerror("cannot scan!");

%%
