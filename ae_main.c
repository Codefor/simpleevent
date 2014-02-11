#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

#include "ae.h"
#include "anet.h"

#define ADDR "10.36.154.56"
#define PORT 8080

#define LOG_DEBUG 0
#define LOG_NOTICE 1
#define LOG_WARNING 2
#define LOG_FATAL 3

#define LOG_LEVEL 0

char neterr[ANET_ERR_LEN];

typedef struct{
	char rbuffer[1024];
	char wbuffer[1024];
	int nread;
	int ntowritten;
}Client;

//function statement
static int serverCron(struct aeEventLoop *el, long long id, void *clientData);
static void acceptTcpHandler(aeEventLoop *el, int fd, void *privdata, int mask);
static void acceptCommonHandler(aeEventLoop *el,int fd, int flags);
static Client *createClient(aeEventLoop *el,int fd);
static void closeClient(aeEventLoop *el, int fd,void *privdata, int mask);
static void readQueryFromClient(aeEventLoop *el, int fd, void *privdata, int mask);
static void sendReplyToClient(aeEventLoop *el, int fd, void *privdata, int mask);
static void log3(int level,const char* format,...);

static void log3(int level,const char* format,...){
    va_list ap;
    char msg[1024];

    if ((level&0xff) < LOG_LEVEL) return;

    va_start(ap, format);
    vsnprintf(msg, sizeof(msg), format, ap);
    va_end(ap);

	fprintf(stderr,msg);
}

static int serverCron(struct aeEventLoop *el, long long id, void *clientData){
	log3(LOG_NOTICE,"serverCron:%ld\n",time(NULL));
	/* the retval is delay milliseconds before next fired,N means every N ms */
	return 1000;
}

static void acceptTcpHandler(aeEventLoop *el, int fd, void *privdata, int mask) {
	int cport, cfd;
	char cip[128];

	cfd = anetTcpAccept(neterr, fd, cip, &cport);
	if (cfd == AE_ERR) {
		log3(LOG_WARNING,"Accepting client connection: %s\n", neterr);
		return;
	}
	log3(LOG_NOTICE,"Accepted %s:%d\n", cip, cport);

    acceptCommonHandler(el,cfd,0);
}

static void acceptCommonHandler(aeEventLoop *el,int fd, int flags) {
	if(createClient(el,fd) == NULL){
        log3(LOG_WARNING,"Error registering fd event for the new client: %s (fd=%d)\n",strerror(errno),fd);
		close(fd);
	}
    log3(LOG_NOTICE,"create Client: (fd=%d) ok\n",fd);
}

static Client *createClient(aeEventLoop *el,int fd) {
	if(fd <= 0) return NULL;

    Client *c = calloc(1,sizeof(Client));

	anetNonBlock(NULL,fd);
	anetEnableTcpNoDelay(NULL,fd);

	if (aeCreateFileEvent(el,fd,AE_READABLE,readQueryFromClient, c) == AE_ERR){
		close(fd);
		free(c);
		return NULL;
	}
	
	if (aeCreateFileEvent(el,fd,AE_WRITABLE,sendReplyToClient, c) == AE_ERR){
		close(fd);
		free(c);
		return NULL;
	}

	return c;
}

static void closeClient(aeEventLoop *el, int fd,void *privdata, int mask){
    Client *c = (Client*) privdata;
	free(c);

    aeDeleteFileEvent(el,fd,AE_READABLE);
    aeDeleteFileEvent(el,fd,AE_WRITABLE);
	close(fd);
    log3(LOG_NOTICE,"close client: (fd=%d) ok\n",fd);
}

static void readQueryFromClient(aeEventLoop *el, int fd, void *privdata, int mask) {
    Client *c = (Client*) privdata;
    int nread;

    nread = read(fd, c->rbuffer, sizeof(c->rbuffer));
	if (nread == -1) {
        if (errno == EAGAIN) {
            nread = 0;
        } else {
            log3(LOG_WARNING, "Reading from client: %s\n",strerror(errno));
            closeClient(el,fd,c,0);
            return;
        }
    } else if (nread == 0) {
        log3(LOG_NOTICE,"Client closed connection\n");
        closeClient(el,fd,c,0);
        return;
    }
	
	c->nread = nread;
	c->ntowritten = c->nread;
	memcpy(c->wbuffer,c->rbuffer,nread);
	log3(LOG_NOTICE,"read %d bytes from fd %d\n",nread,fd);
}

static void sendReplyToClient(aeEventLoop *el, int fd, void *privdata, int mask) {
    Client *c = (Client*) privdata;

	if(c->ntowritten <= 0){
		return;
	}

	int nwritten;

	nwritten = write(fd,c->wbuffer,c->ntowritten);
	if (nwritten == -1) {
        if (errno == EAGAIN) {
            nwritten = 0;
        } else {
            log3(LOG_WARNING,"Error writing to client: %s", strerror(errno));
            closeClient(el,fd,c,0);
            return;
        }
    }
	c->ntowritten -= nwritten;

	log3(LOG_NOTICE,"written %d bytes to fd %d\n",nwritten,fd);
}

int main(){
	aeEventLoop *el = aeCreateEventLoop(1024);
	aeCreateTimeEvent(el, 1000, serverCron, NULL, NULL);

	int ipfd;
	ipfd = anetTcpServer(neterr,PORT,ADDR);
	if (ipfd == ANET_ERR) {
		log3(LOG_FATAL,"Opening prot %d: %s",PORT,ADDR);
		exit(1);
	}

	aeCreateFileEvent(el,ipfd,AE_READABLE,acceptTcpHandler,NULL);

	/* start loop */
	aeMain(el);

    aeDeleteFileEvent(el,ipfd,AE_READABLE);
	aeDeleteEventLoop(el);
	return 0;
}
