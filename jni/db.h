int db_init();
void db_close();
char *check_book(char *s);
void init_search(char *b);
void free_search();
void add_search(int hb, int vb, int he, int ve);
void do_search();
int get_result(int *c, char **s);
int get_citania(int y, int m, int d, char **zt, char **ct);
void get_prev(char *b, int h, char **ob, int *oh); 
void get_next(char *b, int h, char **ob, int *oh);
void get_first(char **ob, int *oh);
void fulltext_search(char *s); 
void free_fulltext_search();
int get_fulltext_result(char **b, int *h, int *v, char **t);

// Do not take ownership
const char* get_uvod_kniha(int id);
const char* get_uvod(int id);
int get_uvod_pre_knihu(const char* b);
