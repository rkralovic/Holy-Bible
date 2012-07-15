#include <mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>
#include "common.h"
#include "db_bin.h"

#define DBUSER ""  // use current user
#define DBPASS ""
#ifndef MYSQL_SOCKET
#define MYSQL_SOCKET NULL
#endif

static MYSQL *conn;

int len, alloc;
void *buf;

void buf_init() {
  len = 0;
  alloc = 1024;
  buf = malloc(alloc);
};

void buf_free() { free(buf); }

int buf_alloc(int l) {
  if (l<0) l = 0;
  l = ((l+3)/4)*4; // align for 32bit words
  if (len+l>=alloc) {
    alloc = 2*(len+l);
    buf = realloc(buf, alloc);
  }
  len += l;
  return len-l;
};

int buf_add(void *data, int l) {
  int a = buf_alloc(l);
  memcpy(buf+a, data, l);
  return a;
}

void dump(char *fn) {
  FILE *f;

  if (!(f = fopen(fn, "w"))) exit(1);
  fwrite(buf, 1, len, f);
  fclose(f);
}

#define ASGN(x,y) { int tmp; tmp = y; x = tmp; }

int main() {
  MYSQL_ROW row;
  MYSQL_RES *result;
  int i;
  char query[1024];

  int hdr;
#define HDR ((struct header *)(buf+hdr))

  if (!(conn = mysql_init(NULL))) return -1;
  mysql_options(conn, MYSQL_SET_CHARSET_NAME, "utf8");
  if (!mysql_real_connect(conn, "localhost", DBUSER, DBPASS, "pismo", 0, MYSQL_SOCKET, 0)) {
    fprintf(stderr, "Failed to connect to database: Error: %s\n", mysql_error(conn));
    return -2;
  }

  buf_init();
  hdr = buf_alloc(sizeof(struct header));
  HDR->signature = SIGNATURE;

  mysql_query(conn, "select o.id, o.spis, coalesce(o.alias,''), max(coalesce(t._1,0)), min(coalesce(t.id,1000000))-1, max(coalesce(t.id,-1))-1 "
      "from doc_biblia_obsah o left outer join doc_biblia_text t on o.id=t._0 "
      "group by o.id, o.spis, coalesce(o.alias,'') "
      "order by o.id asc");
  result = mysql_store_result(conn);
  HDR->n_knihy = mysql_num_rows(result);
  ASGN(HDR->knihy, buf_alloc(HDR->n_knihy * sizeof(struct kniha)));
#define KNH ((struct kniha *)(buf+HDR->knihy))
  i=0;
  while ((row = mysql_fetch_row(result))) {
    int j;

    sscanf(row[0], "%d", &j);
    if (i!=j-1) { fprintf(stderr, "nesuvisle id\n"); exit(1); }

    ASGN(KNH[i].meno, buf_add(row[1], strlen(row[1])+1));
    ASGN(KNH[i].alias, buf_add(row[2], strlen(row[2])+1));
    sscanf(row[3], "%d", &KNH[i].n_hlav);
    sscanf(row[4], "%d", &KNH[i].min);
    sscanf(row[5], "%d", &KNH[i].max);
    ASGN(KNH[i].hlava, buf_alloc(sizeof(struct hlava)*KNH[i].n_hlav));

    for (j=0; j<KNH[i].n_hlav; j++) {
      MYSQL_ROW row2;
      MYSQL_RES *result2;
      int k;
#define HLV ((struct hlava *)(buf+KNH[i].hlava))

      sprintf(query, "select min(coalesce(t.id,1000000))-1, max(coalesce(t.id,-1))-1 "
          "from doc_biblia_text t "
          "where t._0=%d and t._1=%d", i+1, j+1);
      mysql_query(conn, query);
      result2 = mysql_store_result(conn);
      row2 = mysql_fetch_row(result2);
      sscanf(row2[0], "%d", &HLV[j].min);
      sscanf(row2[1], "%d", &HLV[j].max);
      // printf("kniha %d, hlava %d, min %d, max %d\n", i, j, HLV[j].min, HLV[j].max);
      mysql_free_result(result2);

      sprintf(query, "select t._2, min(t.id)-1, max(t.id)-1 "
          "from doc_biblia_text t "
          "where t._0=%d and t._1=%d and t._2 is not null group by t._2", i+1, j+1);
      mysql_query(conn, query);
      result2 = mysql_store_result(conn);
      HLV[j].n_versov = mysql_num_rows(result2);
      // if (!HLV[j].n_versov) printf("kniha = %s; hlava = %d; versov=%d\n", row[1], j, HLV[j].n_versov);
      ASGN(HLV[j].vers, buf_alloc(sizeof(struct vers)*HLV[j].n_versov));

      for (k=0; k < HLV[j].n_versov; k++) {
        int l;
#define VRS ((struct vers *)(buf+HLV[j].vers))
        row2 = mysql_fetch_row(result2);
        sscanf(row2[0], "%d", &l);
        if (l-1 != k) { fprintf(stderr, "nesuvisle id versa k=%d h=%d v=%d!=%d\n", i+1, j+1, l, k+1); exit(1); }
        sscanf(row2[1], "%d", &VRS[k].min);
        sscanf(row2[2], "%d", &VRS[k].max);
        // printf("k=%d min=%d=%s max=%d=%s\n", k, VRS[k].min, row2[1], VRS[k].max, row2[2]);
      }

      mysql_free_result(result2);
    }

    i++;
  }
  mysql_free_result(result);

  mysql_query(conn, "select max(coalesce(id,0)) from doc_biblia_text where _0 is not null order by id asc");
  result = mysql_store_result(conn);
  row = mysql_fetch_row(result);
  sscanf(row[0], "%d", &HDR->n_text);
  mysql_free_result(result);

  mysql_query(conn, "select id, _0-1, coalesce(_1, 0)-1, coalesce(_2, 0)-1, coalesce(html,'') from doc_biblia_text where _0 is not null order by id asc");
  result = mysql_store_result(conn);
  i=0;
  ASGN(HDR->text, buf_alloc(sizeof(struct text)*HDR->n_text));
#define TXT ((struct text *)(buf+HDR->text))
  while ((row = mysql_fetch_row(result))) {
    int j;

    sscanf(row[0], "%d", &j);
    while (i<j-1) {
      TXT[i].k = TXT[i].h = TXT[i].v = -1;
      ASGN(TXT[i].t, buf_add("", 1));
      i++;
    }
    sscanf(row[1], "%d", &TXT[i].k);
    sscanf(row[2], "%d", &TXT[i].h);
    sscanf(row[3], "%d", &TXT[i].v);
    ASGN(TXT[i].t, buf_add(row[4], strlen(row[4])+1));
    // printf("txt %d = %s\n", i, row[4]);
    i++;
  }
  mysql_free_result(result);

  mysql_query(conn, "select max(coalesce(id,0)) from doc_biblia_ppc order by id asc");
  result = mysql_store_result(conn);
  row = mysql_fetch_row(result);
  sscanf(row[0], "%d", &HDR->n_ppc);
  mysql_free_result(result);

  mysql_query(conn, "select id, coalesce(html,'') from doc_biblia_ppc order by id asc");
  result = mysql_store_result(conn);
  i=0;
  ASGN(HDR->ppc, buf_alloc(sizeof(int32_t)*HDR->n_ppc));
#define PPC ((int32_t *)(buf+HDR->ppc))
  while ((row = mysql_fetch_row(result))) {
    int j;

    sscanf(row[0], "%d", &j);
    while (i<j-1) {
      ASGN(PPC[i], buf_add("", 1));
      i++;
    }
    ASGN(PPC[i], buf_add(row[1], strlen(row[1])+1));
    i++;
  }
  mysql_free_result(result);

  mysql_query(conn, "select datum, zalm, citania from liturgicke_citania order by datum asc");
  result = mysql_store_result(conn);
  HDR->n_kalendar = mysql_num_rows(result);
  ASGN(HDR->kalendar, buf_alloc(sizeof(struct kalendar)*HDR->n_kalendar));
#define KAL ((struct kalendar *)(buf+HDR->kalendar))
  i = 0;
  while ((row = mysql_fetch_row(result))) {
    sscanf(row[0], "%d-%d-%d", &KAL[i].y, &KAL[i].m, &KAL[i].d);
    ASGN(KAL[i].zalm, buf_add(row[1], strlen(row[1])+1));
    ASGN(KAL[i].text, buf_add(row[2], strlen(row[2])+1));
    i++;
  }
  mysql_free_result(result);

  for (i=0; i<TABLES; i++) {
    int j;
    int min = (1<<i);
    int max = (i==TABLES-1) ? (1<<30) : (1<<(i+1))-1;
    sprintf(query, "(select 0, id-1, start-1 as start, end-1 from doc_biblia_text "
        "where end-start+1>=%d and end-start+1<=%d) union "
        "(select 1, id-1, start-1, end-1 from doc_biblia_ppc "
        "where end-start+1>=%d and end-start+1<=%d) order by start asc", min, max, min, max);
    mysql_query(conn, query);
    result = mysql_store_result(conn);

    HDR->n_item[i] = mysql_num_rows(result);
    ASGN(HDR->item[i], buf_alloc(sizeof(struct item)*HDR->n_item[i]));
#define ITM ((struct item *)(buf+HDR->item[i]))

    j = 0;
    while ((row = mysql_fetch_row(result))) {
      sscanf(row[0], "%d", &ITM[j].comment);
      sscanf(row[1], "%d", &ITM[j].id);
      sscanf(row[2], "%d", &ITM[j].b);
      sscanf(row[3], "%d", &ITM[j].e);
      j++;
    }
  }

  dump("pismo.bin");
  mysql_close(conn);
  return 0;
}
