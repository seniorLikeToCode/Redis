#include <arpa/inet.h>   // Include for network byte order conversions
#include <errno.h>       // Include for error number definitions
#include <netinet/ip.h>  // Include for IP protocol family
#include <stdint.h>      // Include for fixed width integer types
#include <stdio.h>       // Include for standard I/0
#include <stdlib.h>      // Include for general utilities
#include <string.h>      // Include for string manigulation
#include <sys/socket.h>  // Include for socket-specific definitions
#include <unistd.h>      // Include for POSIX operating system api

const int buffer_size = (1 << 20);

// function to handle program exit upon failure
static void die(const char* msg) {
    int err = errno;                         // Get the last error number
    fprintf(stderr, "[%d] %s\n", err, msg);  // Print the error message with error number
    abort();                                 // Abort the program
}

int main() {
    // create TCP socket using IPv4
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");  // Exit if socket creation failed
    }

    struct sockaddr_in addr = {};                   // Initialize socket address structure
    addr.sin_family = AF_INET;                      // Address family for IPv4
    addr.sin_port = ntohs(1234);                    // Convert port number 1234 to network byte order
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);  // convert loopback address to network byte order

    // Attempt to connect to the specified address and port
    int rv = connect(fd, (const struct sockaddr*)&addr, sizeof(addr));
    if (rv) {
        die("connect");  // Exit if connection failed
    }

    char msg[] = "hello";         // Message to send
    write(fd, msg, strlen(msg));  // Send the message

    char rbuf[buffer_size] = {};                   // Buffer for receiving response
    ssize_t n = read(fd, rbuf, sizeof(rbuf) - 1);  // Read response into buffer
    if (n < 0) {
        die("read");  // Exit if red failed
    }

    printf("server say: %s\n", rbuf);  // Print the server's response
    close(fd);                         // close the socket
    return 0;
}