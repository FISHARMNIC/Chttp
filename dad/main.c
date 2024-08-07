#include "../lib/http.h"

void request(request_t request, response_t response)
{
    printf("new! %s %s %lu %s\n", request.method, request.url, strlen(request.url), http_file_extension(request.url));
    if(http_url_none(request.url))
    {
        // default home 
        http_serve_static(response, "index.html", "Content-Type: text/html");
    }
    else 
    {
        // Get the content type for the file
        char * contentType = http_content_type_from_file(request.url);
        // Try serving the requested file with the contentType header
        int serve_status = 0; http_serve_static(response, request.url, contentType);

        // If there was an error, throw 404
        if(serve_status == HTTP_SERVE_ERR)
        {
            http_serve_404(response);
        }
    }
}

int main()
{
    //const char * IP = "192.168.0.223";
    //const char * IP = "10.10.241.222";
    //const char * IP = "10.0.0.216";
    const char * IP = "192.168.0.92";
    
    server_t website = http_server_create(IP, 8080);
    printf("Initiated:\n"); fflush(stdout);
    http_server_listen(website, request);
}