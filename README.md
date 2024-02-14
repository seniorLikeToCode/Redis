### My Own Redis-like server

This document provides a comprehensive guide and overview for the Command Server, a server application designed to handle a variety of commands sent by clients over a TCP/IP network. It supports operations like setting and getting key-value pairs, manipulating sorted sets, and handling timeouts and idle connections.

#### Overview

The Command Server listens for incoming TCP connections on a specified port, accepting commands from clients, processing them, and sending back responses. Supported operations include basic key-value storage, sorted set manipulations, and advanced features like key expiration and idle connection handling.

#### Requirements

-   A Unix-like operating system (Linux, macOS)
-   C++ compiler (e.g., g++, clang++)
-   GNU Make or another build system (optional, for building the project)

#### Building

To compile the Command Server, navigate to the project directory and use the following command:

```bash
g++ -Wall -Wextra -O2 -g server.cpp -o server
g++ -Wall -Wextra -O2 -g client.cpp -o client
```

Ensure all dependencies, including the C++ standard library and POSIX threads, are correctly linked.

#### Features

-   **TCP Network Communication**: Listens on a specified port for incoming connections.
-   **Command Processing**: Supports a variety of commands for manipulating key-value pairs and sorted sets.
-   **Non-Blocking I/O**: Utilizes non-blocking sockets and a custom event loop for efficient I/O operations.
-   **Connection Handling**: Manages idle connections with timeouts to free up resources.
-   **Concurrency**: Implements a thread pool for offloading certain operations, enhancing scalability and performance.
-   **Key Expiration**: Supports setting expiration times on keys for automatic deletion.

#### Running the Server

To start the server, run:

```bash
./server
```

The server will listen on port 1234 by default. Ensure no other service is using this port or modify the source code to use a different port.

#### Supported Commands

-   `SET key value`: Sets the value for a key.
-   `GET key`: Retrieves the value for a key.
-   `DEL key`: Deletes a key.
-   `PEXPIRE key milliseconds`: Sets an expiration time on a key.
-   `PTTL key`: Gets the remaining time to live (in milliseconds) of a key.
-   `KEYS`: Lists all keys.
-   `ZADD zset score member`: Adds a member to a sorted set, or updates its score if it already exists.
-   `ZREM zset member`: Removes a member from a sorted set.
-   `ZSCORE zset member`: Retrieves the score associated with the member in a sorted set.
-   `ZQUERY zset score member offset limit`: Queries members in a sorted set based on score and name, with pagination support.

#### Command Syntax and Usage

### 1. SET key value

Sets the `value` for a `key`. If the key already exists, its value is overwritten.

```
./client SET mykey somevalue
```

### 2. GET key

Retrieves the `value` for a `key`. If the key does not exist, the server responds with `(nil)`.

```
./client GET mykey
```

### 3. DEL key

Deletes a `key`. If the key is successfully deleted, the server responds with `(int) 1`, otherwise `(int) 0`.

```
./client DEL mykey
```

### 4. PEXPIRE key milliseconds

Sets an expiration time on a `key` in milliseconds. After the expiration time, the key is automatically deleted. The server responds with `(int) 1` if the timeout was set successfully, otherwise `(int) 0`.

```
./client PEXPIRE mykey 5000
```

### 5. PTTL key

Gets the remaining time to live (in milliseconds) of a `key`. The server responds with the remaining time or `(int) -1` if the key does not have an expiration time.

```
./client PTTL mykey
```

### 6. KEYS

Lists all keys stored on the server. The server responds with an array of keys.

```
./client KEYS
```

### 7. ZADD zset score member

Adds a `member` to a sorted set `zset`, or updates its `score` if it already exists. The server responds with `(int) 1` if the member is a new addition, otherwise `(int) 0`.

```
./client ZADD myzset 1 member1
```

### 8. ZREM zset member

Removes a `member` from a sorted set `zset`. The server responds with `(int) 1` if the member was removed, otherwise `(int) 0`.

```
./client ZREM myzset member1
```

### 9. ZSCORE zset member

Retrieves the `score` associated with the `member` in a sorted set `zset`. If the member exists, the server responds with `(dbl) score`, otherwise `(nil)`.

```
./client ZSCORE myzset member1
```

### 10. ZQUERY zset score member offset limit

Queries members in a sorted set `zset` based on `score` and `name`, with pagination support using `offset` and `limit`. The server responds with an array of members and their scores.

```
./client ZQUERY myzset 0 member1 0 10
```

To use these commands, you can write a client in your preferred programming language that establishes a TCP connection to the server and sends these commands as plain text strings followed by a newline character. The server's response will also be in plain text, following the formats described above.

#### Customization

-   **Port Configuration**: To change the listening port, modify the `addr.sin_port` assignment in the `main` function.
-   **Thread Pool Size**: Adjust the size of the thread pool in the `main` function for optimal performance based on your hardware and workload.

#### Limitations

-   The server assumes a little-endian byte order for network communication.
-   There is a fixed maximum message size limit for sending and receiving data.
-   It is designed primarily for demonstration purposes and may require modifications for production use.
