#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>
#include "common.h"
#include "parser.tab.h"
#include "db.h"
#include "db_bin.h"

#ifndef NOANDROID
#include <jni.h>
#endif

char *zalm, *aleluja;
int kalendar;
int html_id;
int comments = 1;
int uvod_kniha = -1;

struct strbuf kontext, out;

int kontext_last_hb, kontext_last_he;
struct kap {
  char *k;
  int h;
} first, last;

void Print(struct casti *_c) {
  char *k, *s;
  int vb, ve, hb, he, comment;
  struct casti *c;

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
    if (uvod_kniha < 0) {
      uvod_kniha = get_uvod_pre_knihu(k);
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
    if (!comment) {
      Prn(&out, "%s\n", s);
    } else {
      Prn(&out, "<span id=\"e%df\" ", html_id);
      Prn(&out, "onclick=\"ToggleComment('e%d')\" ", html_id);
      Prn(&out, "class=\"%skomentar\">(*)</span>\n", comments ? "skryty" : "", s);
      Prn(&out, "<span id=\"e%d\" ", html_id);
      Prn(&out, "onclick=\"ToggleComment('e%d')\" ", html_id);
      Prn(&out, "class=\"%skomentar\">(%s)</span>\n", comments ? "" : "skryty", s);
      html_id++;
    }
  }
  free_search();

  // do not free k - it might be used in first/last
}

