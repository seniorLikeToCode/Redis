#include <arpa/inet.h>   // Include for network byte order conversions
#include <errno.h>       // Include for error number definitions
#include <netinet/ip.h>  // Include for IP protocol family
#include <stdio.h>       // Include for standard input/output
#include <stdlib.h>      // Include for general utilities like dynamic memory management
#include <string.h>      // Include for string manipulation
#include <sys/socket.h>  // Include for socket-specific definitions
#include <unistd.h>      // Include for POSIX operating system API
#include <cassert>       // asserting a statement
#include <cstdint>       // Include for fixed width integer types
#include <cstring>       // Include c++ string standard functions
#include <iostream>      // Include I/O stream
#include <memory>        // Including memory
#include <stdexcept>     // Including standard exception
#include <vector>        // Include vector

const int kMaxMsg = (1 << 20);

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

int32_t query(int fd, const std::string& text) {
    // Check if the text length exceeds the maximum allowed size
    if (text.length() > kMaxMsg) {
        return -1;  // Return error if text is too long
    }

    std::vector<char> wbuf(4 + kMaxMsg);  // Allocate a buffer with space for the length prefix plus the text
    uint32_t len = static_cast<uint32_t>(text.length());

    std::memcpy(wbuf.data(), &len, 4);                          // Copy the length of the text to the first 4 bytes of the buffer
    std::memcpy(wbuf.data() + 4, text.c_str(), text.length());  // Copy the actual text into the buffer, starting after the 4-byte length prefix

    // Write the buffer the file description (network connection)
    if (write_all(fd, wbuf.data(), 4 + len) != 0) {
        return -1;  // Return error if the write operation failed
    }

    // Prepare a buffer to read the response; only allocate space for the header initially
    std::vector<char> rbuf(kMaxMsg);

    // Read the first 4 bytes to get the length of the incomming message
    if (read_full(fd, rbuf.data(), 4) != 0) {
        return -1;  // Return error if read fails
    }

    // Extract the length of the incomming message from the buffer
    std::memcpy(&len, rbuf.data(), 4);  // little endian assumption

    // Check if the reported length is greater than the maximum allowed size
    if (len > kMaxMsg) {
        std::cerr << "too long" << std::endl;
        return -1;  // Return error if the read fails
    }

    // Read the body of the message into the buffer, starting right after the length prefix
    if (read_full(fd, rbuf.data() + 4, len) != 0) {
        return -1;  // Return error if read fails
    }

    // Null-terminate the received message to safely print it as a C-string
    rbuf[4 + len] = '\0';

    // Print the message received from the server
    std::cout << "server says: " << rbuf.data() + 4 << std::endl;
    return 0;  // Return success
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

        // Making multiple requests
        if (query(fd, "hello1") || query(fd, "hello2") || query(fd, "hello3")) {
            die("Query failed");
        }

        close(fd);  // Close the socket
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
