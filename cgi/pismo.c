#include <mysql.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>
#include "common.h"
#include "parser.tab.h"
#include "db.h"

char *zalm;
int kalendar;

struct strbuf kontext;

int kontext_last_hb, kontext_last_he;
struct kap {
  char *k;
  int h;
} first, last;

void Print(struct casti *_c) {
  char *k, *s;
  int vb, ve, hb, he, comment;
  struct casti *c;
  static struct strbuf q;

  Rst(&q);

  //nepekne: spoliehame sa, ze vsetko je z jednej knihy
  if (!_c) return;
  k = check_book(_c->k);
  if (!k) return;

  init_search(k);

  hb=vb=-1;
  he=ve=-1;
  for (c=_c; c!=NULL; c=c->n) {
    if (c->h!=-1) {
      hb = c->h;
      vb=-1;
    }
    if (c->v!=-1) vb = c->v;
    ve = vb;
    he = hb;
    if (c->range && c->n) {
      c=c->n;
      if (c->h!=-1) he = c->h;
      if (c->v!=-1) ve = c->v;
    }

    if (hb==-1) { hb = vb; vb = -1; }
    if (he==-1) { he = ve; ve = -1; }
    // zapiseme do kontextu
    if (first.h==-1) {
      first.h=hb;
      first.k=k;
    }
    if (hb!=kontext_last_hb || he!=kontext_last_he) {
      if (hb!=he) Prn(&kontext, ";%s%d-%d", k, hb,he);
      else Prn(&kontext, ";%s%d", k, hb);
      kontext_last_hb = hb; kontext_last_he = he;
    }

    add_search(hb,vb,he,ve);

    vb = ve;
    hb = he;
  }
  last.h = kontext_last_he;
  last.k = k;

  do_search();

  while (get_result(&comment, &s)) {
    if (!comment) printf("%s\n", s);
    else printf("<span class=\"komentar\">(%s)</span>\n", s);
  }
  free_search();

  // do not free k - it might be used in first/last
}

void Process(struct citania *l) {
  int i;
  for (i=0; l!=NULL; i++, l=l->n) {
    struct varianty *v;

    if (kalendar) {
      if (i%2 && l->n) printf("<div class=\"citanie\">Žalm</div>\n\n");
      else if (l->n) printf("<div class=\"citanie\">%d. čítanie</div>\n\n", i/2 + 1);
      else printf("<div class=\"citanie\">Evanjelium</div>\n\n");

      if (i==1 && l->n)  printf("<p><span class=\"redbold\">R: </span><span class=\"it\">%s</span></p>\n\n", zalm);
    }

    for (v=l->l; v!=NULL; v=v->n) {
/*
      struct casti *c;
      char K[MAXLEN];
      int V,H;

      printf("<div class=\"suradnice\">");
      for (c=v->l; c!=NULL; c=c->n) {
        if (c->k[0]) SSTRC(K, c->k);
        if (c->h!=-1) H = c->h;
        if (c->v!=-1) V = c->v;
        printf("%s %d,%d ", K, H, V);
        if (c->range) printf("- ");
        else if (c->n) printf(". ");
      }
      printf("</div>\n\n");
*/

      Print(v->l);

      if (v->n) printf("<div class=\"varianta\">Alebo:</div>\n\n");
    }
  }
}

char *StringDecode(char *in) {
  int i;
  char *s,*out;

  for (s=in,i=0; *s; s++,i++) {
    if (s[0]=='%') {
      s[1]=toupper(s[1]);
      s[2]=toupper(s[2]);
      if ( !( ( (s[1]>='A')&&(s[1]<='F') ) ||
              ( (s[1]>='0')&&(s[1]<='9') ) ) ) return NULL;
      if ( !( ( (s[2]>='A')&&(s[2]<='F') ) ||
              ( (s[2]>='0')&&(s[2]<='9') ) ) ) return NULL;
      s+=2;
    }
  }
  out=(char *)malloc(i+1);
    
  for (s=in,i=0; *s; s++,i++) {
    if (s[0]=='%') {

      if ( (s[1]>='A')&&(s[1]<='F') ) out[i]=s[1]-'A'+10;
      else out[i]=s[1]-'0';
      out[i]*=16;
      if ( (s[2]>='A')&&(s[2]<='F') ) out[i]+=s[2]-'A'+10;
      else out[i]+=s[2]-'0';
      
      s+=2;
    } else if (s[0]=='+') out[i]=' ';
    else out[i]=s[0];
  }
  out[i]=0;
  return out;
}

char *StringEncode(char *in) {
  static unsigned char tab[17]="0123456789ABCDEF";
  int i;
  unsigned char *s, *out;

  for (s=(unsigned char *)in,i=0; *s; s++) {
    if (
        ( (*s>='a')&&(*s<='z') ) ||
        ( (*s>='A')&&(*s<='Z') ) ||
        ( (*s>='0')&&(*s<='9') )
       ) i+=1;
    else i+=3;
  }
  out=(unsigned char *)malloc(i+1);

  for (s=(unsigned char *)in,i=0; *s; s++) {
    if (
        ( (*s>='a')&&(*s<='z') ) ||
        ( (*s>='A')&&(*s<='Z') ) ||
        ( (*s>='0')&&(*s<='9') )
       ) {
      out[i]=*s;
      i+=1;
    } else {
      out[i]='%';
      out[i+1]=tab[*s / 16];
      out[i+2]=tab[*s % 16];
      i+=3;
    }
  }
  out[i]=0;
  return (char *)out;
}