// FIXME: dealokovat?
void Process(struct citania *l) {
  int i;
  for (i=0; l!=NULL; i++, l=l->n) {
    struct varianty *v;
    int first;

//    Prn(&out, "citanie: %s\n", l->tag);
    if (l->tag != NULL) {
      i = 0;
      Prn(&out, "<div class=\"tagMajor\">%s</div>\n\n", l->tag);
    }
    if (l->l && kalendar) {
      char *a = NULL;
      for (v = l->l; v != NULL; v = v->n) {
        if (v->l && v->tag) {
          a = v->tag;
          break;
        }
      }

      if (i%2 && l->n) {
        Prn(&out, "<div class=\"citanie\">Žalm</div>\n\n");
        Prn(&out, "<p><span class=\"redbold\">R: </span><span class=\"it\">%s</span></p>\n\n", a ? a : zalm);
      }
      else if (l->n) Prn(&out, "<div class=\"citanie\">%d. čítanie</div>\n\n", i/2 + 1);
      else {
        Prn(&out, "<p><span class=\"redbold\">Aleluja: </span><span class=\"it\">%s</span></p>\n\n", a ? a : aleluja);
        Prn(&out, "<div class=\"citanie\">Evanjelium</div>\n\n");
      }

    }

    for (first = 1, v = l->l; v != NULL; v = v->n) {
/*
      struct casti *c;
      char K[MAXLEN];
      int V,H;

      Prn(&out, "<div class=\"suradnice\">");
      for (c=v->l; c!=NULL; c=c->n) {
        if (c->k[0]) SSTRC(K, c->k);
        if (c->h!=-1) H = c->h;
        if (c->v!=-1) V = c->v;
        Prn(&out, "%s %d,%d ", K, H, V);
        if (c->range) Prn(&out, "- ");
        else if (c->n) Prn(&out, ". ");
      }
      Prn(&out, "</div>\n\n");
*/

      if (v->l) {
//        Prn(&out, "varianta: %s\n", v->tag);
        if (!first) Prn(&out, "<div class=\"varianta\">Alebo:</div>\n\n");
        Print(v->l);
        first = 0;
      } else if (v->tag) {
        Prn(&out, "<div class=\"tagMinor\">%s</div>\n\n", v->tag);
      }
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
    }
//    } else if (s[0]=='+') out[i]=' ';
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

int IsSingleChapter(const char* b) {
  return !strcmp(b, "Flm") ||
         !strcmp(b, "2Jn") ||
         !strcmp(b, "3Jn") ||
         !strcmp(b, "Júd");
}

void TOC() {
  char *b1, *b2, *q;
  int h1, h2, h2prev;
  get_first(&b1, &h1);
  int uvod_prev = -1;
  while (b1 != NULL) {
    h2prev = h1;
    int uvod = get_uvod_pre_knihu(b1);
    if (uvod != -1) {
      for (uvod_prev++; uvod_prev < uvod; uvod_prev++) {
        Prn(&out, "<p><a href=\"pismo.cgi?uvod=%d\">%s</a>", uvod_prev, get_uvod_kniha(uvod_prev));
      }
    }
    int single_chapter = IsSingleChapter(b1);
    if (uvod != -1) {
      Prn(&out, "<p><a href=\"pismo.cgi?uvod=%d\">%s</a>: ", uvod, b1);
    } else {
      Prn(&out, "<p> %s: ", b1);
    }
    if (!single_chapter) {
      Prn(&out, "<a href=\"pismo.cgi?c=%s%d\">%d</a>", b1, h1, h1);
    }
    for (get_next(b1, h1, &b2, &h2);
          b2 != NULL && !strcmp(b1,b2); 
          q = b2, get_next(b2, h2, &b2, &h2), free(q)) {
      if (!single_chapter) {
        Prn(&out, ", <a href=\"pismo.cgi?c=%s%d\">%d</a>", b2, h2, h2);
      }
      h2prev = h2;
    }
    if (single_chapter) {
      Prn(&out, "<a href=\"pismo.cgi?c=%s%d-%d\">všetko</a>", b1, h1, h2prev);
    }
    Prn(&out, "\n");
    free(b1);
    b1 = b2;
    h1 = h2;
    if (uvod != -1) uvod_prev = uvod;
  }
}

void ShortTOC() {
  char *b1, *b2, *q;
  int h1, h2, lasth;
  get_first(&b1, &h1);
  while (b1 != NULL) {
    int single_chapter = IsSingleChapter(b1);
    Prn(&out, "<span style=\"display: inline-block; min-width:7em;\">");
    if (!single_chapter) {
      Prn(&out, "%s:&nbsp;<a href=\"pismo.cgi?c=%s%d\">%d</a>-", b1, b1, h1, h1);
    }
    for (get_next(b1, h1, &b2, &h2); b2!=NULL && !strcmp(b1,b2); q=b2, get_next(b2, h2, &b2, &h2), free(q)) lasth = h2;
    if (!single_chapter) {
      Prn(&out, "<a href=\"pismo.cgi?c=%s%d\">%d</a></span>", b1, lasth, lasth);
    } else {
      Prn(&out, "<a href=\"pismo.cgi?c=%s%d-%d\">%s</a></span>", b1, h1, lasth, b1);
    }
    Prn(&out, "\n");
    free(b1);
    b1 = b2;
    h1 = h2;
  }
}

void CommonMain(const char* qstr, const char* css, int css_len) {
  int d,m,y;
  char query[1024];
  char *coord=NULL;
  char *search=NULL;
  char *obsah=NULL;
  int uvod=-1;
  char buf[1024];
  char *tmp;

  time_t t;
  // struct tm *tt;

  aleluja = zalm = NULL;

  /*
  time(&t);
  tt = localtime(&t);
  d = tt->tm_mday;
  m = tt->tm_mon+1;
  y = tt->tm_year+1900;
  */
  d = m = y = -1;
  html_id=0;

  if (qstr) {
    char *s;
    s = strstr(qstr, "d="); if (s) sscanf(s, "d=%d", &d);
    s = strstr(qstr, "m="); if (s) sscanf(s, "m=%d", &m);
    s = strstr(qstr, "y="); if (s) sscanf(s, "y=%d", &y);
    s = strstr(qstr, "c=");
    if (s) s+=2; else { // spatna kompatibilita so znackami
      s = strstr(qstr, "in=");
      if (s) s+=3;
    }
    if (s) {
      query[0]=0;
      sscanf(s, "%1000[^&]", query);
      coord=StringDecode(query);
    }
    s = strstr(qstr, "obsah=");
    if (s) {
      query[0]=0;
      sscanf(s+6, "%1000[^&]", query);
      obsah = StringDecode(query);
    }
    s = strstr(qstr, "search=");
    if (s) {
      query[0]=0;
      sscanf(s+7, "%1000[^&]", query);
      search = StringDecode(query);
    }
    s = strstr(qstr, "zalm=");
    if (s) {
      query[0]=0;
      sscanf(s+5, "%1000[^&]", query);
      zalm = StringDecode(query);
    }
    s = strstr(qstr, "aleluja=");
    if (s) {
      query[0]=0;
      sscanf(s+8, "%1000[^&]", query);
      aleluja = StringDecode(query);
    }
    s = strstr(qstr, "uvod=");
    if (s) {
      sscanf(s+5, "%d", &uvod);
    }
  }

  InitBuf(&out); Rst(&out);
  Prn(&out, "<html><head>\n"
//      "<link rel=\"stylesheet\" href=\"breviar.css\">\n"
      "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">\n"
//      "<meta name=\"viewport\" content=\"width=100%; initial-scale=1; maximum-scale=1; minimum-scale=1; user-scalable=no;\" />"
      );

#ifdef NOANDROID
  Prn(&out, "<link rel=\"stylesheet\" href=\"breviar.css\">\n");
#else
  Prn(&out, "<style type=\"text/css\">\n<!--\n");
  Cpy(&out, css, css_len);
  Prn(&out, "\n--></style>");
#endif

  Prn(&out, "<script type=\"text/javascript\">\n"
#ifdef NOANDROID
      "function submitsearch() {\n"
      "  this.document.location.href = \"pismo.cgi?search=\"+document.getElementById('searchstring').value ;\n"
      "}\n"
      "function submitcoord() {\n"
      "  this.document.location.href = \"pismo.cgi?c=\"+document.getElementById('zobraz').value ;\n"
      "}\n"
#else
      "function submitsearch() {\n"
      "  bridge.loadit(\"pismo.cgi?search=\"+document.getElementById('searchstring').value );\n"
      "}\n"
      "function submitcoord() {\n"
      "  bridge.loadit(\"pismo.cgi?c=\"+document.getElementById('zobraz').value );\n"
      "}\n"
#endif
      "function ToggleClass(eid) {\n"
      "  if (document.getElementById(eid).className == \"komentar\") {\n"
      "    document.getElementById(eid).className = \"skrytykomentar\";\n"
      "  } else {\n"
      "    document.getElementById(eid).className = \"komentar\";\n"
      "  }\n"
      "}\n"
      "function ToggleComment(eid) {\n"
      "  ToggleClass(eid);\n"
      "  ToggleClass(eid+\"f\");\n"
      "}\n"
      "</script>\n");

  InitBuf(&kontext);
  Rst(&kontext);
  kontext_last_hb=kontext_last_he=-1;
  first.h=last.h=-1;

  if (coord && d==-1) {
    Prn(&out, "<title>%s</title>\n"
      "</head><body><div id=\"contentRoot\">\n"
      "<div class=\"nadpis\">%s</div>\n\n", coord, coord);
    kalendar = 0;
    scan_string(coord);
    yyparse();

    if (strlen(kontext.buf)>1) {
      char *tmp = StringEncode(kontext.buf+1);
      Prn(&out, // "<p>\n"
          "<a href=\"pismo.cgi?c=%s\">Kontext</a>", tmp);
      free(tmp);
      Prn(&out, "&nbsp; &nbsp;");
    }
    if (uvod_kniha >= 0 ) {
      Prn(&out, // "<p>\n"
          "<a href=\"pismo.cgi?uvod=%d\">Úvod</a>", uvod_kniha);
      Prn(&out, "&nbsp; &nbsp;");
    }
    if (first.h!=-1) {
      char *b;
      int h;
      get_prev(first.k, first.h, &b, &h);
      if (b!=NULL) {
        snprintf(buf, sizeof(buf), "%s %d", b, h);
        tmp = StringEncode(buf);
        Prn(&out, // "<p>\n"
            "<a href=\"pismo.cgi?c=%s\">Dozadu (%s %d)</a>", tmp, b, h);
        free(tmp);
        free(b);
      }
    }
    Prn(&out, "&nbsp; &nbsp;");
    if (last.h!=-1){
      char *b;
      int h;
      get_next(last.k, last.h, &b, &h); if (b!=NULL) {
        snprintf(buf, sizeof(buf), "%s %d", b, h);
        tmp = StringEncode(buf);
        Prn(&out, // "<p>\n"
            "<a href=\"pismo.cgi?c=%s\">Dopredu (%s %d)</a>", tmp, b, h);
        free(tmp);
        free(b);
      }
    }

    free_scan_string();
  } else if (search && strlen(search)) {
    char *b,*t;
    int h,v,cnt;

    Prn(&out, "<title>Vyhľadávanie \"%s\"</title>\n"
        "</head><body><div id=\"contentRoot\">\n"
        "<div class=\"nadpis\">Vyhľadávanie \"%s\"</div>\n\n",
        search, search);

    fulltext_search(search);
    for (cnt=0; get_fulltext_result(&b, &h, &v, &t); cnt++) {
      int i,in,var;
      snprintf(buf, sizeof(buf), "%s%d", b,h);
      tmp = StringEncode(buf);
      Prn(&out, "\n<p> <a href=\"pismo.cgi?c=%s\">%s%d", tmp, b, h);
      if (v!=-1) Prn(&out, ",%d", v);
      Prn(&out, "</a>: ");
      free(tmp);
      for (i=in=var=0; t[i]; i++) {
        if (t[i]=='<') {
          in = 1;
          if (!strncmp(t+i+1, "var>", 4)) var = 1;
          if (!strncmp(t+i+1, "/var>", 5)) var = 0;
        }
        if (!in && !var) Prn(&out, "%c", t[i]);
        if (t[i]=='>') in = 0;
      }
      if (cnt==1000) {
        Prn(&out, "\n<p> Príliš veľa výsledkov!\n");
        break;
      }
    }

    free_fulltext_search();
  } else if (coord) {
    kalendar = 1;
    //  Prn(&out, "%s %s %s %s\n", row[0], row[1], row[2], row[3]);

    Prn(&out, "<title>Čítania na %d.%d.%d</title>\n"
        "</head><body><div id=\"contentRoot\">\n"
        "<div class=\"nadpis\">Liturgické čítania na %d.%d.%d</div>\n\n",
        d,m,y,d,m,y);
    scan_string(coord);
    yyparse();

    /* toto asi nechceme
    {
      char *tmp = StringEncode(kontext.buf+1);
      Prn(&out, "<p>\n"
          "<a href=\"pismo.cgi?c=%s\">Kontext</a>", tmp);
      free(tmp);
    }
    */
    free_scan_string();

  } else if (uvod >= 0) {
    const char* kniha = get_uvod_kniha(uvod);
    Prn(&out, "<title>%s</title>\n"
        "</head><body>\n"
        "<div class=\"nadpis\">%s</div>\n\n%s<p>", kniha, kniha, get_uvod(uvod));
    if (uvod > 0 ) {
      Prn(&out, // "<p>\n"
          "<a href=\"pismo.cgi?uvod=%d\">Dozadu</a>", uvod - 1);
      Prn(&out, "&nbsp; &nbsp;");
    }
    Prn(&out, // "<p>\n"
        "<a href=\"pismo.cgi?uvod=%d\">Dopredu</a>", uvod + 1);
  } else if (obsah) {
    Prn(&out, "<title>Obsah</title>\n"
        "</head><body><div id=\"contentRoot\">\n"
        "<div class=\"nadpis\">Obsah</div>\n\n");
    TOC();
  } else {
    Prn(&out, "<title>Obsah</title>\n"
        "</head><body><div id=\"contentRoot\">\n"
        "<div class=\"nadpis\">Sväté Písmo</div>\n\n");
    ShortTOC();
  }
  if (zalm) free(zalm);
  if (aleluja) free(aleluja);
  if (coord) free(coord);
  if (obsah) free(obsah);
  // FIXME: leaks from the structure!

  Prn(&out, "<p>\n"
      "<input type=\"text\" id=\"zobraz\">\n"
      "<button onClick=\"submitcoord()\">Zobraz</button>\n");
  Prn(&out, "<p>\n"
      "<input type=\"text\" id=\"searchstring\">\n"
      "<button onClick=\"submitsearch()\">Hľadaj</button>\n");
  Prn(&out, "<p>\n"
      "<a href=\"pismo.cgi?obsah=long\">Obsah</a>\n");
    
  Prn(&out, "</div></body></html>\n");

  FreeBuf(&kontext);
  db_close();
}

#ifdef NOANDROID
int main() {
  const char *qstr;
  qstr = getenv("QUERY_STRING");
  if (db_init() < 0) return 1;
  printf("Content-Type: text/html; charset=UTF-8\n\n");
  CommonMain(qstr, 0, 0);
  printf("%s", out.buf);
  FreeBuf(&out);
  return 0;
}
#else
jstring Java_sk_ksp_riso_svpismo_svpismo_process(JNIEnv* env, jobject thiz, jobject _db, jlong _dblen, jobject _css, jlong css_len, jstring querystring, jboolean _comments) {
  jstring jout;
  const char *qstr;

  db_len = _dblen;
  db = (*env)->GetDirectBufferAddress(env,_db);
  if (db==NULL) return (*env)->NewStringUTF(env, "");
  comments = _comments;
  qstr = (*env)->GetStringUTFChars(env, querystring, NULL);
  __android_log_print(ANDROID_LOG_INFO, "svpismo", "qstr = %s\n", qstr);

  if (db_init() < 0) return (*env)->NewStringUTF(env, "");

  CommonMain(qstr, (*env)->GetDirectBufferAddress(env,_css), css_len);

  if (qstr) (*env)->ReleaseStringUTFChars(env,  querystring, qstr);
  {
    char *tmp = StringEncode(out.buf);
//    __android_log_print(ANDROID_LOG_INFO, "svpismo", "reply = %s\n", tmp);
    jout = (*env)->NewStringUTF(env, tmp);
    free(tmp);
  }
  FreeBuf(&out);
  return jout;
}
#endif
