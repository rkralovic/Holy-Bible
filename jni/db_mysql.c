#include <mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>
#include "common.h"
#include "db.h"

#define DBUSER ""  // use current user
#define DBPASS ""
#ifndef MYSQL_SOCKET
#define MYSQL_SOCKET NULL
#endif

static char *book;
static MYSQL_RES *result;
static MYSQL *conn;
static struct strbuf q1, q2;

int db_init() {
  if (!(conn = mysql_init(NULL))) return -1;
  mysql_options(conn, MYSQL_SET_CHARSET_NAME, "utf8");
  if (!mysql_real_connect(conn, "localhost", DBUSER, DBPASS, "pismo", 0, MYSQL_SOCKET, 0)) {
    fprintf(stderr, "Failed to connect to database: Error: %s\n", mysql_error(conn));
    return -2;
  }
  return 1;
}

void db_close() {
  mysql_close(conn);
}

char *check_book(char *s) {
  char buf[1024];
  char *se,*out;
  MYSQL_RES *result;
  MYSQL_ROW row;

  se = (char *)malloc(strlen(s)*3+1);
  mysql_real_escape_string(conn, se, s, strlen(s));
  if (strlen(se)>sizeof(buf)/2) return NULL;
  snprintf(buf, sizeof(buf), "select spis from doc__biblia_obsah where spis='%s' or coalesce(alias,spis)='%s' limit 1", se, se); 
  free(se);
  mysql_query(conn, buf);
  result = mysql_store_result(conn);
  if (!(row = mysql_fetch_row(result))) return NULL; // nothing found
  out = (char *)malloc(strlen(row[0])+1);
  strcpy(out, row[0]);
  mysql_free_result(result);
  return out;
}

void init_search(char *b) {
  book = (char *)malloc(strlen(b)+1);
  strcpy(book, b);
  result = NULL;
  InitBuf(&q1); InitBuf(&q2);
  Rst(&q1); Rst(&q2);
  Prn(&q1, "(select 1 as typ, id, html from doc__biblia_text o where false");
  Prn(&q2, ") union (select 2 as typ, end as id, html from doc__biblia_ppc o where false");
}

void free_search() {
  free(book);
  FreeBuf(&q1); FreeBuf(&q2);
  if (result) mysql_free_result(result);
}

void add_search(int hb, int vb, int he, int ve) {
  if (ve==-1 || vb==-1) {
    Prn(&q1, " or ( o._spis='%s' and o._1>=%d and o._1<=%d ) ",
           book, hb, he);
    Prn(&q2, " or ( o.end>=(select min(id) from doc__biblia_text t where _spis='%s' and _1>=%d and _1<=%d) and "
           "      o.start<=(select max(id) from doc__biblia_text t where _spis='%s' and _1>=%d and _1<=%d) )",
           book, hb, he, book, hb, he);
  } else {
    Prn(&q1, " or ( o.end>=(select min(id) from doc__biblia_text t where _spis='%s' and _1=%d and _2=%d) and "
         "      o.start<=(select max(id) from doc__biblia_text t where _spis='%s' and _1=%d and _2=%d) )", 
         book, hb, vb, book, he, ve);
    Prn(&q2, " or ( o.end>=(select min(id) from doc__biblia_text t where _spis='%s' and _1=%d and _2=%d) and "
         "      o.start<=(select max(id) from doc__biblia_text t where _spis='%s' and _1=%d and _2=%d) )", 
         book, hb, vb, book, he, ve);
  }
}

void do_search() {
  char *q;
  q = (char *)malloc(strlen(q1.buf)+strlen(q2.buf)+1024);
  sprintf(q, "%s%s) order by id, typ", q1.buf, q2.buf);
  mysql_query(conn, q);
  result = mysql_store_result(conn);
  free(q);
}

int get_result(int *flags, char **s) {
  MYSQL_ROW row;
  if (!(row = mysql_fetch_row(result))) return 0;
  if (!strcmp(row[0], "2")) {
    *flags = RESULT_FLAG_COMMENT;
  } else {
    *flags = 0;
  }
  *s = row[2];
  return 1;
}

int get_citania(int y, int m, int d, char **zt, char **ct) {
  char query[1024];
  MYSQL_RES *result;
  MYSQL_ROW row;

  snprintf(query, sizeof(query), "select nazov,typ,citania,zalm from liturgicke_citania where datum='%d-%02d-%02d'", y,m,d);
  mysql_query(conn, query);
  result = mysql_store_result(conn);

  if (!(row = mysql_fetch_row(result))) return 0;
  *ct = (char *)malloc(strlen(row[2])+1); strcpy(*ct, row[2]);
  *zt = (char *)malloc(strlen(row[3])+1); strcpy(*zt, row[3]);
  mysql_free_result(result);
  return 1;
}

void get_prev(char *b, int h, char **ob, int *oh) {
  char buf[1024];
  MYSQL_RES *result;
  MYSQL_ROW row;

  snprintf(buf, sizeof(buf), "select _spis, _1 from doc__biblia_text where id < (select min(id) from doc__biblia_text where _spis='%s' and _1=%d) and _1 is not null order by id desc limit 1", b, h); 
  mysql_query(conn, buf);
  result = mysql_store_result(conn);
  if ((row = mysql_fetch_row(result))) {
    *ob = (char *)malloc(strlen(row[0])+1); strcpy(*ob, row[0]);
    sscanf(row[1], "%d", oh);
  } else *ob = NULL;
  mysql_free_result(result);
}

void get_next(char *b, int h, char **ob, int *oh) {
  char buf[1024];
  MYSQL_RES *result;
  MYSQL_ROW row;

  snprintf(buf, sizeof(buf), "select _spis, _1 from doc__biblia_text where id > (select max(id) from doc__biblia_text where _spis='%s' and _1=%d) and _1 is not null order by id asc limit 1", b, h); 
  mysql_query(conn, buf);
  result = mysql_store_result(conn);
  if ((row = mysql_fetch_row(result))) {
    *ob = (char *)malloc(strlen(row[0])+1); strcpy(*ob, row[0]);
    sscanf(row[1], "%d", oh);
  } else *ob = NULL;
  mysql_free_result(result);
}

void get_first(char **ob, int *oh) {
  // not very nice to hardcode this, but it should be quite stable :-)
  *oh = 1;
  *ob = strdup("Gn");
}

void fulltext_search(char *s) {
  char *q;
  q = (char *)malloc(3*strlen(s)+1024);
  sprintf(q, "%s%s order by id, typ", q1.buf, q2.buf);
  sprintf(q, "select _spis, _1, coalesce(_2,''), html from doc__biblia_text where html like '%%");
  mysql_real_escape_string(conn, q+strlen(q), s, strlen(s));
  sprintf(q+strlen(q), "%%'");
  mysql_query(conn, q);
  result = mysql_store_result(conn);
  free(q);
}

void free_fulltext_search() {
  if (result) mysql_free_result(result);
}

int get_fulltext_result(char **b, int *h, int *v, char **t) {
  MYSQL_ROW row;
  if (!(row = mysql_fetch_row(result))) return 0;
  *b = row[0];
  sscanf(row[1], "%d", h);
  *v = 1;
  sscanf(row[2], "%d", v);
  *t = row[3];
  return 1;
}

const char* get_uvod(int id) {
  // TODO(riso): implement me
  return "";
}

const char* get_uvod_kniha(int id) {
  // TODO(riso): implement me
  return "";
}

int get_uvod_pre_knihu(const char* b) {
  // TODO(riso): implement me
  return 0;
}

void set_translation(int translation) {
  // TODO(riso): implement me
}
