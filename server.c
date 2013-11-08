#include "cs537.h"
#include "request.h"
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include<string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
//global variables: 

//lock and conditional variable
static pthread_mutex_t  m = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t   fill = PTHREAD_COND_INITIALIZER;
static pthread_cond_t   empty = PTHREAD_COND_INITIALIZER;

/*
typedef struct {
  int fd;
  rio_t rio;
}reqBuf_t;
*/

int bufferSize;
int *connFdBuf;//connection descriptor buffer
rio_t *rioBuf;
//int fillptr = 0;
//reqBuf_t *reqBuf;

int useptr = 0;
int consumers = 1;
int numInBuf = 0;
char *sched;


typedef struct {
  int fd;
  char uri[MAXLINE];
  char method[MAXLINE];
  char version[MAXLINE];
} fdInfo;

fdInfo* fdInfoBuf;

// Parse the new arguments
void getargs(int *port, int *threads, int *buffers, char* schedalg, int argc, char *argv[])
{
    if (argc != 5) {
	fprintf(stderr, "%s [portnum] [threads] [buffers] [schedalg]\n", argv[0]);
	exit(1);
    }
    *port = atoi(argv[1]);
    *threads = atoi(argv[2]);
    *buffers = atoi(argv[3]);   
    strcpy(schedalg, argv[4]);    
}

/*
int findSmallName(){
  int i;
  rio_t * rio = (rio_t*)malloc(sizeof(rio_t));
  int index = 0;
  char uri[MAXLINE], smallest[MAXLINE], version[MAXLINE],method[MAXLINE], buf[MAXLINE];
  for(i=0; i < numInBuf; i++){
    //copy a rio
    rio->rio_fd = rioBuf[0].rio_fd;
    rio->rio_cnt = rioBuf[0].rio_cnt;
    rio->rio_bufptr = rioBuf[0].rio_bufptr;
    for(i=0;i<RIO_BUFSIZE; i++){
      rio->rio_buf[i] = rioBuf[0].rio_buf[i];
    }
    //find smallest name
    Rio_readlineb(rio, buf, MAXLINE);
    sscanf(buf, "%s %s %s", method, uri, version);
    if(i==0){
      strcpy(smallest,uri);
    }
    else if(strcmp(smallest, uri) > 0){
      strcpy(smallest,uri);
      index = i;
    }
    printf("%s %s %s\n", method, uri, version);
  }
  return index;
  }*/

int findSmallSize(){
  int i;
  off_t smallsize;
  struct stat filestat;
  int index = 0;
  for(i=0; i < numInBuf; i++){
    //find smallest name
    if(stat(fdInfoBuf[i].uri, &filestat) < 0){
      return -1;
    }
    if(i==0){
      smallsize = filestat.st_size;
    }
    else if(smallsize > filestat.st_size){
      smallsize = filestat.st_size;
      index = i;
    }
    // printf("%s %s %s\n", method, uri, version);
  }
  return index;

}



int findSmallName(){
  int i;
  char smallest[MAXLINE];// = (rio_t*)malloc(sizeof(rio_t));
  int index = 0;
  for(i=0; i < numInBuf; i++){
    //find smallest name
    if(i==0){
      strcpy(smallest,fdInfoBuf[i].uri);
    }
    else if(strcmp(smallest, fdInfoBuf[i].uri) > 0){
      strcpy(smallest,fdInfoBuf[i].uri);
      index = i;
    }
    // printf("%s %s %s\n", method, uri, version);
  }
  return index;
}



void do_fill(int value){
  //reqBuf[numInBuf].fd = value;
  // Rio_readinitb(&rioBuf[numInBuf], value);
  rio_t rio;
  char buf[MAXLINE];
  // printf("in fill %d\n", value);
  Rio_readinitb(&rio, value);
  Rio_readlineb(&rio, buf, MAXLINE);
  //printf("after rio\n");
  fdInfoBuf[numInBuf].fd = value;
  sscanf(buf, "%s %s %s", fdInfoBuf[numInBuf].method, fdInfoBuf[numInBuf].uri, fdInfoBuf[numInBuf].version);
  printf("%s %s %s\n", fdInfoBuf[numInBuf].method, fdInfoBuf[numInBuf].uri, fdInfoBuf[numInBuf].version);
  numInBuf ++;
  // printf("numInbuf:%d",numInBuf);
}

