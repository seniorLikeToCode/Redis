#include <arpa/inet.h>   // For network byte order conversion functions
#include <assert.h>      // For runtime diagnostics
#include <errno.h>       // For error numbers
#include <fcntl.h>       // For file control options
#include <netinet/ip.h>  // Definitions for internet protocol support
#include <poll.h>        // For polling to check for events on file descriptors
#include <stdint.h>      // For fixed-width integer types
#include <stdio.h>       // For standard input and output
#include <stdlib.h>      // For standard library functions, including memory allocation
#include <string.h>      // For string manipulation functions
#include <sys/socket.h>  // For socket-specific data types and functions
#include <unistd.h>      // For POSIX API
#include <vector>        // For the dynamic array `std::vector`

// Function to print messages to stderr
static void msg(const char* msg) {
    fprintf(stderr, "%s\n", msg);
}

// Function to terminate the program with an error message
static void die(const char* msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

// Function to set a file descriptor (socket) to non-blocking mode
static void fd_set_nb(int fd) {
    errno = 0;
    int flags = fcntl(fd, F_GETFL, 0);
    if (errno) {
        die("fcntl error");
        return;
    }

    flags |= O_NONBLOCK;

    errno = 0;
    (void)fcntl(fd, F_SETFL, flags);
    if (errno) {
        die("fcntl error");
    }
}

const size_t k_max_msg = 4096;  // Maximum message size

// Enum for tracking the state of each connection
enum {
    STATE_REQ = 0,  // Waiting for request
    STATE_RES = 1,  // Sending response
    STATE_END = 2,  // Connection to be closed
};

// Structure representing a connection
struct Conn {
    int fd = -1;         // File descriptor for the connection
    uint32_t state = 0;  // Current state of the connection
    // Buffer for incoming data
    size_t rbuf_size = 0;
    uint8_t rbuf[4 + k_max_msg];
    // Buffer for outgoing data
    size_t wbuf_size = 0;
    size_t wbuf_sent = 0;
    uint8_t wbuf[4 + k_max_msg];
};

// Adds a connection to the fd2conn mapping
static void conn_put(std::vector<Conn*>& fd2conn, struct Conn* conn) {
    if (fd2conn.size() <= (size_t)conn->fd) {
        fd2conn.resize(conn->fd + 1);
    }
    fd2conn[conn->fd] = conn;
}

// Accepts a new connection and adds it to the fd2conn mapping
static int32_t accept_new_conn(std::vector<Conn*>& fd2conn, int fd) {
    struct sockaddr_in client_addr = {};
    socklen_t socklen = sizeof(client_addr);
    int connfd = accept(fd, (struct sockaddr*)&client_addr, &socklen);
    if (connfd < 0) {
        msg("accept() error");
        return -1;  // error
    }

    fd_set_nb(connfd);  // Make the new connection non-blocking
    struct Conn* conn = (struct Conn*)malloc(sizeof(struct Conn));
    if (!conn) {
        close(connfd);
        return -1;
    }
    conn->fd = connfd;
    conn->state = STATE_REQ;
    conn->rbuf_size = 0;
    conn->wbuf_size = 0;
    conn->wbuf_sent = 0;
    conn_put(fd2conn, conn);
    return 0;
}

// Forward declaration of state handling functions
static void state_req(Conn* conn);
static void state_res(Conn* conn);

// Tries to process a request from the connection's buffer
static bool try_one_request(Conn* conn) {
    if (conn->rbuf_size < 4) {
        return false;  // Not enough data for a length prefix
    }
    uint32_t len = 0;
    memcpy(&len, &conn->rbuf[0], 4);
    if (len > k_max_msg) {
        msg("too long");
        conn->state = STATE_END;
        return false;
    }
    if (4 + len > conn->rbuf_size) {
        return false;  // Not enough data for the full message
    }

    printf("client says: %.*s\n", len, &conn->rbuf[4]);

    memcpy(&conn->wbuf[0], &len, 4);
    memcpy(&conn->wbuf[4], &conn->rbuf[4], len);
    conn->wbuf_size = 4 + len;

    size_t remain = conn->rbuf_size - 4 - len;
    if (remain) {
        memmove(conn->rbuf, &conn->rbuf[4 + len], remain);
    }
    conn->rbuf_size = remain;

    conn->state = STATE_RES;
    state_res(conn);

    return (conn->state == STATE_REQ);
}

// Tries to fill the connection's read buffer with data from the socket
static bool try_fill_buffer(Conn* conn) {
    assert(conn->rbuf_size < sizeof(conn->rbuf));
    ssize_t rv = 0;
    do {
        size_t cap = sizeof(conn->rbuf) - conn->rbuf_size;
        rv = read(conn->fd, &conn->rbuf[conn->rbuf_size], cap);
    } while (rv < 0 && errno == EINTR);
    if (rv < 0 && errno == EAGAIN) {
        return false;  // No data available right now
    }
    if (rv < 0) {
        msg("read() error");
        conn->state = STATE_END;
        return false;
    }
    if (rv == 0) {
        msg("EOF");
        conn->state = STATE_END;
        return false;
    }

    conn->rbuf_size += (size_t)rv;

    while (try_one_request(conn)) {
    }
    return (conn->state == STATE_REQ);
}

// Handles the request state for a connection
static void state_req(Conn* conn) {
    while (try_fill_buffer(conn)) {
    }
}

// Tries to flush the connection's write buffer to the socket
static bool try_flush_buffer(Conn* conn) {
    ssize_t rv = 0;
    do {
        size_t remain = conn->wbuf_size - conn->wbuf_sent;
        rv = write(conn->fd, &conn->wbuf[conn->wbuf_sent], remain);
    } while (rv < 0 && errno == EINTR);
    if (rv < 0 && errno == EAGAIN) {
        return false;  // Cannot write more data right now
    }
    if (rv < 0) {
        msg("write() error");
        conn->state = STATE_END;
        return false;
    }
    conn->wbuf_sent += (size_t)rv;
    if (conn->wbuf_sent == conn->wbuf_size) {
        conn->state = STATE_REQ;
        conn->wbuf_sent = 0;
        conn->wbuf_size = 0;
        return false;
    }
    return true;
}

// Handles the response state for a connection
static void state_res(Conn* conn) {
    while (try_flush_buffer(conn)) {
    }
}

// Processes I/O for a connection based on its current state
static void connection_io(Conn* conn) {
    if (conn->state == STATE_REQ) {
        state_req(conn);
    } else if (conn->state == STATE_RES) {
        state_res(conn);
    } else {
        assert(0);  // Invalid state
    }
}

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);  // Create a listening socket
    if (fd < 0) {
        die("socket()");
    }

    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));  // Allow socket reuse

    struct sockaddr_in addr = {};  // Bind the socket to a local address
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);      // Listen on port 1234
    addr.sin_addr.s_addr = ntohl(0);  // Listen on any interface
    int rv = bind(fd, (const sockaddr*)&addr, sizeof(addr));
    if (rv) {
        die("bind()");
    }

    rv = listen(fd, SOMAXCONN);  // Start listening for connections
    if (rv) {
        die("listen()");
    }

    std::vector<Conn*> fd2conn;  // Map of connections indexed by file descriptor

    fd_set_nb(fd);  // Make the listening socket non-blocking

    std::vector<struct pollfd> poll_args;  // Array of pollfd structures for poll()
    while (true) {
        poll_args.clear();
        struct pollfd pfd = {fd, POLLIN, 0};  // Listen for incoming connections
        poll_args.push_back(pfd);
        for (Conn* conn : fd2conn) {
            if (!conn) continue;
            struct pollfd pfd = {};
            pfd.fd = conn->fd;
            pfd.events = (conn->state == STATE_REQ) ? POLLIN : POLLOUT;
            pfd.events |= POLLERR;  // Also listen for errors
            poll_args.push_back(pfd);
        }

        int rv = poll(poll_args.data(), (nfds_t)poll_args.size(), 1000);  // Poll for events
        if (rv < 0) {
            die("poll");
        }

        for (size_t i = 1; i < poll_args.size(); ++i) {
            if (poll_args[i].revents) {
                Conn* conn = fd2conn[poll_args[i].fd];
                connection_io(conn);
                if (conn->state == STATE_END) {
                    fd2conn[conn->fd] = NULL;
                    close(conn->fd);
                    free(conn);
                }
            }
        }

        if (poll_args[0].revents) {
            accept_new_conn(fd2conn, fd);  // Accept new connections if possible
        }
    }

    return 0;
}
