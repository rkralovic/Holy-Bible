#define SIGNATURE 0xADDE1610
#include <stdint.h>

#define TABLES 16  // malo by byt zhruba log_2(pocet zaznamov)

struct header {
  uint32_t signature;
  int32_t n_knihy, knihy;
  int32_t n_text, text;
  int32_t n_ppc, ppc;
  int32_t n_kalendar, kalendar;
  int32_t n_uvod, uvod;
  int32_t n_item[TABLES], item[TABLES];
};

struct kniha {
  int32_t meno, alias, n_hlav, hlava;
  int32_t min, max;
};

struct uvod {
  int32_t kniha;
  int32_t text;
};

struct hlava {
  int32_t n_versov, vers;
  int32_t min, max;
};

struct vers {
  int32_t min, max;
};

struct text {
  int32_t k, h, v;
  int32_t t;
};

struct item {
  int32_t b, e, id, comment;
};

struct kalendar {
  int32_t y, m, d;
  int32_t zalm, text;
};

extern void *db;
extern int db_len;
