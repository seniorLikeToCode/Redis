#include <arpa/inet.h>   // Include for network byte order conversions
#include <errno.h>       // Include for error number definitions
#include <netinet/ip.h>  // Include for IP protocol family
#include <stdint.h>      // Include for fixed width integer types
#include <stdio.h>       // Include for standard input/output
#include <stdlib.h>      // Include for general utilities like dynamic memory management
#include <string.h>      // Include for string manipulation
#include <sys/socket.h>  // Include for socket-specific definitions
#include <unistd.h>      // Include for POSIX operating system API
#include <cstring>       // Include c++ string standard functions
#include <iostream>      // Include I/O stream
#include <memory>        // Including memory
#include <stdexcept>     // Including standard exception
#include <vector>        // Include vector

const size_t buffer_size = (1 << 20);

// Function to throw a std::runtime_error with the last error number
[[noreturn]] void die(const std::string& msg) {
    throw std::runtime_error(msg + " : " + std::strerror(errno));
}

// Function to handle communication with the connected client
void do_something(int connfd) {
    std::vector<char> rbuf(buffer_size);
    ssize_t n = read(connfd, rbuf.data(), buffer_size);

    if (n < 0) {
        std::cerr << "read() error\n";
        return;
    }

    rbuf[n] = '\0';
    std::cout << rbuf.data() << std::endl;  // Logging Response from the client

    const std::string httpResponse =  // Reponse to send
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "\r\n"
        "<html><body><h1>Hello, World!</h1></body></html>\n";

    ssize_t written = write(connfd, httpResponse.c_str(), httpResponse.size());  // send the response

    if (written < 0) {
        std::cerr << "response failed\n";  // Exit if reponse failed
    }
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

        struct sockaddr_in addr = {};  // Initialize socket address structure for IPv4
        addr.sin_family = AF_INET;     // Address family for IPv4
        addr.sin_port = htons(1234);   // Convert port number 1234 to network byte order
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        // addr.sin_addr.s_addr = INADDR_ANY;  // Bind to all available interfaces

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
                std::cerr << "accept() error\n";  // Print error message if accept fails
                continue;                         // Continue to the next iteration of the loop if accept fails
            }

            do_something(connfd);  // Handle communication with the connected client
            close(connfd);         // Close the connection
        }
    } catch (const std::exception& e) {
        std::cerr << "Exceptin: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
