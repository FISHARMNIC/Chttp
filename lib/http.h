/*
https://www.geeksforgeeks.org/socket-programming-cc/
https://levelup.gitconnected.com/building-the-web-sockets-and-servers-for-dummies-886d1595a4f8
https://www.gnu.org/software/libc/manual/html_node/Sockets.html
https://pubs.opengroup.org/onlinepubs/009604499/functions/socket.html
https://pubs.opengroup.org/onlinepubs/009604499/basedefs/sys/socket.h.html

BEST:
https://beej.us/guide/bgnet/html/#structs
https://www.gta.ufrj.br/ensino/eel878/sockets/index.html
https://www3.ntu.edu.sg/home/ehchua/programming/webprogramming/http_basics.html
*/

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>
#include <ctype.h>

#define http_url_none(url) strlen(url) == 0
#define HTTP_SERVE_ERR -1
#define HTTP_MAX_HEADERS 1024
#define send_assert(...) {\
    int i = 0; \
    retry:\
    if(send(__VA_ARGS__) != -1) \
    {\
        printf("SERVE FAILIURE, retrying\n");\
        if(i++ < 3) goto retry;\
    }\
}

typedef int server_t;
typedef int response_t;

/// @brief Structure containing header map
/// @param raw    (char *) Original un-formatted header string
/// @param keys   (char **[]) Pointer to string array of keys
/// @param values (char **[]) Pointer to string array of values
typedef struct
{
    char *raw;
    char **keys[HTTP_MAX_HEADERS];   // pointer to string array
    char **values[HTTP_MAX_HEADERS]; // pointer to string array
} http_header_map_t;

/// @brief Structure containing request information
/// @param method (char *) Type of request
/// @param url    (char *) URL request (not including first slash)
/// @param header (http_header_map_t) Header map type containing request header
/// @param data   (char *) POST data or any other data after the headers(NULL if none)
typedef struct
{
    char *method;
    char *url;
    http_header_map_t header;
    char *data;
} request_t;

/// @brief Request function type
typedef void (*reqfn_t)(request_t, response_t);

/// @brief Structure containing client and listening information
/// @deprecated Intended for internal use
typedef struct
{
    server_t client_socket_des;
    char *getP;
    void (*onRequest)(request_t, response_t);
} __http_threader_args__;

/// @brief Create a server
/// @param ipAddr String containing the host IP address
/// @param port Number containing the port you want to specify
/// @return Descriptor to the server
server_t http_server_create(const char *, uint16_t);

/// @brief Take raw request data and convert it into a request structure.
/// Note that the properties {.url, .method, .header.raw, .data} should be freed with (free)
/// after using the data.
/// @param str Raw request string
/// @return Formatted request structure
request_t http_parse_struct_from_req(char *);

/// @brief Function for threading client requests
/// @param vp_arg Pointer to __http_threader_args__
/// @return Exit status
/// @note Intended for internal use
void *__http_server_listen_threader__(void *);

/// @brief Begin listening on a server
/// @param server_socket_des Descriptor to the server
/// @param onRequest Function to be executed on each request
void http_server_listen(server_t, reqfn_t);

/// @brief Serve data from a socket
/// @param socket Socket descriptor
/// @param data Data buffer
/// @param len Data buffer length
/// @param headers Response headers, seperated with new-line. Do not leave trailing newline!
void http_serve(int, char *, int, char *);

/// @brief Serve a 404 error
/// @param socket Socket descriptor
static inline void http_serve_404(int socket)
{
    send_assert(socket, "HTTP/1.0 404 Not Found\nContent-Type: text/html\n\n<h1>Error 404</h1><p>Not Found</p>", 82, 0);
}

/// @brief Serve a 403 error
/// @param socket Socket descriptor
static inline void http_serve_403(int socket)
{
    send_assert(socket, "HTTP/1.0 403 Forbidden\nContent-Type: text/html\n\n<h1>Error 403</h1></p>Access Forbidden</p>", 90, 0);
}

/// @brief Serve data without the http protocol required headers
/// @param socket Socket descriptor
/// @param data Data buffer
/// @param length Data buffer length
static inline void http_serve_raw(int socket, const char *data, int length)
{
    send_assert(socket, data, length, 0);
}

/// @brief Serve a string of data, like http_serve but without specifying length
/// @param socket Socket descriptor
/// @param data Data buffer
/// @param headers Response headers, seperated with new-line. Do not leave trailing newline!
static inline void http_serve_string(int socket, char *data, char *headers)
{
    http_serve(socket, data, strlen(data), headers);
}

/// @brief Serve a file.
/// Recommended to specify the format in the header using "http_content_type_from_file"
/// @param socket Socket descriptor
/// @param filename Name of the file
/// @param headers Response headers, seperated with new-line. Do not leave trailing newline!
/// @return Error code. -1 for file not found
int http_serve_static(int, char *, char *);

/// @brief Get a file extension
/// @param filename Name of the file
/// @return File extension
char *http_file_extension(char *);

/// @brief Create the accurate Content-Type header from a file
/// @param filename Name of the file
/// @return Content-Type header
char *http_content_type_from_file(char *);

/// @brief Create a map of headers from a string
/// @param string Headers to be mapped
/// @return Header map
http_header_map_t http_headers_format(char *);

/// @brief Get the value of a header
/// @param hmap Map
/// @param cmp Header to search
/// @return Header's value, NULL if not found
char *http_headers_value(http_header_map_t*, char *);

int http_headers_free(http_header_map_t*);