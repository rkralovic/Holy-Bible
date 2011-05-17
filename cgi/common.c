#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>
#include "common.h"

void Rst(struct strbuf *q) { q->i = 0; }
void Prn(struct strbuf *q, char *fmt, ...) {
  va_list ap;
  int qj;
  va_start(ap, fmt);
  qj = vsnprintf(q->buf+q->i, sizeof(q->buf)-q->i, fmt, ap);
  q->i += qj;
  va_end(ap);
}

