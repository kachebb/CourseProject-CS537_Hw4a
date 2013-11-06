#include "cs537.h"
#include "request.h"
#include <stdlib.h>
// 
// server.c: A very, very simple web server
//
// To run:
//  server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

// CS537: Parse the new arguments too
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


int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen;
    int threads, buffers;
    char schedalg[4];
    struct sockaddr_in clientaddr;
 

    getargs(&port,&threads, &buffers,schedalg, argc, argv);
    
    int *conDescriptor = (int*)malloc(buffers*sizeof(int));
    // 
    // CS537: Create some threads...
    //

    listenfd = Open_listenfd(port);
    while (1) {
	clientlen = sizeof(clientaddr);
	connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);

	// 
	// CS537: In general, don't handle the request in the main thread.
	// Save the relevant info in a buffer and have one of the worker threads 
	// do the work. However, for SFF, you may have to do a little work
	// here (e.g., a stat() on the filename) ...
	// 
	requestHandle(connfd);

	Close(connfd);
    }

}


    


 
