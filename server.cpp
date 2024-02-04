#include <arpa/inet.h>   // Include for network byte order conversions
#include <errno.h>       // Include for error number definitions
#include <netinet/ip.h>  // Include for IP protocol family
#include <stdint.h>      // Include for fixed width integer types
#include <stdio.h>       // Include for standard input/output
#include <stdlib.h>      // Include for general utilities like dynamic memory management
#include <string.h>      // Include for string manipulation
#include <sys/socket.h>  // Include for socket-specific definitions
#include <unistd.h>      // Include for POSIX operating system API

const int buffer_size = (1 << 20);

// Function to print a message to stderr without exiting the program
static void msg(const char* msg) {
    fprintf(stderr, "%s\n", msg);
}

// Function to handle program exit upon failure, prints the error and exits
static void die(const char* msg) {
    int err = errno;                         // Get the last error number
    fprintf(stderr, "[%d] %s\n", err, msg);  // Print the error message with error number
    abort();                                 // Abort the program
}

// Function to handle communication with the connected client
static void do_something(int connfd) {
    char rbuf[buffer_size] = {};                   // Buffer to store data read from the client
    int n = read(connfd, rbuf, sizeof(rbuf) - 1);  // Read data from the client
    if (n < 0) {
        msg("read() error");  // Print error message if read fails
        return;               // Exit the function if read fails
    }
    printf("%s\n", rbuf);  // Print the message received from the client
    // char wbuf[] = "world";              // Response message to be sent to the client
    // write(connfd, wbuf, strlen(wbuf));  // Send response message to the client

    const char* httpResponse =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "\r\n"
        "<html><body><h1>Hello, World!</h1></body></html>\n";

    ssize_t written = write(connfd, httpResponse, strlen(httpResponse));
}

int main() {
    // Create a TCP socket using IPv4
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");  // Exit if socket creation failed
    }

    // Enable the SO_REUSEADDR option to allow the socket to be quickly reused after the server is closed
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    struct sockaddr_in addr = {};  // Initialize socket address structure for IPv4
    addr.sin_family = AF_INET;     // Address family for IPv4
    addr.sin_port = htons(1234);   // Convert port number 1234 to network byte order
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    // addr.sin_addr.s_addr = INADDR_ANY;  // Bind to all available interfaces

    // Bind the socket to the specified IP address and port
    int rv = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
    if (rv) {
        die("bind()");  // Exit if bind fails
    }

    // Listen for incoming connection requests on the socket
    if (listen(fd, 10) < 0) {
        die("listen()");  // Exit if listen fails
    }

    while (true) {
        struct sockaddr_in client_addr = {};                                // Client address structure
        socklen_t socklen = sizeof(client_addr);                            // Size of client address structure
        int connfd = accept(fd, (struct sockaddr*)&client_addr, &socklen);  // Accept a connection request

        if (connfd < 0) {
            msg("accept() error");  // Print error message if accept fails
            continue;               // Continue to the next iteration of the loop if accept fails
        }

        do_something(connfd);  // Handle communication with the connected client
        close(connfd);         // Close the connection
    }

    return 0;
}
