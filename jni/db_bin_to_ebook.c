#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "db.h"
#include "db_bin.h"

#define GETPTR(type, offset) ((struct type *)(GetBase() + (offset)))
#define GETSTR(offset) ((char *)(GetBase() + (offset)))
#define GET(type, offset) (*GETPTR(type, (offset)))
#define HDR GET(header, 0)
#define UVOD GETPTR(uvod, HDR.uvod)
#define KNIHA GETPTR(kniha, HDR.knihy)

void Iterate(void (*cb_uvod)(int), void (* cb_kniha)(int)) {
  int kniha, uvod;
  for (kniha = 3, uvod = 0; kniha < HDR.n_knihy || uvod < HDR.n_uvod; ) {
    if (uvod < HDR.n_uvod && (kniha >= HDR.n_knihy ||
                              uvod <= get_uvod_pre_knihu(GETSTR(KNIHA[kniha].meno)))) {
      cb_uvod(uvod);
      uvod++;
    } else {
      if (KNIHA[kniha].n_hlav > 0) {
        cb_kniha(kniha);
      }
      kniha++;
    }
  }
}

void cb_obsah_uvod(int uvod) {
  printf("<li><a href=\"#uvod_%d\">%s (úvod)</a>\n", uvod, GETSTR(UVOD[uvod].kniha));
}

void cb_obsah_kniha(int kniha) {
  char* meno = GETSTR(KNIHA[kniha].meno);
  printf("<li><a href=\"#%s\">%s</a>\n", meno, meno);
}

// TODO: dealokovat
void Process(struct citania *l) {
  int h, v;
  if (l == NULL) return;
  if (l->l == NULL) return;
  if (l->l->l == NULL) return;
  h = l->l->l->h;
  v = l->l->l->v;
  if (h < 1) { h = 1; v = 1; }
  if (v < 1) { v = 1; }
  printf("%s%d,%d", l->l->l->k, h, v);
}

void TranslateLinksAndPrint(char* s) {
  static const char *href = "href='?in=";
  char *e, *tmp;
  while (1) {
    e = strstr(s, href);
    if (e == NULL) break;

    tmp = strndup(s, e - s);
    printf("%shref='#", tmp);
    free(tmp);
    e += strlen(href);

    s = strstr(e, "'>");
    if (s == NULL) return;  // should not happen

    tmp = strndup(e, s - e);
    // workaround bison bug
    if (tmp[strlen(tmp) - 1] == ';') {
      tmp[strlen(tmp) - 1] = 0;
    }
    scan_string(tmp);
    yyparse();
    free_scan_string();
    free(tmp);
  }
  printf("%s", s);
}

void cb_main_uvod(int uvod) {
  printf("\n\n<a name=\"uvod_%d\">\n", uvod);
  TranslateLinksAndPrint(GETSTR(UVOD[uvod].text));
}

int html_id = 0;
void cb_main_kniha(int kniha) {
  int comment;
  char *s;
  int h, v, id;

  char* meno = GETSTR(KNIHA[kniha].meno);
  printf("\n\n<a name=\"%s\">\n", meno);

  init_search(meno);
  add_search(1, 1, -1, -1);
  do_search();

  h = 0; v = 0;
  while (get_result_id(&comment, &s, &id)) {
    if (!comment) {
      while (1) {
        // normalize h / v
        while (1) {
          if (h < 0 || h >= KNIHA[kniha].n_hlav) {
            h = -1;
            break;
          } else if (v >= GETPTR(hlava, KNIHA[kniha].hlava)[h].n_versov) {
            v = 0;
            h++;
          } else break;
        }
        if (h >= 0 &&
            GETPTR(vers, (GETPTR(hlava, KNIHA[kniha].hlava)[h].vers))[v].min <= id) {
          printf("<a name=\"%s%d,%d\">\n", meno, h + 1, v + 1);
          v++;
        } else break;
      }
      TranslateLinksAndPrint(s);
      printf("\n");
    } else {
      printf("<span id=\"e%d\" ", html_id);
      printf("class=\"komentar\">(");
      TranslateLinksAndPrint(s);
      printf(")</span>\n");
      html_id++;
    }
  }
}

int main() {
  db_init();
  printf("<!DOCTYPE html>\n");
  printf("<html><head>\n");
  printf("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n");
  printf("</head><body>\n");
  /*
  printf("<h1>Obsah</h1><ul>\n");
  Iterate(cb_obsah_uvod, cb_obsah_kniha);
  printf("</ul>\n\n");
  */
  Iterate(cb_main_uvod, cb_main_kniha);
  printf("</body></html>");
  db_close();
  return 0;
}
