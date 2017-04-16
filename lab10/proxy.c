/*
 * proxy.c - CS:APP Web proxy
 *
 * TEAM MEMBERS:
 *     zhangziyang 51503091038
 * 
 * IMPORTANT: Give a high level description of your code here. You
 * must also provide a header comment at the beginning of each
 * function that describes what that function does.
 */ 

#include "csapp.h"
#define NTHREADS 8
#define SBUFSIZE 16


typedef struct{
    int fd;
    struct sockaddr_in addr;
}client_in;


// I use SBUF package

/*
 * SBUF protoypes
 */
typedef struct{
    client_in **buf;
    int n;
    int front;
    int rear;
    sem_t mutex;
    sem_t slots;
    sem_t items;
}sbuf_t;
void sbuf_init(sbuf_t *sp, int n);
void sbuf_deinit(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, client_in* client_inp);
client_in* sbuf_remove(sbuf_t *sp);


/*
 * Implement  of SBUF (copy from the textbook)
 */
void sbuf_init(sbuf_t *sp, int n){
    sp->buf = Calloc(n, sizeof(int));
    sp->n = n;
    sp->front = sp->rear = 0;
    Sem_init(&sp->mutex, 0, 1);
    Sem_init(&sp->slots, 0, n);
    Sem_init(&sp->items, 0, 0);
}

// Deinit sbuf. Free all malloced data
void sbuf_deinit(sbuf_t *sp){
    Free(sp->buf);
}

// Insert a client to sbuf
void sbuf_insert(sbuf_t *sp, client_in* item){
    P(&sp->slots);
    P(&sp->mutex);
    sp->buf[(++sp->rear)%(sp->n)] = item;
    V(&sp->mutex);
    V(&sp->items);
}

// Remove and return a client's pointer
client_in* sbuf_remove(sbuf_t *sp){
    client_in* item;
    P(&sp->items);
    P(&sp->mutex);
    item = sp->buf[(++sp->front)%(sp->n)];
    V(&sp->mutex);
    V(&sp->slots);
    return item;
}



/*
 * Function prototypes
 */
void *thread(void* vargp);
int Open_clientfd_ts(char* hostname, int port);
void doit(client_in *client_inp);
void writelog(char* log);
ssize_t Rio_readn_w(rio_t *rp, void *usrbuf, size_t n);
ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen);
ssize_t Rio_writen_w(int fd, void *usrbuf, size_t n);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
int parse_uri(char *uri, char *target_addr, char *path, int  *port);
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, int size);

/*global var*/
sem_t mutex1;   // used in Open_clientfd_ts()
sem_t mutex2;   // used in writelog()
sbuf_t sbuf;    




/* 
 * main - Main routine for the proxy program 
 */
int main(int argc, char **argv)
{

    /* Check arguments */
    if (argc != 2) {
	fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
	exit(0);
    }
    Signal(SIGPIPE, SIG_IGN);
    int listenfd, port;
    socklen_t clientlen=sizeof(struct sockaddr_in);
    pthread_t tid;
    //init two sems
    Sem_init(&mutex1, 0, 1);
    Sem_init(&mutex2, 0, 1);
    sbuf_init(&sbuf, SBUFSIZE);
    
    port=atoi(argv[1]);
    listenfd=Open_listenfd(port);

    //use prethreading techinque
    int i;
    for(i=0; i<NTHREADS; i++)
        Pthread_create(&tid, NULL, thread, NULL);

    while(1){
        client_in *client_inp = (client_in*)Malloc(sizeof(int));
        client_inp->fd = Accept(listenfd, (SA*)&client_inp->addr, &clientlen);
        sbuf_insert(&sbuf,client_inp);
    }
    exit(0);
}

void *thread(void *vargp)
{
    Pthread_detach(Pthread_self());
    while(1){
        client_in* client_inp=sbuf_remove(&sbuf);
        doit(client_inp);
    }
    return NULL;

}

/*
 * doit - process a HTTP request
 */
