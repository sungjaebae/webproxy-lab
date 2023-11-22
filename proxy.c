#include "sock.h"
#include <stdbool.h>
#include <stdio.h>

void *thread_proxy(void *arg);
void proxy(int clientfd);
void read_request_headers(rio_t *rp, char *buf[]);
void parse_url(char url[], char scheme[], char hostname[], char port[],
               char path[]);
int make_request_start_line(char buf[], char method[], char path[]);
int make_reqeust_headers(char endserver_buf[], int endserver_buf_length,
                         char *client_buf[], char hostname[]);
int add_empty_line(char buf[], int buf_length);
int http_send(char request_buf[], int requst_buf_length,
              char server_response_start_line[],
              char **server_response_header_buf, char **end_server_response_buf,
              char *host, char *port);
int read_response_headers(char *buf[], rio_t *rp);
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  pthread_t tid;

  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (sockaddr *)&clientaddr, &clientlen);
    Getnameinfo((sockaddr *)&clientaddr, clientlen, hostname, MAXLINE, port,
                MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    void *arg = Malloc(sizeof(int));
    memcpy(arg, &connfd, sizeof(int));
    Pthread_create(&tid, NULL, thread_proxy, arg);
  }
  return 0;
}
void *thread_proxy(void *arg) {
  int connfd = *(int *)arg;
  Pthread_detach(Pthread_self());
  proxy(connfd);
  free(arg);
  close(connfd);
  return NULL;
}

