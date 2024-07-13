# Chttp, a lightweight C HTTP server library


## How To
1. Prepare
  * Ensure MacOS firewall is **OFF**
  * Working directory example:
```
|- MyWebsite
     |- website.c
     |- index.html
     |- index.css
     |- images
          |- cow.png
|- lib
     |- http.c
     |- http.h
```
3. Create the server with your local IP as a string, followed by the port number
```C 
server_t website = http_server_create("192.168.0.1", 8080);
```
3. Create a function that handles each request
```C
void request(request_t request, response_t response)
{
    // If no path given, just serve index.html
    if(http_url_none(request.url))
    {
        http_serve_static(response, "index.html", "Content-Type: text/html");
    }
    else 
    {
        // Otherwise, get serve any requested file

        // Get the contentype of the file
        char * contentType = http_content_type_from_file(request.url);

        // Try to serve the file
        int serve_status = http_serve_static(response, request.url, contentType);

        // If it fails, just serve a default 404 page
        if(serve_status == HTTP_SERVE_ERR)
        {
            http_serve_404(response);
        }
    }
}
```
4. Start the server
```C
http_server_listen(website, request);
```
5. Compile and link with `http.c`
```
clang MyWebsite/website.c lib/http.c -o website.out; ./website.out
```
7. Open a browser and navigate to `ip:port` (ex. `192.168.0.1:8080`)
