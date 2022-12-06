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

typedef int server_t;

#define http_url_none(url) strlen(url) == 0
#define HTTP_SERVE_FAIL -1
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
    return (server_t) server_socket_des;
}

/// @brief Structure containing client and listening information
/// @deprecated Intended for internal use 
typedef struct {
    server_t client_socket_des;
    char* getP;
    void (*onRequest)(char *, int);
} __http_threader_args__;

/// @brief Function for threading client requests
/// @param vp_arg Void pointer to __http_threader_args__
/// @return Exit status
/// @deprecated Intended for internal use
void* __http_server_listen_threader__(void * vp_arg)
{
    // Convert void pointer to correct format
    __http_threader_args__ data = *((__http_threader_args__*)vp_arg);

    // Load the client data with the request data 
    recv(data.client_socket_des, data.getP, 1024, 0);

    // Run the client onRequest function
    data.onRequest(data.getP, data.client_socket_des);

    // Close the client
    shutdown(data.client_socket_des, 1);

    // Exit the thread
    pthread_exit(NULL);
}

/// @brief Begin listening on a server
/// @param server_socket_des Descriptor to the server
/// @param onRequest Function to be executed on each request
void http_server_listen(server_t server_socket_des, void (*onRequest)(char *, int))
{
    // Start listening
    listen(server_socket_des, 10);

    // Set up the client
    struct sockaddr_in client_socket;
    socklen_t client_socket_len;
    int client_socket_des;
    char getP[1024];

    printf("SERVER INITIATED\n"); fflush(stdout);
    
    pthread_t tid;
    while (1)
    {
        // Accept any incoming reuquest
        client_socket_des = accept(server_socket_des, (struct sockaddr *)&client_socket, &client_socket_len);
        __http_threader_args__ args = {
            .client_socket_des = client_socket_des,
            .getP = getP,
            .onRequest = onRequest
        };

        // Process the request and serve data on a seperate thread
        pthread_create(&tid, NULL, __http_server_listen_threader__, (void*)(&args));
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
    char *headerST ="HTTP/1.1 200 OK\nAccept-Ranges: bytes\nContent-Length: ";
    char *headerEND = "\nConnection: close\n";

    // Load the size into Content-Length
    char *sizeStr = malloc((int)((ceil(log10(len)) + 1) * sizeof(char)));
    sprintf(sizeStr, "%d", len);

    strcpy(ostring, headerST);
    strcat(ostring, sizeStr);
    strcat(ostring, headerEND);
    if (headers != NULL)
        strcat(ostring, headers);
    strcat(ostring, "\n\n");
    
    int olen = strlen(ostring);
    char * zero = (char*)(olen + ostring);
    memcpy(zero, data, len);

    // Send the output data throught the socket
    send(socket, ostring, olen + len, 0);
    
    free(ostring);
    free(sizeStr);
}

/// @brief Serve a 404 error
/// @param socket Socket descriptor
static inline void http_serve_404(int socket)
{
    send(socket, "HTTP/1.0 404 Not Found\n\nNot Found", 34, 0);
}

/// @brief Serve a 403 error
/// @param socket Socket descriptor
static inline void http_serve_403(int socket)
{
    send(socket, "HTTP/1.0 403 Forbidden\nCannot Access This", 42, 0);
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

/// @brief Serve a file. Recommended to specify the format in the header
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
    if (fp)
    {
        fseek(fp, 0, SEEK_END);
        length = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        buffer = malloc(length);
        if (buffer)
            fread(buffer, 1, length, fp);
        else
        {
            printf("Malloc Error");
            exit(EXIT_FAILURE);
        }
        fclose(fp);
    }
    else
    {
        printf("Unkown File: %s\n", filename);
        fflush(stdout);
        return -1;
    }

    // Serve the file contents
    buffer[length + 1] = 0;
    http_serve(socket, buffer, length, headers);
    free(buffer);

    return 0;
}

/// @brief Parse the requested URL from a request header
/// @param buffer request header
/// @return the URL 
char *http_url_response(char *buffer)
{
    static char sub[100];
    memset(sub, 0, 100);

    // Check to see if the request is GET
    char *p = strstr(buffer, "GET");
    if (p != NULL)
    {
        // Look for the requested directory and return that
        char *end = strstr(buffer, "HTTP");
        strncpy(sub, (char *)(p + 5), (int)(end - p - 6));
        return sub;
    }
    else
    {
        // Not a GET request
        return NULL;
    }
}

/// @brief Get a file extension
/// @param filename Name of the file
/// @return File extension
const char *http_file_extension(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

/// @brief Create the accurate Content-Type header from a file
/// @param filename Name of the file
/// @return Content-Type header
char * http_content_type(char *filename)
{
    char* mimeprefix = malloc(100);
    strcpy(mimeprefix, "Content-Type: ");
    strcat(mimeprefix, http_file_extension(filename));
    return mimeprefix;
}