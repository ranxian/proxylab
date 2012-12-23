/*
 * proxy.c - CS:APP Web proxy
 *
 * TEAM MEMBERS:
 *     Andrew Carnegie, ac00@cs.cmu.edu 
 *     Harry Q. Bovik, bovik@cs.cmu.edu
 * 
 * IMPORTANT: Give a high level description of your code here. You
 * must also provide a header comment at the beginning of each
 * function that describes what that function does.
 */ 

#include "csapp.h"
#include "cache.h"
 
/*
 * Consts
 */
const char* user_agent = "Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3)Gecko/20120305 Firefox/10.0.3";
const char* accept_str = "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8";
const char* accept_encoding = "gzip, deflate";

#define MAX_HDR_LEN 1024
pthread_rwlock_t rwlock;
/*
 * Function prototypes
 */
int parse_uri(char *uri, char *target_addr, char *path, int  *port);
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, int size);
void serve(int fd);
void clienterror(int fd, char *msg);
void build_requesthdrs(rio_t *rp, char *hdrstr, char *hostname);
void *thread(void *argvp);

void server_log(struct sockaddr_in *sockaddr) {
    char msg[MAXLINE];
    
    format_log_entry(msg, sockaddr, NULL, 0);

    printf("%s\n", msg);
}

/* 
 * main - Main routine for the proxy program 
 */
int main(int argc, char **argv)
{
    int port, listenfd, *connfdp;
    unsigned clientlen;
    struct sockaddr_in clientaddr;
    pthread_t tid;
    /* Ignore SIGPIPE */
    Signal(SIGPIPE, SIG_IGN);

    /* Check arguments */
    if (argc != 2) {
	   fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
	   exit(0);
    }

    port = atoi(argv[1]);

    listenfd = Open_listenfd(port);

    cache_init();
    // pthread_rwlock_init(&rwlock, NULL);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfdp = Malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        server_log(&clientaddr);

        Pthread_create(&tid, NULL, thread, (void *)connfdp);
    }
    // pthread_rwlock_destroy(&rwlock);
    exit(0);
}

/*
 * serve - the proxy server serves the request after connection 
 *         with the cliend built.
 */
void serve(int fd) {
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char hdrstr[MAX_HDR_LEN] = {}, hostname[MAXLINE], pathname[MAXLINE];
    int clientfd, port, readcnt;
    char request[MAXLINE];
    CE *entry;
    rio_t rio;
  
    /* Read request line and headers */
    Rio_readinitb(&rio, fd);
    Rio_readlineb(&rio, buf, MAXLINE);                 

    strcpy(request, buf);
    if ((entry = is_in_cache(request)) != NULL) {
        Rio_writen(fd, entry->content, entry->content_size);
        return;
    }


    sscanf(buf, "%s %s %s", method, uri, version);       
    if (strcasecmp(method, "GET")) {                     
        clienterror(fd, "Sorry, prxoy server servers only GET method now.");
        return;
    }         
    parse_uri(uri, hostname, pathname, &port);
    // printf("Uri parsed result: hostname: %s, pathname: %s, port: %d\n", hostname, pathname, port);

    build_requesthdrs(&rio, hdrstr, hostname);

    /* Following will send request to the `real' server, ignore any error */
    if ((clientfd = Open_clientfd(hostname, port)) < 0) {
        return;
    }
    sprintf(buf, "%s /%s HTTP/1.0\r\n", method, pathname);
    // printf("%s", buf);
    Rio_writen(clientfd, buf, strlen(buf));
    Rio_writen(clientfd, hdrstr, strlen(hdrstr));
    // printf("%s", hdrstr);


    char cache_buf[MAX_OBJECT_SIZE];
    int cnt = 0, cached = 1;
    while ((readcnt = Rio_readn(clientfd, buf, MAXLINE)) > 0) {
        if (cnt + readcnt <= MAX_OBJECT_SIZE) {
            memcpy(cache_buf + cnt, buf, readcnt);
            cnt += readcnt;
        } else {
            cached = 0;
        }
        Rio_writen(fd, buf, readcnt);
    }
    if (cached) {
        add_cache_entry(request, cache_buf, cnt);
    }
    Close(clientfd);
}

