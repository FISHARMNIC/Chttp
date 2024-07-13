#include "http.h"

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

    // Ignore SIGPIPE errors
    signal(SIGPIPE, SIG_IGN);

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

    free(out.data);
    // TODO: fix, leak here
    //http_headers_free(out.header);
    free(out.header.raw);
    free(out.method);
    free(out.url);


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
    // freed 
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
    send_assert(socket, ostring, olen + len, 0);

    free(ostring);
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

    // freed
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
    char *ostring = string;

    static char *map_keys[HTTP_MAX_HEADERS];
    static char *map_values[HTTP_MAX_HEADERS];

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
            map_values[destIndex] = 0;
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
            map_keys[destIndex++] = des;
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

    // TODO copy map_keys and map_values to dyn alloc arr 

    map_keys[destIndex] = 0;
    map_values[destIndex] = 0;
    return (http_header_map_t){
        .keys = map_keys,
        .values = map_values,
        .raw = ostring,
    };
}

/// @brief Get the value of a header
/// @param hmap Map
/// @param cmp Header to search
/// @return Header's value, NULL if not found
char *http_headers_value(http_header_map_t* hmap_, char *cmp)
{
    http_header_map_t hmap = *hmap_;   
    for (int i = 0; i < HTTP_MAX_HEADERS; i++)
    {
        if (strcmp(*hmap.keys[i], cmp) == 0)
        {
            return *hmap.values[i];
        }
    }
    return NULL;
}

int http_headers_free(http_header_map_t* hmap_)
{
    http_header_map_t hmap = *hmap_;   
    for (int i = 0; i < HTTP_MAX_HEADERS; i++)
    {
        if (hmap.values[i] == 0)
        {
            return 0;
        }
        free(hmap.keys[i]);
        free(hmap.values[i]);
        hmap.keys[i] = 0;
        hmap.values[i] = 0;
    }
    return 0;
}