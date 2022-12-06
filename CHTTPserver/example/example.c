#include "../lib/http.c"

void request(char *request, server_t response)
{
    // Parse the url
    char * url = http_url_response(request);
    printf("request: [%s] len: %lu\n", url, strlen(url));
    
    // If there is no URL specified
    if(http_url_none(url))
    {
        http_serve_static(response, "site/hello.html", "Content-Type: text/html");
    }
    // Otherwise
    else 
    {
        // Try serving the requested file
        int serve_status = http_serve_static(response, url, http_content_type(url));
        // If there was an error, throw 404
        if(serve_status == HTTP_SERVE_FAIL)
        {
            http_serve_404(response);
        }
    }
}

int main()
{
    //const char * IP = "192.168.0.223";
    //const char * IP = "10.10.241.222";
    const char * IP = "10.0.0.216";
    
    server_t website = http_server_create(IP, 3490);
    http_server_listen(website, request);
}