void proxy(int connfd) {
  // 클라이언트의 요청을 파싱하기 위한 변수
  rio_t rio;
  char request_line[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char *client_request_headers_buf[MAXLINE] = {
      NULL,
  };

  // 서버에 요청을 보내기 위한 변수
  int header_length = 0;
  char end_server_request_buf[8 * 1024];
  char scheme[MAXLINE], hostname[MAXLINE], path[MAXLINE], port[MAXLINE];

  // 서버의 응답을 받기 위한 변수
  char *end_server_response_buf = NULL;
  char server_response_start_line[MAXLINE];
  char *server_response_header_buf[MAXLINE] = {
      NULL,
  };
  int content_length = 0;

  // 클라이언트에 응답을 보내기 위한 변수
  char client_response_header_buf[8 * 1024];

  Rio_readinitb(&rio, connfd);
  // 클라이언트 요청의 스타트 라인를 읽는다
  Rio_readlineb(&rio, request_line, MAXLINE);
  sscanf(request_line, "%s %s %s", method, uri, version);
  // 클라이언트 요청의 헤더를 읽는다
  read_request_headers(&rio, client_request_headers_buf);

  // 서버로 보낼 헤더를 만든다
  parse_url(uri, scheme, hostname, port, path);
  header_length = make_request_start_line(end_server_request_buf, method, path);
  header_length = make_reqeust_headers(end_server_request_buf, header_length,
                                       client_request_headers_buf, hostname);

  header_length = add_empty_line(end_server_request_buf, header_length);
  if ((content_length = http_send(
           end_server_request_buf, header_length, server_response_start_line,
           server_response_header_buf, &end_server_response_buf, hostname,
           port)) == -2) { // 전송 실패. 엔드 서버가 꺼져있을 것이다
    return;
  }
  // 클라이언트에 응답 스타트 라인을 보낸다
  Rio_writen(connfd, server_response_start_line,
             strlen(server_response_start_line));
  // 클라이언트에 응답 헤더를 보낸다
  header_length = 0;
  for (int i = 0; server_response_header_buf[i] != NULL; i++) {
    header_length += sprintf(client_response_header_buf + header_length, "%s",
                             server_response_header_buf[i]);
    free(server_response_header_buf[i]);
  }
  header_length = add_empty_line(client_response_header_buf, header_length);
  Rio_writen(connfd, client_response_header_buf, header_length);
  // 클라이언트에 바디를 보낸다
  if (content_length != -1) {
    Rio_writen(connfd, end_server_response_buf, content_length);
  }
}

void read_request_headers(rio_t *rp, char *buf[]) {
  char temp[MAXLINE];
  int i = 0;
  ssize_t size = 0;
  while ((size = Rio_readlineb(rp, temp, MAXLINE)) !=
         2) { // 빈줄을 읽을 때까지 계속 읽는다
    printf("%s", temp);
    buf[i] = Malloc(sizeof(char) * (size + 1));
    strcpy(buf[i], temp);
    buf[i][size] = '\0';
    i++;
  }

  return;
}

void parse_url(char url[], char scheme[], char hostname[], char port[],
               char path[]) {
  char temp[MAXLINE];
  strcpy(temp, url);
  char *p;
  // scheme이 제대로 존재하는가? 없으면 만들어준다
  if ((p = strstr(temp, "://")) == NULL) {
    printf("[parse_url]: no sheme, make sheme http\n");
    strcpy(scheme, "http");
  } else {
    *p = '\0';
    strcpy(scheme, temp);
    p += 3;
    sprintf(url, "%s", p);
  }
  strcpy(temp, url);
  // path가 제대로 존재하는가? 없으면 /를 붙여준다
  if ((p = strchr(temp, '/')) == NULL) {
    printf("[parse_url]: no path, make path /\n");
    strcpy(path, "/");
  } else {
    *p = '\0';
    strcpy(url, temp);
    strcpy(path, "/");
    strcat(path, p + 1);
  }
  strcpy(temp, url);
  // port가 제대로 존재하는가? 없으면 80을 만들어준다
  if ((p = strchr(temp, ':')) == NULL) {
    printf("[parse_url]: no port, make port 80\n");
    strcpy(hostname, temp);
    strcpy(port, "80");
  } else {
    *p = '\0';
    strcpy(hostname, temp);
    strcpy(port, p + 1);
  }
  sprintf(url, "%s://%s:%s%s", scheme, hostname, port, path);
}

int make_request_start_line(char buf[], char method[], char path[]) {
  return sprintf(buf, "%s %s HTTP/1.0\r\n", method, path);
}

int make_reqeust_headers(char endserver_buf[], int endserver_buf_length,
                         char *client_buf[], char hostname[]) {
  int length = endserver_buf_length;
  char *p;
  bool is_host_processed = false;
  bool is_user_agent_processed = false;
  bool is_connection_processed = false;
  bool is_proxy_connection_processed = false;
  for (int i = 0; client_buf[i] != NULL; i++) {
    printf("client_buf[%d]=%s", i, client_buf[i]);
    if ((p = strstr(client_buf[i], "Host:")) != NULL) {
      printf("Host header already exists, override it\n");
      length += sprintf(endserver_buf + length, "Host: %s\r\n", hostname);
      is_host_processed = true;
    } else if ((p = strstr(client_buf[i], "User-Agent:")) != NULL) {
      printf("User-Agent already exists, override it\n");
      length += sprintf(endserver_buf + length, "%s", user_agent_hdr);
      is_user_agent_processed = true;
    } else if ((p = strstr(client_buf[i], "Proxy-Connection:")) != NULL) {
      printf("Proxy-Connection already exists, override it\n");
      length += sprintf(endserver_buf + length, "Proxy-Connection: close\r\n");
      is_proxy_connection_processed = true;
    } else if ((p = strstr(client_buf[i], "Connection:")) != NULL) {
      printf("Connection already exists, override it\n");
      length += sprintf(endserver_buf + length, "Connection: close\r\n");
      is_connection_processed = true;
    } else {
      length += sprintf(endserver_buf + length, "%s", client_buf[i]);
    }
    free(client_buf[i]);
  }
  if (!is_host_processed) {
    printf("Host is not added to header. append it\n");
    length += sprintf(endserver_buf + length, "Host: %s\r\n", hostname);
    is_host_processed = true;
  }
  if (!is_user_agent_processed) {
    printf("User Agent is not added to header. append it\n");
    length += sprintf(endserver_buf + length, "%s", user_agent_hdr);
    is_user_agent_processed = true;
  }
  if (!is_connection_processed) {
    printf("Connection is not added to header. append it\n");
    length += sprintf(endserver_buf + length, "Connection: close\r\n");
    is_connection_processed = true;
  }
  if (!is_proxy_connection_processed) {
    printf("Proxy Connection is not added to header. append it\n");
    length += sprintf(endserver_buf + length, "Proxy-Connection: close\r\n");
    is_proxy_connection_processed = true;
  }
  return length;
}

int add_empty_line(char buf[], int buf_length) {
  int length = buf_length;
  length += sprintf(buf + length, "\r\n");
  return length;
}

int http_send(char request_buf[], int requst_buf_length,
              char server_response_start_line[],
              char **server_response_header_buf, char **end_server_response_buf,
              char *host, char *port) {
  int clientfd;
  rio_t rio;

  if ((clientfd = open_clientfd(host, port)) == -1) {
    fprintf(stderr, "open_clientfd error: %s\n", strerror(errno));
    return -2;
  }
  Rio_readinitb(&rio, clientfd);

  Rio_writen(clientfd, request_buf, requst_buf_length);

  Rio_readlineb(&rio, server_response_start_line,
                MAXLINE); // 응답 스타트 라인 읽음

  int content_length = read_response_headers(server_response_header_buf, &rio);
  for (int i = 0; server_response_header_buf[i] != NULL; i++) {
    printf("server_response_header_buf[%d]=%s", i,
           server_response_header_buf[i]);
  }
  if (content_length != -1) {
    *end_server_response_buf = Malloc(sizeof(char) * content_length);

    rio_readnb(&rio, *end_server_response_buf, content_length);
    Rio_writen(STDOUT_FILENO, *end_server_response_buf, content_length);
  }
  Close(clientfd);
  return content_length;
}

int read_response_headers(char *buf[], rio_t *rp) {
  char temp[MAXLINE];
  int i = 0, size = 0;
  char *p;
  int content_length = -1;
  while ((size = Rio_readlineb(rp, temp, MAXLINE)) != 2) {
    printf("read_response_headers[%d]=%s", i, temp);
    buf[i] = Malloc(sizeof(char) * (size + 1));
    strcpy(buf[i], temp);

    if ((p = strstr(buf[i], "Content-length:")) != NULL ||
        (p = strstr(buf[i], "Content-Length:")) != NULL) {
      content_length = atoi(buf[i] + 15);
    }
    i++;
  }
  return content_length;
}