#include <arpa/inet.h>   // For network byte order conversions
#include <errno.h>       // For error number definitions
#include <netinet/ip.h>  // For IP protocol family
#include <stdio.h>       // For standard input/output
#include <stdlib.h>      // For general utilities like dynamic memory management
#include <string.h>      // For string manipulation
#include <sys/socket.h>  // For socket-specific definitions
#include <unistd.h>      // For POSIX operating system API
#include <cassert>       // For asserting a statement
#include <cstdint>       // For fixed-width integer types
#include <cstring>       // For C++ string standard functions
#include <iostream>      // For I/O stream
#include <memory>        // For managing memory resources
#include <stdexcept>     // For standard exceptions
#include <vector>        // For working with vectors (dynamic arrays)

const size_t kMaxMsg = (1 << 20);  // Define the maximum message size

// Function to throw a std::runtime_error with the last error number
void die(const std::string& msg) {
    throw std::runtime_error(msg + " : " + std::strerror(errno));
}

// function to read full content from a socket
int32_t read_full(int fd, char* buf, size_t n) {
    while (n > 0) {
        // Attempt to read 'n' bytes from the file description 'fd' into the buffer 'buf'
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) return -1;  // Return error on read failure or unexpected EOF

        // Ensure the read value is within expected bounds
        assert(static_cast<size_t>(rv) <= n);

        // Decrement 'n' by the number of bytes read and advance the buffer pointer
        n -= static_cast<size_t>(rv);
        buf += rv;
    }

    return 0;
}

// Function to write the entire buffer to a socket
int32_t write_all(int fd, const char* buf, size_t n) {
    while (n > 0) {
        // Attempt to write 'n' bytes from 'buf' to the file description 'fd'
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) {
            return -1;  // Return error on write failure
        }

        // Ensure the written value is within expected bounds
        assert(static_cast<size_t>(rv) <= n);

        // Decrement 'n' by the number of bytes written and advance the buffer pointer
        n -= static_cast<size_t>(rv);
        buf += rv;
    }

    return 0;
}

// Function to handle a single request from a connect client
int32_t one_request(int connfd) {
    char rbuf[kMaxMsg];  // Request buffer

    // Read the first 4 bytes to get the message length
    if (int32_t err = read_full(connfd, rbuf, 4); err) {
        std::cerr << ((errno == 0) ? "EOF" : "read() error") << std::endl;
        return err;  // Return error if read fails
    }

    uint32_t len = 0;
    memcpy(&len, rbuf, 4);  // Copy the first 4 bytes to get the length of the incomming message

    // check if the message length exceeds the maximum allowed size
    if (len > kMaxMsg) {
        std::cerr << "too long" << std::endl;
        return -1;  // message to long
    }

    if (int32_t err = read_full(connfd, &rbuf[4], len); err) {  // Read the rest of the message based on the length specified
        std::cerr << "read() error" << std::endl;
        return err;  // Return error if read fails
    }

    rbuf[4 + len] = '\0';  // Null-terminate the received message from pointing
    std::cout << "Client says: " << &rbuf[4] << std::endl;

    const char reply[] = "world";                // Prepare the reply message
    char wbuf[4 + sizeof(reply)];                // Buffer for the reply, including the length prefix
    len = static_cast<uint32_t>(strlen(reply));  // Recalculate 'len' for reply
    memcpy(wbuf, &len, 4);                       // Prefix the reply with its length
    memcpy(&wbuf[4], reply, len);                // Copy the reply message itself

    return write_all(connfd, wbuf, 4 + len);  // Send the reply back to the client
}

int main() {
    try {
        // Create a TCP socket using IPv4
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            die("socket() failed");  // Exit if socket creation failed
        }

        // Enable the SO_REUSEADDR option to allow the socket to be quickly reused after the server is closed
        int val = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0) {
            die("setsocketop() failed");
        }

        struct sockaddr_in addr = {};       // Initialize socket address structure for IPv4
        addr.sin_family = AF_INET;          // Address family for IPv4
        addr.sin_port = htons(1234);        // Convert port number 1234 to network byte order
        addr.sin_addr.s_addr = INADDR_ANY;  // Bind to all available interfaces

        // Bind the socket to the specified IP address and port
        if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            die("bind() failed");  // Exit if bind fails
        }

        // Listen for incoming connection requests on the socket
        if (listen(fd, 10) < 0) {
            die("listen() failed");  // Exit if listen fails
        }

        while (true) {
            struct sockaddr_in client_addr = {};                                                  // Client address structure
            socklen_t socklen = sizeof(client_addr);                                              // Size of client address structure
            int connfd = accept(fd, reinterpret_cast<struct sockaddr*>(&client_addr), &socklen);  // Accept a connection request

            if (connfd < 0) {
                std::cerr << "accept() error\n";
                continue;  // skip to the next iteration on error
            }

            // Handle requests from the connected client
            while (true) {
                if (int32_t err = one_request(connfd); err) {
                    break;  // Exit the loop if an error occurs
                }
            }

            close(connfd);  // Close the connection
        }
    } catch (const std::exception& e) {
        std::cerr << "Exceptin: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
