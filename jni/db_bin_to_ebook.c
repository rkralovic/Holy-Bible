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

void cb_main_uvod(int uvod) {
  printf("\n\n<a name=\"uvod_%d\">\n%s\n\n", uvod, GETSTR(UVOD[uvod].text));
}

int html_id = 0;
void cb_main_kniha(int kniha) {
  int comment;
  char *s;

  char* meno = GETSTR(KNIHA[kniha].meno);
  printf("\n\n<a name=\"%s\">\n", meno);

  init_search(meno);
  add_search(1, 1, -1, -1);
  do_search();

  while (get_result(&comment, &s)) {
    if (!comment) {
      printf("%s\n", s);
    } else {
      printf("<span id=\"e%d\" ", html_id);
      printf("class=\"komentar\">(%s)</span>\n", s);
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
