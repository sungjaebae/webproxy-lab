/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(int argc, char **argv) {
  char *buf, *p;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
  int n1 = 0, n2 = 0;
  char *is_head_msg = argv[0];
  p = strchr(is_head_msg, '=');
  int is_head = atoi(p + 1);

  if ((buf = getenv("QUERY_STRING")) != NULL) {
    char kv1[MAXLINE], kv2[MAXLINE];
    p = strchr(buf, '&');
    *p = '\0';
    strcpy(kv1, buf);
    kv1[strlen(buf)] = '\0';

    strcpy(kv2, p + 1);
    kv2[strlen(p + 1)] = '\0';
    p = strchr(kv1, '=');
    if (p != NULL) {
      strcpy(arg1, p + 1); //=의 뒤쪽을 arg1에 복사한다
    } else {
      strcpy(arg1, kv1);
    }
    p = strchr(kv2, '=');
    if (p != NULL) {
      strcpy(arg2, p + 1); //=의 뒤쪽을 arg2에 복사한다
    } else {
      strcpy(arg2, kv2);
    }
    n1 = atoi(arg1); // n1은 arg1을 숫자로 변환
    n2 = atoi(arg2); // n2는 arg2를 숫자로 변환
  }
  int length = 0;
  length += sprintf(content + length, "Welcome to add.com: ");
  length += sprintf(content + length, "THE Internet addition portal.\r\n<p>");
  length += sprintf(content + length, "The answer is: %d, %d = %d\r\n<p>", n1,
                    n2, n1 + n2);
  length += sprintf(content + length, "Thanks for visiting!\r\n");

  printf("Connection: close\r\n");
  printf("Content-length: %d\r\n", length);
  printf("Content-type: text/html\r\n\r\n");
  if (!is_head) {
    printf("%s", content);
  }
  fflush(stdout);

  exit(0);
}
/* $end adder */
