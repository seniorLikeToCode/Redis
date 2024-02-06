#include <arpa/inet.h>   // For network byte order conversions
#include <netinet/ip.h>  // For IP protocol family
#include <sys/socket.h>  // For socket-specific definitions
#include <unistd.h>      // For POSIX operating system API
#include <cerrno>        // For error number definitions
#include <cstring>       // For string manipulation
#include <iostream>      // For standard I/O
#include <memory>        // For smart pointers
#include <stdexcept>     // For standard exceptions
#include <string>        // For std::string

const int buffer_size = (1 << 20);

// Function to throw a std::runtime_error with the last error number
void die(const std::string& msg) {
    throw std::runtime_error(msg + ": " + std::strerror(errno));
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

int main() {
    try {
        // Create TCP socket using IPv4
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            die("socket() failed");
        }

        sockaddr_in addr = {};                          // Initialize socket address structure
        addr.sin_family = AF_INET;                      // Address family for IPv4
        addr.sin_port = htons(1234);                    // Convert port number 1234 to network byte order
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  // Use loopback address

        // Attempt to connect to the specified address and port
        if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
            die("connect failed");
        }

        std::string msg = "hello";  // Message to send
        if (write(fd, msg.c_str(), msg.size()) < 0) {
            die("write failed");
        }

        auto rbuf = std::make_unique<char[]>(buffer_size);  // Buffer for receiving response
        ssize_t n = read(fd, rbuf.get(), buffer_size);      // Read response into buffer

        if (n < 0) {
            die("read failed");
        }
        rbuf[n] = '\0';  // Ensure null-termination

        std::cout << "Server says: " << rbuf.get() << std::endl;  // Print the server's response

        close(fd);  // Close the socket
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
