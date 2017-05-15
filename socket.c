/* socket.c: Simple Socket Functions */

#include "spidey.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

/**
 * Allocate socket, bind it, and listen to specified port.
 **/

int socket_listen(const char *port)
{
    struct addrinfo *results;
    int    socket_fd = -1;
    struct addrinfo hints =  { 

    .ai_family  = AF_UNSPEC, 
    .ai_socktype    = SOCK_STREAM, 
    .ai_flags   = AI_PASSIVE,
    }; 

    /* Lookup server address information */
    int status;
    if ((status = getaddrinfo(NULL, port, &hints, &results)) != 0) {
        fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(status));
        return -1;
    }


    /* For each server entry, allocate socket and try to connect */
    for (struct addrinfo *p = results; p != NULL && socket_fd < 0; p = p->ai_next) {
    /* Allocate socket */
    if((socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0){
        fprintf(stderr, "Socket Failed: %s\n", strerror(errno)); 
        continue; 
    }
    /* Bind socket */
    if(bind(socket_fd, p->ai_addr, p->ai_addrlen) < 0){
            fprintf(stderr, "Bind Failed: %s\n", strerror(errno)); 
            close(socket_fd); 
            socket_fd = -1;
            continue; 
    }

        /* Listen to socket */
    if(listen(socket_fd,  SOMAXCONN) < 0){
            fprintf(stderr, "Listen Failed: %s\n", strerror(errno)); 
            close(socket_fd); 
            socket_fd = -1;
            continue;       
    }
    } 

    freeaddrinfo(results);
    return socket_fd;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */

