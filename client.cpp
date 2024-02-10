#include <arpa/inet.h>   // Provides declarations for internet operations
#include <assert.h>      // Provides a macro for adding diagnostics
#include <errno.h>       // Defines macros for reporting and retrieving error conditions
#include <netinet/ip.h>  // Defines structures and constants for internet domain addresses
#include <stdint.h>      // Defines exact-width integer types
#include <stdio.h>       // Provides declarations for input and output functions
#include <stdlib.h>      // Provides declarations for general purpose functions
#include <string.h>      // Provides declarations for string handling functions
#include <sys/socket.h>  // Provides declarations for socket interface
#include <unistd.h>      // Provides access to the POSIX operating system API

// Function to print messages to stderr
static void msg(const char* msg) {
    fprintf(stderr, "%s\n", msg);
}

// Function to print error messages to stderr and abort the program
static void die(const char* msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

// Reads exactly `n` bytes from file descriptor `fd` into `buf`
static int32_t read_full(int fd, char* buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) {
            return -1;  // error, or unexpected EOF
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

// Writes exactly `n` bytes from `buf` to file descriptor `fd`
static int32_t write_all(int fd, const char* buf, size_t n) {
    while (n > 0) {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) {
            return -1;  // error
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

const size_t k_max_msg = 4096;  // Maximum message size

// Sends a request `text` to the server by writing it to file descriptor `fd`
static int32_t send_req(int fd, const char* text) {
    uint32_t len = (uint32_t)strlen(text);
    if (len > k_max_msg) {
        return -1;
    }

    char wbuf[4 + k_max_msg];
    memcpy(wbuf, &len, 4);  // assume little endian
    memcpy(&wbuf[4], text, len);
    return write_all(fd, wbuf, 4 + len);
}

// Reads a response from the server from file descriptor `fd`
static int32_t read_res(int fd) {
    char rbuf[4 + k_max_msg + 1];
    errno = 0;
    int32_t err = read_full(fd, rbuf, 4);
    if (err) {
        if (errno == 0) {
            msg("EOF");
        } else {
            msg("read() error");
        }
        return err;
    }

    uint32_t len = 0;
    memcpy(&len, rbuf, 4);  // assume little endian
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }

    err = read_full(fd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }

    rbuf[4 + len] = '\0';
    printf("server says: %s\n", &rbuf[4]);
    return 0;
}

// Main function: sets up a socket, connects to the server, sends and receives messages
int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);  // Create a TCP socket
    if (fd < 0) {
        die("socket()");
    }

    struct sockaddr_in addr = {};                                       // Socket address structure
    addr.sin_family = AF_INET;                                          //
    addr.sin_port = ntohs(1234);                                        // Server port number
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);                      // Server IP address (127.0.0.1)
    int rv = connect(fd, (const struct sockaddr*)&addr, sizeof(addr));  // Connect to the server
    if (rv) {
        die("connect");
    }

    // Send multiple pipelined requests to the server
    const char* query_list[3] = {"hello1", "hello2", "hello3"};
    for (size_t i = 0; i < 3; ++i) {
        int32_t err = send_req(fd, query_list[i]);
        if (err) {
            goto L_DONE;
        }
    }
    // Read responses for each request
    for (size_t i = 0; i < 3; ++i) {
        int32_t err = read_res(fd);
        if (err) {
            goto L_DONE;
        }
    }

L_DONE:
    close(fd);  // Close the socket
    return 0;
}
