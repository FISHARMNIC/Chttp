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
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>
#include <ctype.h>

#define http_url_none(url) strlen(url) == 0
#define HTTP_SERVE_ERR -1
#define HTTP_MAX_HEADERS 1024

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

// see documentation at declaration
http_header_map_t http_headers_format(char *string);

/// @brief Create a server
/// @param ipAddr String containing the host IP address
/// @param port Number containing the port you want to specify
/// @return Descriptor to the server
server_t http_server_create(const char *ipAddr, uint16_t port)
{
    // Load our IP into the address
    struct in_addr internetAddress;
    inet_pton(AF_INET, ipAddr, &(internetAddress.s_addr));

    struct sockaddr_in socketInternet = {
        .sin_family = AF_INET,
        // Use htons to convert the port to Network Byte Order
        .sin_port = htons(port),
        .sin_addr = internetAddress,
    };
    // Needs to be filled with zero
    memset(socketInternet.sin_zero, 0, 8);

    /*
    AF_INET     : IPV4 domain type
    SOCK_STREAM : TCP connection - remote server with transfer of bytes
    0           : Default protocol
    */
    int server_socket_des = socket(AF_INET, SOCK_STREAM, 0);

    // Bind the socket to our port
    bind(server_socket_des, (struct sockaddr *)&socketInternet, sizeof(socketInternet));

    // Return the socket
    return (server_t)server_socket_des;
}

/// @brief Take raw request data and convert it into a request structure.
/// Note that the properties {.url, .method, .header.raw, .data} should be freed with (free)
/// after using the data.
/// @param str Raw request string
/// @return Formatted request structure
request_t http_parse_struct_from_req(char *str)
{
    /*
    Fromat looks like this:
    method /url HTTP/1.1
    headers

    data
    */
    int len = strlen(str);

    char *method = calloc(len, 1);
    char *url = calloc(len, 1);
    char *headers = calloc(len, 1);
    char *data = calloc(len, 1);

    assert(method != 0 && url != 0 && headers != 0 && data != 0);

    char *firstSpace = strchr(str, ' ');
    char *secSpace = strchr(firstSpace + 1, ' ');

    char *eol = strchr(str, '\n') + 1;
    char *secEol = strstr(eol, "\n\n");

    // memcpy(method, firstSpace+1, firstSpace - str- 2);
    // memcpy(url, secSpace + 1, eol - secSpace - 3);
    memcpy(url, firstSpace + 2, secSpace - str - 5);
    memcpy(method, str, firstSpace - str);

    if (eol == NULL)
        printf("RIP");
    if (secEol == NULL)
    {
        strcpy(headers, eol);
    }
    else
    {
        memcpy(headers, eol, secEol - eol - 2);
        strcat(headers, "\n");
        strcpy(data, secEol);
    }

    return (request_t){
        .data = data,
        .header = http_headers_format(headers),
        .method = method,
        .url = url};
}

/// @brief Function for threading client requests
/// @param vp_arg Pointer to __http_threader_args__
/// @return Exit status
/// @note Intended for internal use
void *__http_server_listen_threader__(void *vp_arg)
{
    // Convert void pointer to correct format
    __http_threader_args__ data = *((__http_threader_args__ *)vp_arg);

    // Load the client data with the request data
    recv(data.client_socket_des, data.getP, 1024, 0);
    printf("FINISHED RECV\n");
    request_t out = http_parse_struct_from_req(data.getP);
    // Run the client onRequest function

    data.onRequest(out, data.client_socket_des);

    // why can't I free this! bug
    // free(out.data);
    // free(out.header.raw);
    // free(out.method);
    // free(out.url);

    // Close the client
    printf("Closing\n");
    shutdown(data.client_socket_des, 1);
    printf("Closed\n");

    // Exit the thread
    return NULL;
    // pthread_exit(NULL);
}

/// @brief Begin listening on a server
/// @param server_socket_des Descriptor to the server
/// @param onRequest Function to be executed on each request
void http_server_listen(server_t server_socket_des, reqfn_t onRequest)
{
    // Start listening
    listen(server_socket_des, 100);
    printf("Listening...\n");
    fflush(stdout);

    // Set up the client
    struct sockaddr_in client_socket;
    socklen_t client_socket_len;
    int client_socket_des;
    char getP[1024];

    pthread_t tid;

    int fbuff;

    while (1)
    {
        printf("Waiting...\n");
        fflush(stdout);

        // Accept any incoming reuquest
        // this is causing the issue some socket is closed alr?
        if (read(server_socket_des, &fbuff, 1) != 0)
        {
            printf("Attempting accept %i\n", server_socket_des); fflush(stdout);
            client_socket_des = accept(server_socket_des, (struct sockaddr *)&client_socket, &client_socket_len);
            printf("Got %i\n", client_socket_des); fflush(stdout);
            if (client_socket_des != -1)
            {
                printf("req\n");
                fflush(stdout);
                __http_threader_args__ args = {
                    .client_socket_des = client_socket_des,
                    .getP = getP,
                    .onRequest = onRequest,
                };

                // Process the request and serve data on a seperate thread
                printf("Creating thread\n");
                if (pthread_create(&tid, NULL, __http_server_listen_threader__, (void *)(&args)) != 0)
                {
                    printf("Error! Cannot create thread");
                }
            }
            else
            {
                printf("Socket Accept Fail... Continuing");
            }
        }
    }
}

