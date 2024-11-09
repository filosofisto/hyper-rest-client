#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

char *get(const char *hostname, const char *path, const char *port, int buffer_size)
{
    struct addrinfo hints, *res;
    int sockfd;
    char request[1024];
    char buffer[buffer_size];
    char *response = NULL;
    size_t response_size = 0;

    // Step 1: Set up the hints structure and resolve the server address
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;          // IPv4
    hints.ai_socktype = SOCK_STREAM;    // TCP stream sockets

    if (getaddrinfo(hostname, port, &hints, &res) != 0) {
        perror("getaddrinfo");
        return NULL;
    }

    // Step 2: Create a socket
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        perror("socket");
        freeaddrinfo(res);
        return NULL;
    }

    // Step 3: Connect to the server
    if (connect(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("connect");
        close(sockfd);
        freeaddrinfo(res);
        return NULL;
    }

    // Step 4: Formulate the HTTP GET request
    snprintf(request, sizeof(request), "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", path, hostname);

    // Step 5: Send the request
    if (send(sockfd, request, strlen(request), 0) < 0) {
        perror("send");
        close(sockfd);
        freeaddrinfo(res);
        return NULL;

    }

    // Step 6: Read the response
    int bytes_received;
    while ((bytes_received = recv(sockfd, buffer, buffer_size-1, 0)) > 0) {
        buffer[bytes_received] = '\0'; // Null-terminate the received data
        
        // Dynamically expand the response buffer to accomodate new data
        char *new_response = (char*) realloc(response, response_size+bytes_received+1);

        if (new_response == NULL) {
            // No memory
            perror("realloc");
            free(response);
            close(sockfd);
            freeaddrinfo(res);
            return NULL;
        }

        response = new_response;

        // Append the new data to the response
        memcpy(response + response_size, buffer, bytes_received+1);
        response_size += bytes_received;
    }

    if (bytes_received < 0) {
        perror("recv");
        free(response);
        response = NULL;
    }

    // Step 7: Clean up
    close(sockfd);
    freeaddrinfo(res);

    return response; // Caller is responsible for freeing the response
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("Wrong number of parameters\n");
        return -1;
    }

    const char *hostname = argv[1];
    const char *path = "/";

    printf("Sending HTTP GET request to %s%s\n", hostname, path);

    char *content = get(hostname, path, "80", 4096);

    if (content) {
        printf("Received content:\n\n%s\n", content);
        free(content);
    } else {
        printf("Failed to retrieve content.\n");
    }

    return 0;
}
