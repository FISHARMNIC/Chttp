#include "../lib/server.c"

void request(int clientSocket, char *dir)
{
    printf("request\n");
    char * url = http_response_url(dir);
    //printf("url: %s\n", url); fflush(stdout);
    http_serve(clientSocket, url, NULL);
}

int main()
{
    const char * IP = "192.168.0.223";
    http_createServer(IP, 3490, request);
}