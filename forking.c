/* forking.c: Forking HTTP Server */

#include "spidey.h"

#include <errno.h>
#include <signal.h>
#include <string.h>

#include <unistd.h>

/**
 * Fork incoming HTTP requests to handle the concurrently.
 *
 * The parent should accept a request and then fork off and let the child
 * handle the request.
 **/
void
forking_server(int sfd)
{
    struct request *request;
    pid_t pid;

    /* Accept and handle HTTP request */
    while (true) {
    	/* Accept request */
        request = accept_request(sfd);
        if (request == NULL) {
            continue;
        }
	/* Ignore children */
        signal(SIGCHLD, SIG_IGN);
	/* Fork off child process to handle request */
        pid_t pid = fork();
        if (pid < 0){
            fprintf(stderr, "Unable to fork: %s\n", strerror(errno))
        }
        else if (pid == 0){
            close(sfd);
            handle_request(request);
            exit(EXIT_SUCCESS);
        }
        else {
            free_request(request);
        }
    }   

    /* Close server socket and exit*/
    close(sfd);
    exit(EXIT_SUCCESS);
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
