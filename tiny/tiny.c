/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize, int is_head);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs, int is_head);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg, int is_head);

int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen); // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);  // line:netp:tiny:doit
    Close(connfd); // line:netp:tiny:close
  }
}

void doit(int fd) {
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  // 리퀘스트 라인를 읽는다
  rio_readinitb(&rio, fd);
  rio_readlineb(&rio, buf, MAXLINE);
  printf("Request lines:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version);
  int is_head = !strcasecmp(method, "HEAD");

  if (strcasecmp(method, "GET") && strcasecmp(method, "HEAD")) {
    clienterror(fd, method, "501", "Not implemented",
                "Tiny does not implement this method", is_head);
    return;
  }
  // 리퀘스트 라인를 읽는다
  printf("Request headers:\n");
  read_requesthdrs(&rio);

  // GET, HEAD 요청의 URI를 읽는다
  is_static = parse_uri(uri, filename, cgiargs);
  if (stat(filename, &sbuf) < 0) {
    clienterror(fd, filename, "404", "Not found",
                "Tiny couldn't find this file", is_head);
    return;
  }

  if (is_static) {
    // regular 파일이 아니면 에러, 사용자가 읽을 수 없으면 에러
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn't read the file", is_head);
      return;
    }
    serve_static(fd, filename, sbuf.st_size, is_head);
  } else { // 동적 컨텐츠이다
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn't run the CGI program", is_head);
      return;
    }
    serve_dynamic(fd, filename, cgiargs, is_head);
  }
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg, int is_head) {
  char buf[MAXLINE], body[MAXBUF];
  int length = 0;
  length += sprintf(body + length, "<html><title>Tiny Error</title>");
  length += sprintf(body + length, "<body bgcolor="
                                   "ffffff"
                                   ">\r\n");
  length += sprintf(body + length, "%s: %s\r\n", errnum, shortmsg);
  length += sprintf(body + length, "<p>%s: %s</p>\r\n", longmsg, cause);
  length += sprintf(body + length,
                    "<hr><em>The Tiny Web server</em></body></html>\r\n");

  // 리스폰스, 헤더 전송
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  // 바디 전송
  if (!is_head)
    Rio_writen(fd, body, strlen(body));
}

void read_requesthdrs(rio_t *rp) {
  char buf[MAXLINE];

  while (strcmp(buf, "\r\n")) { // 빈줄을 읽을 때까지 계속 읽는다
    rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

int parse_uri(char *uri, char *filename, char *cgiargs) {
  char *ptr;
  if (!strstr(uri, "cgi-bin")) { // 정적 컨텐츠
    strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    if (uri[strlen(uri) - 1] == '/')
      strcat(filename, "home.html");
    return 1;
  }
  // 동적 컨텐츠
  ptr = index(uri, '?');
  if (ptr) {
    strcpy(cgiargs, ptr + 1); // ?다음부터 전부 복사한다.
    *ptr = '\0';              //  ?자리에 널문자 넣어주기
  } else { // ?가 없다면 마치 정적 컨텐츠처럼 동작하는데?
    strcpy(cgiargs, "");
  }
  strcpy(filename, ".");
  strcat(filename, uri);
  return 0;
}

void serve_static(int fd, char *filename, int filesize, int is_head) {
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  get_filetype(filename, filetype);
  int length = 0;
  length += sprintf(buf + length, "HTTP/1.0 200 OK\r\n");
  length += sprintf(buf + length, "Server: Tiny Web Server\r\n");
  length += sprintf(buf + length, "Connection: close\r\n");
  length += sprintf(buf + length, "Content-length: %d\r\n", filesize);
  length += sprintf(buf + length, "Content-type: %s\r\n\r\n", filetype);
  Rio_writen(fd, buf,
             strlen(buf)); // strlen으로 얼마나 보내야하는지 알아낼 수 있다
  printf("Response headers:\n%s", buf);

  // 진짜 파일 내용 복사. mmap으로 하는데 이해 못함
  if (!is_head) {
    srcfd = Open(filename, O_RDONLY, 0);
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    Close(srcfd);
    Rio_writen(fd, srcp, filesize);
    Munmap(srcp, filesize);
  }
  // 숙제 11.9
  srcfd = Open(filename, O_RDONLY, 0);
  char *body = (char *)malloc(sizeof(char) * filesize);
  Rio_readn(srcfd, body, filesize);
  Close(srcfd);
  Rio_writen(fd, body, filesize);
  free(body);
}

void get_filetype(char *filename, char *filetype) {
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else if (strstr(filename, ".mpg") || strstr(filename, ".mpeg"))
    strcpy(filetype, "video/mpeg");
  else if (strstr(filename, ".mp4"))
    strcpy(filetype, "video/mp4");
  else
    strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs, int is_head) {
  char is_head_msg[MAXLINE];
  sprintf(is_head_msg, "is_head=%d", is_head);
  char buf[MAXLINE], *argv[] = {is_head_msg, (char *)0};

  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if (Fork() == 0) {
    setenv("QUERY_STRING", cgiargs, 1);
    Dup2(fd, STDOUT_FILENO);
    Execve(filename, argv, environ);
  }
  Wait(NULL);
}