int main() {
  int d,m,y;
  char query[1024];
  char *env;
  char *coord=NULL;
  char *search=NULL;
  char buf[1024];
  char *tmp;

  time_t t;
  struct tm *tt;

  time(&t);
  tt = localtime(&t);
  d = tt->tm_mday;
  m = tt->tm_mon+1;
  y = tt->tm_year+1900;

  env = getenv("QUERY_STRING");
  if (env) {
    char *s;
    s = strstr(env, "d="); if (s) sscanf(s, "d=%d", &d);
    s = strstr(env, "m="); if (s) sscanf(s, "m=%d", &m);
    s = strstr(env, "y="); if (s) sscanf(s, "y=%d", &y);
    s = strstr(env, "c="); 
    if (s) s+=2; else { // spatna kompatibilita so znackami
      s = strstr(env, "in=");
      if (s) s+=3;
    }
    if (s) {
      query[0]=0;
      sscanf(s, "%1000[^&]", query);
      coord=StringDecode(query);
    }
    s = strstr(env, "search="); 
    if (s) {
      query[0]=0;
      sscanf(s+7, "%1000[^&]", query);
      search = StringDecode(query);
    }
  }

  if (db_init()<0) return 1;

  printf("Content-Type: text/html; charset=UTF-8\n\n");

  printf("<html><head>\n"
      "<link rel=\"stylesheet\" href=\"breviar.css\">\n");

  Rst(&kontext);
  kontext_last_hb=kontext_last_he=-1;
  first.h=last.h=-1;

  if (coord) {
    printf("<title>%s</title>\n"
      "</head><body>\n"
      "<div class=\"nadpis\">%s</div>\n\n", coord, coord);
    kalendar = 0;
    scan_string(coord);
    yyparse();
    printf("<p>\n"
        "<form action=\"pismo.cgi\" method=\"get\">\n"
        "<input type=\"text\" name=\"c\">\n"
        "<input type=\"submit\" value=\"Zobraz\">\n"
        "</form>\n");
    {
      char *tmp = StringEncode(kontext.buf+1);
      printf(// "<p>\n"
          "<a href=\"pismo.cgi?c=%s\">Kontext</a>", tmp);
      free(tmp);
    }
    printf("&nbsp; &nbsp;");
    if (first.h!=-1) {
      char *b;
      int h;
      get_prev(first.k, first.h, &b, &h);
      if (b!=NULL) {
        snprintf(buf, sizeof(buf), "%s %d", b, h);
        tmp = StringEncode(buf);
        printf(// "<p>\n"
            "<a href=\"pismo.cgi?c=%s\">Dozadu (%s %d)</a>", tmp, b, h);
        free(tmp);
        free(b);
      }
    }
    printf("&nbsp; &nbsp;");
    if (last.h!=-1){
      char *b;
      int h;
      get_next(last.k, last.h, &b, &h);
      if (b!=NULL) {
        snprintf(buf, sizeof(buf), "%s %d", b, h);
        tmp = StringEncode(buf);
        printf(// "<p>\n"
            "<a href=\"pismo.cgi?c=%s\">Dopredu (%s %d)</a>", tmp, b, h);
        free(tmp);
        free(b);
      }
    }
    printf("<p>\n"
        "<form action=\"pismo.cgi\" method=\"get\">\n"
        "<input type=\"text\" name=\"search\">\n"
        "<input type=\"submit\" value=\"Hľadaj\">\n"
        "</form>\n");
  } else if (search) {
    char *b,*t;
    int h,v;

    printf("<title>Vyhľadávanie \"%s\"</title>\n"
        "</head><body>\n"
        "<div class=\"nadpis\">Vyhľadávanie \"%s\"</div>\n\n",
        search, search);

    fulltext_search(search);

    while (get_fulltext_result(&b, &h, &v, &t)) {
      int i,in,var;
      snprintf(buf, sizeof(buf), "%s%d", b,h);
      tmp = StringEncode(buf);
      printf("\n<p> <a href=\"pismo.cgi?c=%s\">%s%d", tmp, b, h);
      if (v!=-1) printf(",%d", v);
      printf("</a>: ");
      free(tmp);
      for (i=in=var=0; t[i]; i++) {
        if (t[i]=='<') {
          in = 1;
          if (!strncmp(t+i+1, "var>", 4)) var = 1;
          if (!strncmp(t+i+1, "/var>", 5)) var = 0;
        }
        if (!in && !var) printf("%c", t[i]);
        if (t[i]=='>') in = 0;
      }
    }

    printf("<p>\n"
        "<form action=\"pismo.cgi\" method=\"get\">\n"
        "<input type=\"text\" name=\"c\">\n"
        "<input type=\"submit\" value=\"Zobraz\">\n"
        "</form>\n");
    printf("<p>\n"
        "<form action=\"pismo.cgi\" method=\"get\">\n"
        "<input type=\"text\" name=\"search\">\n"
        "<input type=\"submit\" value=\"Hľadaj\">\n"
        "</form>\n");
    
    free_fulltext_search();
  } else {
    char *ct;
    if (!get_citania(y,m,d, &zalm, &ct)) return 3;

    kalendar = 1;

    //  printf("%s %s %s %s\n", row[0], row[1], row[2], row[3]);

    printf("<title>Čítania na %d.%d.%d</title>\n"
        "</head><body>\n"
        "<div class=\"nadpis\">Liturgické čítania na %d.%d.%d</div>\n\n",
        d,m,y,d,m,y);
    scan_string(ct);
    yyparse();

    /* toto asi nechceme
    {
      char *tmp = StringEncode(kontext.buf+1);
      printf("<p>\n"
          "<a href=\"pismo.cgi?c=%s\">Kontext</a>", tmp);
      free(tmp);
    }
    */

    free(zalm);
    free(ct);
  }
  printf("</body></html>\n");

  free_scan_string();

  db_close();
  return 0;
}
