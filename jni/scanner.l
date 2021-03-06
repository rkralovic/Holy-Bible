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

void despace(char *s) {
  char *i;
  for (i=s; *i; i++) {
    *s = *i;
    if ((unsigned char)(*s) > ' ') s++;
  }
  *s = *i;
}

char* duptag(char *s) {
  char *out;
  if (s[0] == ':' || s[0] == '{') s++;
  if (s[0] == '-') s++;
  if (s[0] == '-') s++;
  out = strdup(s);
  int l = strlen(out);
  if (l > 0 && out[l-1] == '\n') l--;
  if (l > 0 && out[l-1] == '\r') l--;
  if (l > 0 && out[l-1] == '}') l--;
  out[l] = 0;
  return out;
}
       
%}

extern YYSTYPE yylval;
ALNUM  [^\r\n -.,:;"'\\<>()*]
ALPHA  [^\r\n -.,:;"'\\0-9<>()*]
IDLIST 1\ *[sS]am|2\ *[sS]am|1\ *[kK]r|2\ *[kK]r|1\ *[kK]rn|2\ *[kK]rn|1\ *[kK]or|2\ *[kK]or|1\ *[sS]ol|2\ *[sS]ol|1\ *[tT]im|2\ *[tT]im|1\ *[pP]t|2\ *[pP]t|1\ *[jJ]n|2\ *[jJ]n|3\ *[jJ]n|1\ *[mM]ach|2\ *[mM]ach|[a-zA-Z]{ALPHA}+

%%
\([^)]*\)  // skip parentheses
\/al\.[^/]*\/  return OR;
\/kr\.[^/]*\/  return OR;
\|        return OR;
\:[^\r\n;]*{ALPHA}[^\r\n;]*\r?\n?      yylval.id = duptag(yytext); return DESC;
\:        return COMMA;  // V anglictine sa pouziva : namiesto ciarky, ludia na to mozu byt zvyknuti
\{-[^}]*\}\r?\n?     yylval.id = duptag(yytext); return TAG_MINOR;
\{[^-][^}]*\}\r?\n?  yylval.id = duptag(yytext); return TAG_MAJOR;
\/[^/]*\/  // skip comments // strncpy(yylval.id, yytext, sizeof(yylval.id)); return COMMENT;
\<[^>]*\>    yylval.id = duptag(yytext); return REGEXP;
{IDLIST}  yylval.id = duptag(yytext); despace(yylval.id); return ID;
[0-9]+    sscanf(yytext, "%d", &yylval.num); return NUM;
\-        return DASH;
\xe2\x80\x93  return DASH;
\+        return DOT;
\.        return DOT;
\,        return COMMA;
\;        return SEMICOLON;
\r        return SEMICOLON;
\n        // skip whitespace
\         // skip spaces
{ALPHA}+  yylval.id = duptag(yytext); despace(yylval.id); return ID;
.         // silently ignore unknown characters (maybe strange whitespace?) // yyerror("cannot scan!"); return ERROR;

%%
