#include "cache.h"
#include <string.h>

static cache_t caches[11];
static int turn = 0;
void init_caches() { memset(caches, 0, sizeof(cache_t) * 11); }

void add_cache(char *request_line, char response_line[], char headers_buf[],
               char body_buf[], int header_length, int content_length) {
  if (content_length > MAX_OBJECT_SIZE) {
    return;
  }
  for (int i = 0; i < 11; i++) {
    if (strcmp(caches[i].request_line, "") == 0) {
      strcpy(caches[i].request_line, request_line);
      strcpy(caches[i].response_line, response_line);
      strcpy(caches[i].headers_buf, headers_buf);
      caches[i].header_length = header_length;
      caches[i].content_length = content_length;
      memcpy(caches[i].body_buf, body_buf, content_length);
      return;
    }
  }
  strcpy(caches[turn].request_line, request_line);
  strcpy(caches[turn].response_line, response_line);
  strcpy(caches[turn].headers_buf, headers_buf);
  caches[turn].header_length = header_length;
  caches[turn].content_length = content_length;
  memcpy(caches[turn].body_buf, body_buf, content_length);
  turn++;
  turn %= 11;
}
void try_get_cache(char *request_line, char response_line[], char headers_buf[],
                   char **body_buf, int *header_length, int *content_length) {
  for (int i = 0; i < 11; i++) {
    if (strcmp(request_line, caches[i].request_line) == 0) {
      response_line = caches[i].response_line;
      headers_buf = caches[i].headers_buf;
      *body_buf = caches[i].body_buf;
      *header_length = caches[i].header_length;
      *content_length = caches[i].content_length;
      break;
    }
  }
}