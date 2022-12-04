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

void http_createServer(const char *ipAddr, uint16_t port, void (*onRequest)(int, char *))
{
    // Load our IP into the address
    struct in_addr internetAddress;
    inet_pton(AF_INET, ipAddr, &(internetAddress.s_addr));

    struct sockaddr_in socketInternet = {
        .sin_family = AF_INET,
        // use htons to convert the port to Network Byte Order
        .sin_port = htons(port),
        .sin_addr = internetAddress,
    };
    // needs to be filled with zero
    memset(socketInternet.sin_zero, 0, 8);

    /*
    AF_INET     : IPV4 domain type
    SOCK_STREAM : TCP connection - remote server with transfer of bytes
    0           : Default protocol
    */
    int server_socket_des = socket(AF_INET, SOCK_STREAM, 0);
    // bind the socket to our port
    bind(server_socket_des, (struct sockaddr *)&socketInternet, sizeof(socketInternet));

    // set up the server
    listen(server_socket_des, 10);

    // Set up the communication with the client
    struct sockaddr_in client_socket;
    socklen_t client_socket_len;

    // accept the client request
    int client_socket_des;
    char getP[1024];
    while (1)
    {
        client_socket_des = accept(server_socket_des, (struct sockaddr *)&client_socket, &client_socket_len);
        //printf("req\n");
        recv(client_socket_des, getP, sizeof(getP), 0);
        onRequest(client_socket_des, getP);
    }
}

void http_serve(int socket, char *data, char *headers)
{
    int len = strlen(data);
    char *ostring = malloc(len + 300);

    char *headerST =
        "HTTP/1.1 200 OK\n\
Accept-Ranges: bytes\n\
Content-Length: ";
    char *headerEND =
        "\nContent-Type: text/html\n\
Connection: close\n";

    char *sizeStr = malloc((int)((ceil(log10(len)) + 1) * sizeof(char)));
    sprintf(sizeStr, "%d", len);

    strcpy(ostring, headerST);
    strcat(ostring, sizeStr);
    strcat(ostring, headerEND);
    if(headers != NULL)
        strcat(ostring, headers);
    strcat(ostring, "\n");
    strcat(ostring, data);

    //printf("---sending---\n%s\n", ostring);
    send(socket, ostring, strlen(ostring) + 5, 0);
}

void http_serve_static(int socket, char *filename, char * headers)
{
    char *buffer = 0;
    long length;
    FILE *fp = fopen(filename, "rb");

    if (fp)
    {
        fseek(fp, 0, SEEK_END);
        length = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        buffer = malloc(length);
        if (buffer)
            fread(buffer, 1, length, fp);
        else {
            printf("Malloc Error");
            exit(EXIT_FAILURE);
        }
        fclose(fp);
    } else {
        printf("Unkown File: %s", filename);
    }

    http_serve(socket, buffer, headers);
    free(buffer);
}

char * http_response_url(char *buffer)
{
    static char sub[100];
    char *p = strstr(buffer, "GET");
    printf("---%s---", buffer); fflush(stdout);
    if(p != NULL)
    {
        char *end = strstr(buffer, "HTTP");
        strncpy(sub, (char*)(p + 4), (int)(end - p - 4));
        return sub;
    } else {
        return NULL;
    }
}