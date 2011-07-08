#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <time.h>
#include <ctype.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "common.h"
#include "db_bin.h"

#define MAX_RES 65536

static const void *base;
void *db;
int db_len;

static const struct header *hdr;
static const struct kniha *knh;

#define STR(x) ((char *)(base+x))

static char *Normalize(const char *s) {
  static struct conv {
    char in[2];
    char out;
  } C[] = {
    { "ľ", 'l' },
    { "ĺ", 'l' },
    { "š", 's' },
    { "č", 'c' },
    { "ť", 't' },
    { "ž", 'z' },
    { "ý", 'y' },
    { "á", 'a' },
    { "é", 'e' },
    { "í", 'i' },
    { "ó", 'o' },
    { "ô", 'o' },
    { "ú", 'u' },
    { "ä", 'a' },
    { "ď", 'd' },
    { "ň", 'n' },
    { "ě", 'e' },
    { "ř", 'r' },
    { "Ľ", 'l' },
    { "Ĺ", 'l' },
    { "Š", 's' },
    { "Č", 'c' },
    { "Ť", 't' },
    { "Ž", 'z' },
    { "Ý", 'y' },
    { "Á", 'a' },
    { "É", 'e' },
    { "Í", 'i' },
    { "Ó", 'o' },
    { "Ô", 'o' },
    { "Ú", 'u' },
    { "Ä", 'a' },
    { "Ď", 'd' },
    { "Ň", 'n' },
    { "Ě", 'e' },
    { "Ř", 'r' },
  };
  static char T[65536];
  static int inited=0;
  static int n=sizeof(C)/sizeof(struct conv);
  char *out = (char *)malloc(strlen(s)+1);
  const char *i;
  char *j;
  int k;

  if (!inited) {
    inited = 1;
    memset(T, 0, sizeof(T));
    for (k=0; k<n; k++)
      T[((int)((uint8_t)C[k].in[0]))<<8 | (int)(((uint8_t)C[k].in[1]))] = C[k].out;
  }
  for (i=s, j=out; *i; i++, j++) {
    *j = T[((int)((uint8_t)i[0]))<<8 | (int)((uint8_t)i[1])];
    if (!*j) *j = tolower(*i);
    else i++;
  }
  *j = 0;
  return out;
}


void db_close() {
}

int db_init() {
  base = db;

  __android_log_print(ANDROID_LOG_INFO, "svpismo", "db_init: db=%x, db_len=%d\n", db, db_len);
  hdr = (struct header *)base;
  if (hdr->signature!=SIGNATURE) {
    __android_log_print(ANDROID_LOG_INFO, "svpismo", "db_init: wrong signature %x != %x\n", hdr->signature, SIGNATURE);
    db_close();
    //fprintf(stderr, "bad db signature\n");
    return -1;
  }

  knh = (struct kniha *)(base+hdr->knihy);
  
  return 1;
}

static int cmp(const char *a, const char *b) {
  char *_a = Normalize(a);
  char *_b = Normalize(b);
  int out = strcmp(_a, _b);
  free(_a); free(_b);
  return out;
}

static int find_book(char *s) {
  int i;
  for (i=0; i<hdr->n_knihy; i++) {
    if (!cmp( s, STR(knh[i].meno) )) return i;
    if (!cmp( s, STR(knh[i].alias) )) return i;
  }
  return -1;
}

char *check_book(char *s) {
  int i;
  char *out;
  i = find_book(s);
  if (i==-1) return NULL;
  out = (char *)malloc(strlen(STR(knh[i].meno))+1);
  strcpy(out, STR(knh[i].meno));
  return out;
}

int book;
int n,m;
struct res_t {
  int comment, id, t;
} res[MAX_RES];

int cmp_res(struct res_t *a, struct res_t *b) {
  if (a->id<b->id) return -1;
  if (a->id>b->id) return 1;
  if (a->comment>b->comment) return 1;
  if (a->comment<b->comment) return -1;
  return 0;
}

void init_search(char *b) {
  book = find_book(b);
  n = 0;
}

void free_search() {
}

static int find_coord(int k, int hl, int v, int dir) {
  struct hlava *H;
  struct vers *V;

  if (hl <= -1) return (dir==1) ? knh[k].min : knh[k].max;
  if (hl<0) hl = 0;
  if (hl>=knh[k].n_hlav) hl = knh[k].n_hlav -1;
  H = ((struct hlava *)(base + knh[k].hlava)) + hl;

  if (v <= -1) return (dir==1) ? H->min : H->max;
  if (v<0) v = 0;
  if (v>=H->n_versov) v = H->n_versov -1;
  V = ((struct vers *)(base + H->vers)) + v;

  return (dir==1) ? V->min : V->max;
}

static int get_id(int comment, int id, int e) {
  if (!comment) return id;
  else return e;
}

static int get_t(int comment, int id) {
  if (comment) {
    return ((int32_t *)(base+hdr->ppc))[id];
  } else {
    return ((struct text *)(base+hdr->text))[id].t;
  }
}

