/* handler.c: HTTP Request Handlers */

#include "spidey.h"

#include <errno.h>
#include <limits.h>
#include <string.h>

#include <dirent.h>
#include <unistd.h>

/* Internal Declarations */
http_status handle_browse_request(struct request *request);
http_status handle_file_request(struct request *request);
http_status handle_cgi_request(struct request *request);
http_status handle_error(struct request *request, http_status status);

/**
 * Handle HTTP Request
 *
 * This parses a request, determines the request path, determines the request
 * type, and then dispatches to the appropriate handler type.
 *
 * On error, handle_error should be used with an appropriate HTTP status code.
 **/
http_status
handle_request(struct request *r)
{
    http_status result;

    /* Parse request */
    int rstatus = parse_request(r);
    if (rstatus == 0){
    /* Determine request path */
    r->path = determine_request_path(r->uri);
    request_type rtype = determine_request_type(r->path);     
    debug("HTTP REQUEST PATH: %s", r->path);
    /* Dispatch to appropriate request handler type */
    if(rtype == REQUEST_BROWSE)
        result = handle_browse_request(r);    
    else if(rtype == REQUEST_CGI)
        result = handle_cgi_request(r);
    else if(rtype == REQUEST_FILE)
        result = handle_file_request(r);

    }
    log("HTTP REQUEST STATUS: %s", http_status_string(result));
    return result;
}

/**
 * Handle browse request
 *
 * This lists the contents of a directory in HTML.
 *
 * If the path cannot be opened or scanned as a directory, then handle error
 * with HTTP_STATUS_NOT_FOUND.
 **/
http_status handle_browse_request(struct request *r) {
    struct dirent **entries;
    int n;
    char link[BUFSIZ];
    struct header *header;
    char *host;
    /* Open a directory for reading or scanning */
    n = scandir(r->path, &entries, NULL, alphasort);
    if (n == -1) {;
        return HTTP_STATUS_NOT_FOUND;
    }
    
    header = r->headers;
    while (header != NULL){
        if (streq(header->name, "Host")){
            host = header->value;
        }
        header = header->next;
    }
    /* Write HTTP Header with OK Status and text/html Content-Type */
    fprintf(r->file, "HTTP/1.0 200 OK\r\n Content-Type: text/html\r\n\r\n");

    /* For each entry in directory, emit HTML list item */
    fprintf(r->file, "<html>\n");
    fprintf(r->file, "<ul>\n");
    while (n--) {
        if(!streq(r->uri, "/"))
            sprintf(link, "%s%s/%s", host, r->uri, entries[n]->d_name);
        else
            sprintf(link, "%s%s%s", host, r->uri, entries[n]->d_name);
    fprintf(r->file, "<li><a href=\"http://%s\"> %s</a></li>\n", link ,entries[n]->d_name);
        free(entries[n]);
    }
    fprintf(r->file, "</ul>\n");
    fprintf(r->file, "</html>\n");
    free(entries);

    /* Flush socket, return OK */
    fflush(r->file);
    return HTTP_STATUS_OK;
}

/**
 *  * Handle file request
 *   *
 *    * This opens and streams the contents of the specified file to the socket.
 *     *
 *      * If the path cannot be opened for reading, then handle error with
 *       * HTTP_STATUS_NOT_FOUND.
 *        **/
http_status handle_file_request(struct request *r){
    FILE *fs;
    char buffer[BUFSIZ];
    char *mimetype = NULL;
    size_t nread;

    /* Open file for reading */
    fs = fopen(r->path, "r");//switch it to R
    if (fs == NULL){
        return HTTP_STATUS_NOT_FOUND;
    }

    /* Determine mimetype */
    mimetype = determine_mimetype(r->path);

    /* Write HTTP Headers with OK status and determined Content-Type */
    fprintf(r->file, "HTTP/1.0 200 OK\r\n Content-Type: %s\r\n\r\n", mimetype);
    /* Read from file and write to socket in chunks */
    size_t members;
    while((members = fread(buffer, 1, BUFSIZ, fs)) > 0 ){
        if(members != fwrite(buffer, 1, members, r->file)){
            fclose(fs); 
            free(mimetype);
            return HTTP_STATUS_INTERNAL_SERVER_ERROR;
        }
    }   

    /* Close file, flush socket, deallocate mimetype, return OK */
    fclose(fs); 
    fflush(r->file);
    free(mimetype);
    return HTTP_STATUS_OK;
}

