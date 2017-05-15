/* single.c: Single User HTTP Server */

#include "spidey.h"

#include <errno.h>
#include <string.h>

#include <unistd.h>

/**
 * Handle one HTTP request at a time
 **/

void
single_server(int sfd)
{
    struct request *request;
    http_status status;
    /* Accept and handle HTTP request */
    while (true) {
        /* Accept request */
        request = accept_request(sfd);
        if (request != NULL){
        /* Handle request */
            status = handle_request(request);
        
            /* Free request */
            free_request(request);
        }
    }
    /* Close socket and exit */
    close(sfd);
    return EXIT_SUCCESS;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