void build_requesthdrs(rio_t *rp, char *hdrstr, char *hostname)
{
    char buf[MAXLINE];
    int provided_host = 0;

    sprintf(hdrstr, "User-Agent: %s\r\n", user_agent);
    sprintf(hdrstr, "%sAccept: %s\r\n", hdrstr, accept_str);
    sprintf(hdrstr, "%sAccept-Encoding: %s\r\n", hdrstr, accept_encoding);
    sprintf(hdrstr, "%sConnection: %s\r\n", hdrstr, "close");
    sprintf(hdrstr, "%sProxy-Connection: %s\r\n", hdrstr, "close");
    
    Rio_readlineb(rp, buf, MAXLINE);
    while (strcmp(buf, "\r\n")) {
        if (strstr(buf, "User-Agent") ||
            strstr(buf, "Accept") ||
            strstr(buf, "Accept-Encoding") ||
            strstr(buf, "Connection") ||
            strstr(buf, "Proxy-Connection"))
        {
            /* Ignore them */
        } else {
            if (strstr(buf, "Host")) {
                provided_host = 1;
            }
            strcat(hdrstr, buf);
        }
        Rio_readlineb(rp, buf, MAXLINE);
    };
    if (!provided_host) {
        sprintf(hdrstr, "%sHost: %s\r\n", hdrstr, hostname);
    }

    strcat(hdrstr, "\r\n");
    return;
}

/*
 * thread - thread routine
 */
void *thread(void *vargp)
{
    int connfd = *((int *)vargp);
    Pthread_detach(pthread_self());
    Free(vargp);

    serve(connfd);
    Close(connfd);

    return NULL;
}

void clienterror(int fd, char *msg) {
    char buf[MAXLINE] = {};

    sprintf(buf, "%s\n", msg);
    Rio_writen(fd, buf, strlen(buf));
}

/*
 * parse_uri - URI parser
 * 
 * Given a URI from an HTTP proxy GET request (i.e., a URL), extract
 * the host name, path name, and port.  The memory for hostname and
 * pathname must already be allocated and should be at least MAXLINE
 * bytes. Return -1 if there are any problems.
 */
int parse_uri(char *uri, char *hostname, char *pathname, int *port)
{
    char *hostbegin;
    char *hostend;
    char *pathbegin;
    int len;

    if (strncasecmp(uri, "http://", 7) != 0) {
	hostname[0] = '\0';
	return -1;
    }
       
    /* Extract the host name */
    hostbegin = uri + 7;
    hostend = strpbrk(hostbegin, " :/\r\n\0");
    len = hostend - hostbegin;
    strncpy(hostname, hostbegin, len);
    hostname[len] = '\0';
    
    /* Extract the port number */
    *port = 80; /* default */
    if (*hostend == ':')   
	*port = atoi(hostend + 1);
    
    /* Extract the path */
    pathbegin = strchr(hostbegin, '/');
    if (pathbegin == NULL) {
	pathname[0] = '\0';
    }
    else {
	pathbegin++;	
	strcpy(pathname, pathbegin);
    }

    return 0;
}

/*
 * format_log_entry - Create a formatted log entry in logstring. 
 * 
 * The inputs are the socket address of the requesting client
 * (sockaddr), the URI from the request (uri), and the size in bytes
 * of the response from the server (size).
 */
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, 
		      char *uri, int size)
{
    time_t now;
    char time_str[MAXLINE];
    unsigned long host;
    unsigned char a, b, c, d;

    /* Get a formatted time string */
    now = time(NULL);
    strftime(time_str, MAXLINE, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));

    /* 
     * Convert the IP address in network byte order to dotted decimal
     * form. Note that we could have used inet_ntoa, but chose not to
     * because inet_ntoa is a Class 3 thread unsafe function that
     * returns a pointer to a static variable (Ch 13, CS:APP).
     */
    host = ntohl(sockaddr->sin_addr.s_addr);
    a = host >> 24;
    b = (host >> 16) & 0xff;
    c = (host >> 8) & 0xff;
    d = host & 0xff;


    /* Return the formatted log entry string */
    sprintf(logstring, "%s: %d.%d.%d.%d %s", time_str, a, b, c, d, uri);
}