void add_search(int hb, int vb, int he, int ve) {
  int b = find_coord(book, hb-1, vb-1, 1);
  int e = find_coord(book, he-1, ve-1, -1);
  int i;
  struct item *I;

  if (e<=-1 || b<=-1) return;

  // binary search for <TABLES-1
  for (i=0; i<TABLES-1; i++) {
    int ni = hdr->n_item[i];
    int max = (1<<(i+1))-1;
    int l,r,p;
    I = (struct item *)(base+hdr->item[i]);

    l = -1; r = ni-1;
    if ( I[r].b > b-max ) { // if not all outside
      while (r-l>1) {
        p = (l+r)/2;
        if (I[p].b > b-max) r = p; 
        else l = p;
      }
      while (r<ni && I[r].b <= e) {
        if (I[r].e >= b) {
          res[n].comment=I[r].comment;
          res[n].id=get_id(I[r].comment, I[r].id, I[r].e);
          res[n].t=get_t(I[r].comment, I[r].id);
          n++;
        }
        r++;
      }
    }
  }

  // linear search for TABLES-1
  I = (struct item *)(base+hdr->item[TABLES-1]);
  for (i=0; i<hdr->n_item[TABLES-1]; i++) {
    if (I[i].b <=e && I[i].e >=b) {
      res[n].comment=I[i].comment;
      res[n].id=get_id(I[i].comment, I[i].id, I[i].e);
      res[n].t=get_t(I[i].comment, I[i].id);
      n++;
    }
  }
}

void do_search() {
  m=0;
  qsort(res, n, sizeof(struct res_t), (int (*)(const void*, const void*))cmp_res);
}

int get_result(int *c, char **s) {
  while (m<n && res[m].id==res[m+1].id && res[m].comment==res[m+1].comment) m++;
  if (m>=n) return 0;
  *c = res[m].comment;
  *s = STR( res[m].t );
  m++;
  return 1;
}

static int cmp_date(int y1, int m1, int d1, int y2, int m2, int d2) {
  if (y2>y1) return 1; if (y2<y1) return -1;
  if (m2>m1) return 1; if (m2<m1) return -1;
  if (d2>d1) return 1; if (d2<d1) return -1;
  return 0;
}

int get_citania(int y, int m, int d, char **zt, char **ct) {
  int l,r,p;
  struct kalendar *kal = (struct kalendar *)(base+hdr->kalendar);

  l = 0; r = hdr->n_kalendar;
  while (r-l>1) {
    p = (l+r)/2;
    if (cmp_date(y, m, d, kal[p].y, kal[p].m, kal[p].d)>0) r = p; else l = p;
  }
  if (cmp_date(y, m, d, kal[l].y, kal[l].m, kal[l].d)==0) {
    *ct = (char *)malloc(strlen(STR(kal[l].text))+1); strcpy(*ct, STR(kal[l].text));
    *zt = (char *)malloc(strlen(STR(kal[l].zalm))+1); strcpy(*zt, STR(kal[l].zalm));
    return 1;
  }
  return 0;
}

void get_prev_next(char *b, int hl, char **ob, int *oh, int dir) {
  int _b = find_book(b);
  struct kniha *knh = (struct kniha *)(base+hdr->knihy);
  if (_b==-1 || hl < 1 || hl > knh[_b].n_hlav ) {
    *ob = NULL;
    return;
  }

  hl+=dir;
  if (dir<0) {
    while (_b>=0 && hl==0) {
      _b--;
      if (b>=0) hl = knh[_b].n_hlav;
    }
  } else {
    while (_b < hdr->n_knihy && hl > knh[_b].n_hlav) {
      _b++;
      if (_b < hdr->n_knihy) hl = 1;
    }
  }
  if (_b < 0 || _b >= hdr->n_knihy) {
    *ob = NULL;
    return;
  }

  *ob = (char *)malloc( strlen(STR(knh[_b].meno))+1 ); strcpy(*ob,STR(knh[_b].meno));
  *oh = hl;
}

void get_prev(char *b, int hl, char **ob, int *oh) {
  get_prev_next(b,hl,ob,oh,-1);
}

void get_next(char *b, int hl, char **ob, int *oh) {
  get_prev_next(b,hl,ob,oh,1);
}

char *search;
void fulltext_search(char *s) {
  search = Normalize(s);
  m = 0;
}

void free_fulltext_search() {
  free(search);
}

int get_fulltext_result(char **b, int *hl, int *v, char **txt) {
  while (m<hdr->n_text) {
    struct text *t = (struct text *)(base+hdr->text);
    char *x = Normalize(STR( t[m].t));
    m++;
    if (strstr(x, search)!=NULL) {
      *b = STR( ((struct kniha *)(base+hdr->knihy))[t[m-1].k].meno);
      *hl = t[m-1].h+1;
      *v = t[m-1].v+1;
      if (*v == 0) *v = -1;
      *txt = STR(t[m-1].t);
      return 1;
    }
    free(x);
  }
  return 0;
}
