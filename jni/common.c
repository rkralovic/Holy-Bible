#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>
#include "common.h"

void Rst(struct strbuf *q) { q->i = 0; q->buf[0] = 0; }
void Prn(struct strbuf *q, char *fmt, ...) {
  va_list ap;
  int qj;
  va_start(ap, fmt);
  qj = vsnprintf(q->buf+q->i, 0, fmt, ap);
  va_end(ap);
  if (q->i+qj >= q->alen-1) {
    q->alen = (q->i+qj)<<1;
    q->buf = realloc(q->buf, q->alen);
  }
  va_start(ap, fmt);
  qj = vsnprintf(q->buf+q->i, q->alen - q->i, fmt, ap);
  va_end(ap);
  q->i += qj;
}

void InitBuf(struct strbuf *q) {
  q->alen = 1024;
  q->i = 0;
  q->buf = malloc(q->alen);
}

void FreeBuf(struct strbuf *q) {
  free(q->buf);
}

void Cpy(struct strbuf *q, const void *buf, int len) {
  if (q->i+len >= q->alen-1) {
    q->alen = (q->i+len)<<1;
    q->buf = realloc(q->buf, q->alen);
//    __android_log_print(ANDROID_LOG_INFO, "svpismo", "realloc, q->buf = %x, len=%d\n", q->buf, q->alen);
  }
  memcpy(q->buf+q->i, buf, len);
  q->i += len;
  q->buf[q->i] = 0;
}