/// @brief Serve data from a socket
/// @param socket Socket descriptor
/// @param data Data buffer
/// @param len Data buffer length
/// @param headers Response headers, seperated with new-line. Do not leave trailing newline!
void http_serve(int socket, char *data, int len, char *headers)
{
    // Mandatory header information
    char *ostring = malloc(len + 300);

    char *headerST = "HTTP/1.1 200 OK\nAccept-Ranges: bytes\nContent-Length: ";
    char *headerEND = "\nConnection: close\n";

    // Load the size into Content-Length
    char sizeStr[((int)((ceil(log10(len)) + 1) * sizeof(char)))];

    sprintf(sizeStr, "%d", len);

    strcpy(ostring, headerST);
    strcat(ostring, sizeStr);
    strcat(ostring, headerEND);
    if (headers != NULL)
        strcat(ostring, headers);
    strcat(ostring, "\n\n");

    int olen = strlen(ostring);
    char *zero = (char *)(olen + ostring);
    memcpy(zero, data, len);
    // printf("\n\n\n\%s\n\n\n", ostring);
    //  Send the output data throught the socket
    send(socket, ostring, olen + len, 0);

    free(ostring);
}

/// @brief Serve a 404 error
/// @param socket Socket descriptor
static inline void http_serve_404(int socket)
{
    send(socket, "HTTP/1.0 404 Not Found\nContent-Type: text/html\n\n<h1>Error 404</h1><p>Not Found</p>", 82, 0);
}

/// @brief Serve a 403 error
/// @param socket Socket descriptor
static inline void http_serve_403(int socket)
{
    send(socket, "HTTP/1.0 403 Forbidden\nContent-Type: text/html\n\n<h1>Error 403</h1></p>Access Forbidden</p>", 90, 0);
}

/// @brief Serve data without the http protocol required headers
/// @param socket Socket descriptor
/// @param data Data buffer
/// @param length Data buffer length
static inline void http_serve_raw(int socket, const char *data, int length)
{
    send(socket, data, length, 0);
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
int http_serve_static(int socket, char *filename, char *headers)
{
    char *buffer = 0;
    long length;

    // Load up a buffer with the file contents
    FILE *fp = fopen(filename, "rb");

    if (fp == NULL)
    {
        printf("--HTTP ERROR-- Unkown File [%s] --HTTP_SERVE_STATUS-- returning -1\n", filename);
        fflush(stdout);
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    length = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    buffer = (char *)malloc(length + 1);
    if (buffer)
    {
        fread(buffer, 1, length, fp);
        buffer[length + 1] = 0;
        printf("Serving\n");
        http_serve(socket, buffer, length, headers);
        printf("Fin\n");
        free(buffer);
    }
    else
    {
        printf("Malloc Error");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }
    fclose(fp);

    printf("Read from file\n");

    return 0;
}

/// @brief Get a file extension
/// @param filename Name of the file
/// @return File extension
char *http_file_extension(char *filename)
{
    char *dot = strrchr(filename, '.');
    if (!dot || dot == filename)
        return "";
    dot += 1;
    int i = 0;
    // fix for dads trash website that requests all cap formats
    while (dot[i] != 0)
    {
        dot[i] = tolower((int)(dot[i]));
        i++;
    }
    printf("NEWFNAME: %s\n", dot);
    return dot;
}

/// @brief Create the accurate Content-Type header from a file
/// @param filename Name of the file
/// @return Content-Type header
char *http_content_type_from_file(char *filename)
{
    if (strlen(filename) + 16 > 60)
        return NULL;

    static char mimeprefix[60];
    strcpy(mimeprefix, "Content-Type: ");
    strcat(mimeprefix, http_file_extension(filename));
    strcat(mimeprefix, "\0");
    return mimeprefix;
}

/// @brief Create a map of headers from a string
/// @param string Headers to be mapped
/// @return Header map
http_header_map_t http_headers_format(char *string)
{
    int destIndex = 0;
    const int buildSize = 200;
    char build[buildSize];
    char *buildP = build;
    char *des;

    char *map_keys[HTTP_MAX_HEADERS];
    char *map_values[HTTP_MAX_HEADERS];

    memset(build, 0, buildSize);
    while (destIndex < HTTP_MAX_HEADERS)
    {
        // clear build on eol
        if (*string == '\n' || *string == 0)
        {
            *buildP = '\0';
            buildP = build;

            des = (char *)malloc(buildSize);
            strcpy(des, build);
            map_values[destIndex++] = des;
            // exit if finished
            if (*string == 0)
                goto ret;
        }
        // clear build on colon
        else if (*string == ':')
        {
            *buildP = '\0';
            buildP = build;

            des = (char *)malloc(buildSize);
            strcpy(des, build);
            map_keys[destIndex] = des;
        }
        else
        {
            *(buildP++) = *string;
        }
        string++;
    }
    printf("Error: Too many headers");
    fflush(stdout);
    exit(1);
ret:
    return (http_header_map_t){
        .keys = map_keys,
        .values = map_values,
        .raw = string,
    };
}

/// @brief Get the value of a header
/// @param hmap Map
/// @param cmp Header to search
/// @return Header's value, NULL if not found
char *http_headers_value(http_header_map_t hmap, char *cmp)
{
    for (int i = 0; i < HTTP_MAX_HEADERS; i++)
    {
        if (strcmp(*hmap.keys[i], cmp) == 0)
        {
            return *hmap.values[i];
        }
    }
    return NULL;
}