/**
 *  * Handle CGI request
 *   *
 *    * This popens and streams the results of the specified executables to the
 *     * socket.
 *      *
 *       *
 *        * If the path cannot be popened, then handle error with
 *         * HTTP_STATUS_INTERNAL_SERVER_ERROR.
 *          **/
http_status
handle_cgi_request(struct request *r)
{
    FILE *pfs;
    char buffer[BUFSIZ];
    struct header *header;
    int result;
    
    /* Export CGI environment variables from request:
 *     * http://en.wikipedia.org/wiki/Common_Gateway_Interface */
    result = setenv("DOCUMENT_ROOT", RootPath, 1);
    
    if (result < 0)
        fprintf(stderr, "failed to set environmental variable: %s\n", strerror(errno));
    result = setenv("QUERY_STRING", r->query, 1);
    if (result < 0)
        fprintf(stderr, "failed to set environmental variable: %s\n", strerror(errno));
    result = setenv("REMOTE_ADDR", r->host, 1);
    if (result < 0)
        fprintf(stderr, "failed to set environmental variable: %s\n", strerror(errno));
    result = setenv("REMOTE_PORT", r->port, 1);
    if (result < 0)
        fprintf(stderr, "failed to set environmental variable: %s\n", strerror(errno));
    result = setenv("REQUEST_METHOD", r->method, 1);
    if (result < 0)
        fprintf(stderr, "failed to set environmental variable: %s\n", strerror(errno));
    result = setenv("REQUEST_URI", r->uri, 1);
    if (result < 0)
        fprintf(stderr, "failed to set environmental variable: %s\n", strerror(errno));
    result = setenv("SCRIPT_FILENAME", r->path, 1);
    if (result < 0)
        fprintf(stderr, "failed to set environmental variable: %s\n", strerror(errno));
    result = setenv("SERVER_PORT", Port, 1);
    if (result < 0)
        fprintf(stderr, "failed to set environmental variable: %s\n", strerror(errno));
 
    /* Export CGI environment variables from request headers */
    header = r->headers;
    while (header != NULL){
        if (streq(header->name, "Accept")){
            result = setenv("HTTP_ACCEPT", header->value, 1);
        }
        else if (streq(header->name,"Host")){
            result = setenv("HTTP_HOST", header->value, 1);

        }
        else if (streq (header->name, "Accept-Language")){
            result = setenv("HTTP_ACCEPT_LANGUAGE", header->value, 1);

        }
        else if (streq (header->name, "Accept-Encoding")){
            result = setenv("HTTP_ACCEPT_ENCODING", header->value, 1);

        }
        else if (streq (header->name, "Connection")){
            result = setenv("HTTP_CONNECTION", header->value, 1);

        }
        else if (streq (header->name, "User-Agent")){
            result = setenv("HTTP_USER_AGENT", header->value, 1);

        }
        header = header->next;
    }

    /* POpen CGI Script */
    if(!(pfs = popen(r->path, "r")))
          return HTTP_STATUS_INTERNAL_SERVER_ERROR;

    /* Copy data from popen to socket */
    while(fgets(buffer, BUFSIZ, pfs) != NULL){
        fputs(buffer, r->file);
     }   

    /* Close popen, flush socket, return OK */
    pclose(pfs);
    fflush(r->file);
    return HTTP_STATUS_OK;
}

/**
 *  * Handle displaying error page
 *   *
 *    * This writes an HTTP status error code and then generates an HTML message to
 *     * notify the user of the error.
 *      **/
http_status
handle_error(struct request *r, http_status status)
{
    const char *status_string = http_status_string(status);

    /* Write HTTP Header */
    fprintf(r->file, "HTTP/1.0 %s\r\n", status_string);
    fprintf(r->file, "Content-Type: text/html\r\n");
    fprintf(r->file, "\r\n");
    /* Write HTML Description of Error*/
    fprintf(r->file, "%s", status_string);
    /* Return specified status */
    return status;
}


/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
