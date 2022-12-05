#include "../lib/server.c"

char errorMessage[25] = "<h1>Unknown Resource<h1>";

void request(int client, char *dir)
{
    char * url = http_response_url(dir);

    if(strlen(url) == 0) //if default directory
    {
        http_serve_static(client, "site/hello.html","Content-Type: text");
    } else {
        // serve the url
        // if we get -1, the file doesn't exist
        int serve_status = http_serve_static(client, url, http_content_type(url));
        if(serve_status == -1)
        {
            http_serve_404(client);
        }
    }
}

int main()
{
    const char * IP = "192.168.0.223";
    http_createServer(IP, 3490, request);
}