void doit(client_in* client_inp){
  int clientfd=client_inp->fd;
  int serverfd;
  int port;
  struct sockaddr_in clientaddr=client_inp->addr;
  //free the used block 
  Free(client_inp);
  //prepare for RIO
  char buf[MAXLINE],method[MAXLINE],uri[MAXLINE],version[MAXLINE];
  rio_t clientRio,serverRio;
  char message_to_client[MAXLINE],message_to_server[MAXLINE];
  //prepare for parse_uri
  char hostname[MAXLINE],pathname[MAXLINE];
  
  Rio_readinitb(&clientRio,clientfd);
  if((Rio_readlineb_w(&clientRio,buf,MAXLINE))<0)
    return; //failed to receive requests from clients
//spilit buf
sscanf(buf,"%s %s %s",method,uri,version);
//only support GET
if(strcasecmp(method,"GET")){
    clienterror(clientfd,method,"501","Not Implemented",
    "Proxy does not implement this method");
    return;
}

parse_uri(uri,hostname,pathname,&port);

if((serverfd = Open_clientfd_ts(hostname, port)) < 0){
    strcpy(message_to_client, "Cannot connect to server");
    Rio_writen_w(clientfd, message_to_client, strlen(message_to_client));
    return;
}
//receive message from client and send it to  the server
sprintf(message_to_server,"%s %s %s\r\n", method,pathname,version);
while(strcmp(buf,"\r\n")!=0){
    Rio_readlineb_w(&clientRio,buf,MAXLINE);
    strcat(message_to_server,buf);
}
Rio_writen_w(serverfd,message_to_server,sizeof(message_to_server));



//reveive message from server and send it to the client
Rio_readinitb(&serverRio,serverfd);
int n,count=0;             //count is used in logs
while((n=Rio_readn_w(&serverRio,buf,MAXLINE))>0){
    Rio_writen_w(clientfd,buf,n);
    count+=n;
}
Close(serverfd);
format_log_entry(buf, &clientaddr, uri, count);
writelog(buf);
Close(clientfd);
return;
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

    *port = 80; /* default */

    /* Extract the host name */
    hostbegin = uri + 7;
    hostend = strpbrk(hostbegin, " :/\r\n");
    len = hostend - hostbegin;
    if(strlen(hostbegin) < len){
        // Failed to find the characters
        strcpy(hostname, hostbegin);
        strcpy(pathname, "/");
        return 0;
    }
    strncpy(hostname, hostbegin, len);
    hostname[len] = '\0';

    /* Extract the port number */
    if (*hostend == ':')
        *port = atoi(hostend + 1);

    /* Extract the path */
    pathbegin = strchr(hostbegin, '/');
    if (pathbegin == NULL)
        strcpy(pathbegin, "/");
    else
        strcpy(pathname, pathbegin);

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
    sprintf(logstring, "%s: %d.%d.%d.%d %s %d\n", time_str, a, b, c, d, uri, size);
}

/*
 * Rewrite rio functions
 */

ssize_t Rio_readn_w(rio_t *rp, void *usrbuf, size_t n){
    ssize_t rc;
    if ((rc = rio_readnb(rp, usrbuf, n)) < 0){
        printf("Error with rio_readnb.\n");
        return -1;
    }
    return rc;
}

ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen){
    ssize_t rc;
    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0){
        printf("Error with rio_readlineb.\n");
        return -1;
    }
    return rc;
}

ssize_t Rio_writen_w(int fd, void *usrbuf, size_t n){
    ssize_t rc;
    if ((rc = rio_writen(fd, usrbuf, n)) <0){
        printf("Error with rio_writen.\n");
        return -1;
    }
    return n;
}

/*
 * Open_clientfd_ts - Open_clientfd (thread-safe mode) (using locks to prevent races)
 */

int Open_clientfd_ts(char* hostname,int port){
    P(&mutex1);
    int fd;
    while((fd=open_clientfd(hostname,port))<0)
        printf("Error with open_clientfd.\n");
    V(&mutex1);
    return fd;
}

/*
 * writelog - write logs (using locks to prevent races)
 */

void writelog(char* log){
    P(&mutex2);
    FILE* logfile = Fopen("proxy.log", "a");
    fprintf(logfile, "%s", log);
    Fclose(logfile);
    V(&mutex2);
}

/*
 * clienterror -  report errors to clients (copy from the textbook)
 */
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg){
    char buf[MAXLINE], body[MAXBUF];

    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen_w(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen_w(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen_w(fd, buf, strlen(buf));
    Rio_writen_w(fd, body, strlen(body));
}

