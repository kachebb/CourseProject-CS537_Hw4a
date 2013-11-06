#include "cs537.h"
#include "request.h"
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>


//global variables: 

//lock and conditional variable
static pthread_mutex_t  m = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t   fill = PTHREAD_COND_INITIALIZER;
static pthread_cond_t   empty = PTHREAD_COND_INITIALIZER;

int bufferSize;
int *connFdBuf;//connection descriptor buffer
int fillptr = 0;
int useptr = 0;
int consumers = 1;
static int numInBuf = 0;


void do_fill(int value){
  connFdBuf[fillptr] = value;
  fillptr = (fillptr + 1)%bufferSize;
  numInBuf ++;
}

int do_get(){
  int tmp = connFdBuf[useptr];
  useptr = (useptr+1)%bufferSize;
  numInBuf --;
  return tmp;
}

// Parse the new arguments
void getargs(int *port, int *threads, int *buffers, char*schedalg, int argc, char *argv[])
{
    if (argc != 4) {
	fprintf(stderr, "%s [portnum] [threads] [buffers] [schedalg]\n", argv[0]);
	exit(1);
    }
    *port = atoi(argv[1]);
    *threads = atoi(argv[2]);
    *buffers = atoi(argv[3]);
    schedalg = argv[4];
}

void *requestHandler(void)
{
  while(1){
    printf("consumer run\n");
    pthread_mutex_lock(&m);
    while(numInBuf == 0){
      printf("consumer waiting\n");
      pthread_cond_wait(&fill,&m);
    }
    int connFd = do_get();
    printf("get %d\n", connFd);
    pthread_cond_signal(&empty);
    pthread_mutex_unlock(&m);
    requestHandle(connFd);
    Close(connFd);
  }
}


int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen;
    int threads, buffers;
    char schedalg[4];
    struct sockaddr_in clientaddr;
    

    getargs(&port,&threads, &buffers,schedalg, argc, argv);
    //initialize global variables 
    connFdBuf = (int*)malloc(buffers*sizeof(int));
    bufferSize = buffers;
    useptr = 0;
    fillptr = 0;
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

      //this operation should be unlock!!!!
      connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
      
      pthread_mutex_lock(&m);
      do_fill(connfd);
      //  printf("num after fill in buffer:%d\n", numInBuf);
      //  printf("finish fill:%d\n", connfd);
      pthread_cond_signal(&fill); 
      pthread_mutex_unlock(&m);
    }

}


    


 
