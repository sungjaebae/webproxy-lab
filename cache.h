#ifndef __CACHE_H__
#define __CACHE_H__

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define MAXLINE 8192
typedef struct cache_t {
  char request_line[MAXLINE];
  char response_line[MAXLINE];
  char headers_buf[MAXLINE];
  char body_buf[MAX_OBJECT_SIZE];
  int header_length;
  int content_length;
} cache_t;

void init_caches();
void add_cache(char *request_line, char response_line[], char headers_buf[],
               char body_buf[], int header_length, int content_length);
void try_get_cache(char *request_line, char response_line[], char headers_buf[],
                   char **body_buf, int *header_length, int *content_length);

#endif /* __CACHE_H__ */