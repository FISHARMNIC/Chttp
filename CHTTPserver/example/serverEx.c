#include "../lib/server.c"

void request(char *request, server_t response)
{
    char * url = http_url_response(request); // parse the url

    printf("request: [%s] len: %lu\n", url,strlen(url));
    if(http_default_dir(url)) //if default directory
    {
        http_serve_static(response, "site/hello.html","Content-Type: text/html");
    } else {
        // serve the url
        // if we get -1, the file doesn't exist
        int serve_status = http_serve_static(response, url, http_content_type(url));
        if(serve_status == -1)
        {
            http_serve_404(response);
        }
    }
}

int main()
{
    //const char * IP = "192.168.0.223";
    const char * IP = "10.10.241.222";
    server_t server1 = http_server_create(IP, 3490);
    http_server_listen(server1, request);
}