/*
rio_t* get(int index){
  printf("get index%d\n", index);
  rio_t* rio = (rio_t *)malloc(sizeof(rio_t));
  int i,j;
  rio->rio_fd = rioBuf[index].rio_fd;
  rio->rio_cnt = rioBuf[index].rio_cnt;
  rio->rio_bufptr = rioBuf[index].rio_bufptr;
  for(i=0;i<RIO_BUFSIZE; i++){
    rio->rio_buf[i] = rioBuf[index].rio_buf[i];
  }
  printf("finish get\n");
  //get one data out, move the rest forward
  for(j=index; j< numInBuf-1; j++){
    rioBuf[j] = rioBuf[j+1];
  }
  printf("minus:%d\n",numInBuf);
  numInBuf --;
  return rio;
  }*/

fdInfo* get(int index){
  printf("get index%d\n", index);
  // return fdInfoBuf[index];
  fdInfo* ret = (fdInfo *)malloc(sizeof(fdInfo));
  int i;
  ret->fd = fdInfoBuf[index].fd;
  strcpy(ret->uri, fdInfoBuf[index].uri);
  strcpy(ret->method , fdInfoBuf[index].method);
  strcpy(ret->version , fdInfoBuf[index].version);
  //get one data out, move the other forward
  for(i = index; i< numInBuf-1; i++){
    fdInfoBuf[i] = fdInfoBuf[i+1];
  }
  numInBuf --;
  return ret;
}
/*
rio_t* do_get(){
  printf("current sched: %s\n", sched);
  // reqBuf_t *ret = (reqBuf_t* )malloc(sizeof(reqBuf_t));
  int i,j;
  if(!strcmp(sched, "FIFO")){
    printf("in fifo\n");
    return get(0);
  }
  else if (!strcmp(sched, "SFNF")){
    printf("in SFNF\n");
    int smallindex = findSmallName();
    return get(smallindex);
  }
  else if (!strcmp(sched, "SFF")){
    printf("SFF\n");
  }
  // else return ;
  }*/

fdInfo* do_get(){
  printf("current sched: %s\n", sched);
  // reqBuf_t *ret = (reqBuf_t* )malloc(sizeof(reqBuf_t));
  int index;
  if(!strcmp(sched, "FIFO")){
    printf("in fifo\n");
    return get(0);
  }
  else if (!strcmp(sched, "SFNF")){
    // printf("in SFNF\n");
    index = findSmallName();
    return get(index);
  }
  else if (!strcmp(sched, "SFF")){
    printf("in SFF\n");
    index = findSmallSize();
    return get(index);
  }
  // else return ;
}


//consumer thread
void *requestHandler(void)
{
  while(1){
    printf("consumer run\n");
    pthread_mutex_lock(&m);
    while(numInBuf == 0){
      printf("consumer waiting\n");
      pthread_cond_wait(&fill,&m);
    }
    fdInfo *fdinfo;
    fdinfo = do_get();
    printf("request file %s\n", fdinfo->uri);
    pthread_cond_signal(&empty);
    pthread_mutex_unlock(&m);
    requestHandle(fdinfo->fd, fdinfo->method, fdinfo->uri, fdinfo->version);
    Close(fdinfo->fd);
  }
}


int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen;
    int threads, buffers;
    struct sockaddr_in clientaddr;
    
    sched = (char*) malloc(5*sizeof(char));
    getargs(&port,&threads, &buffers,sched, argc, argv);
    //initialize global variables 
    //  reqBuf = (reqBuf_t *)malloc(buffers*sizeof(reqBuf_t));
    // rioBuf = (rio_t *)malloc(buffers*sizeof(rio_t));
    fdInfoBuf = (fdInfo *)malloc(buffers*sizeof(fdInfo));
    bufferSize = buffers;
    numInBuf = 0;
    consumers = threads;

    //create threads
    pthread_t handlerC[consumers];
    int i;
    for(i=0;i<consumers;i++){
      pthread_create(&handlerC[i], NULL, requestHandler, NULL);
    }
    
    printf("consumers join\n");

    listenfd = Open_listenfd(port);
    while (1) {
      pthread_mutex_lock(&m);
      printf("num in buffer:%d\n", numInBuf);
      while(numInBuf == bufferSize){
	printf("producer waiting\n");
	pthread_cond_wait(&empty, &m);
      }
      clientlen = sizeof(clientaddr);
      pthread_mutex_unlock(&m);
      printf("listenning\n");
      //this operation should be unlock!!!!
      connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
      pthread_mutex_lock(&m);
      do_fill(connfd);
      // printf("num after fill in buffer:%d\n", numInBuf);
      pthread_cond_signal(&fill); 
      pthread_mutex_unlock(&m);
    }

}
