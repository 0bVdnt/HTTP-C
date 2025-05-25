# HTTP-C: A Simple HTTP Server in C

HTTP-C is a lightweight, basic HTTP server implemented in C. It's designed to demonstrate fundamental concepts of socket programming, HTTP request parsing, and response generation for specific routes.

## Table of Contents

-   [Features](#features)
-   [Components](#components)
-   [Prerequisites](#prerequisites)
-   [Building the Server](#building-the-server)
-   [Running the Server](#running-the-server)
-   [Usage Examples](#usage-examples)
-   [Makefile Details](#makefile-details)
-   [License](#license)

## Features

*   **HTTP GET Request Handling:** Specifically handles `GET` requests.
*   **Root Path (`/`):** Responds with a `200 OK` status line.
*   **Echo Endpoint (`/echo/<message>`):** Dynamically generates a `200 OK` response with the provided message as the body, demonstrating content negotiation and dynamic content.
*   **404 Not Found:** Sends a `404 Not Found` response for unhandled routes.
*   **405 Method Not Allowed:** Responds with `405 Method Not Allowed` for any HTTP method other than `GET`.
*   **Graceful Shutdown:** Implements a `SIGINT` (Ctrl+C) signal handler for clean server termination.
*   **Request Parsing:** Parses incoming HTTP request lines into structured data (method, route, HTTP version, and arguments).
*   **Basic Error Handling:** Includes error checks for socket operations and memory allocations.

## Components

The project consists of a single C source file:

*   **`main.c`**:
    *   Contains the core logic for setting up the server socket (`setup_server_socket`).
    *   Manages the main server loop, accepting new client connections.
    *   Implements `handle_client` to read client requests, parse them, and send appropriate HTTP responses.
    *   Defines `RequestStruct` and `ResponseStruct` for structured HTTP data.
    *   Includes `parseRequest` for dissecting the initial HTTP request line.
    *   Provides `echoResponse` to construct dynamic responses for the `/echo/` route.
    *   Manages memory allocation and deallocation for request/response data.
    *   Sets up signal handling for graceful shutdown.
    *   Includes helper functions for initializing and freeing server responses.

## Prerequisites

To build and run this server, you will need:

*   A C compiler (e.g., `gcc`, `clang`).
*   `make` utility.
*   Standard C libraries and POSIX network headers (typically available on Unix-like systems).

## Building the Server

Navigate to the project directory containing `main.c` and the `Makefile`, then run one of the following commands:

*   **Standard Build:**
    ```bash
    make
    ```
    This command compiles `main.c` and creates the `http_server` executable.

*   **Debug Build:**
    ```bash
    make debug
    ```
    This compiles the server with debugging symbols (`-g`) and defines the `DEBUG` preprocessor macro, which can be useful for adding conditional debug output in your C code.

## Running the Server

After building, you can start the server using one of the following methods:

*   **Direct Execution:**
    ```bash
    ./http_server
    ```

*   **Using the Makefile `run` target:**
    ```bash
    make run
    ```
    This will first ensure the server is built (if not already), then execute it.

The server will start listening on port `42069`. You will see log messages indicating its status.

To stop the server, press `Ctrl+C`. The server is configured for graceful shutdown using a signal handler.

Example output when starting:

```
Logs from the program will appear here.
Waiting for a client to connect on port 42069...
Press Ctrl+C to stop the server.
```

## Usage Examples

You can use `curl` or a web browser to interact with the server.

1.  **Accessing the Root Path (`/`):**
    ```bash
    curl http://localhost:42069/
    ```
    Expected Server Log: `200 OK Response Sent (root).`
    Expected `curl` Output: `HTTP/1.1 200 OK` (Note: the server only sends the status line for the root path for simplicity).

2.  **Using the Echo Endpoint (`/echo/`):**
    ```bash
    curl http://localhost:42069/echo/hello_world
    ```
    Expected Server Log: `Echo Response Sent.`
    Expected `curl` Output (including headers):
    ```http
    HTTP/1.1 200 OK
    Content-Type: text/plain
    Content-Length: 11

    hello_world
    ```

3.  **Accessing a Non-Existent Path:**
    ```bash
    curl http://localhost:42069/nonexistent
    ```
    Expected Server Log: `404 Not Found Response Sent.`
    Expected `curl` Output: `HTTP/1.1 404 Not Found`

4.  **Using an Unsupported HTTP Method (e.g., POST):**
    ```bash
    curl -X POST http://localhost:42069/
    ```
    Expected Server Log: `405 Method Not Allowed sent.`
    Expected `curl` Output (including headers):
    ```http
    HTTP/1.1 405 Method Not Allowed
    Allow: GET
    ```

## Makefile Details

The provided `Makefile` streamlines the build process:

*   **`all`**: The default target. Compiles `main.c` into `http_server`.
*   **`$(TARGET): $(OBJ)`**: This rule compiles source files (`.c`) into object files (`.o`) and then links them to create the executable. This is beneficial for larger projects as `make` only recompiles changed source files.
*   **`%.o: %.c`**: A generic rule for compiling any `.c` file into its corresponding `.o` object file.
*   **`clean`**: Removes the compiled `http_server` executable and any intermediate `.o` object files.
    ```bash
    make clean
    ```
*   **`debug`**: Compiles the server with additional debugging symbols (`-g`) and defines a `DEBUG` macro. This target first adds `-g -DDEBUG` to `CFLAGS` and then calls `all`.
    ```bash
    make debug
    ```
*   **`run`**: A convenience target that builds the `http_server` (if necessary) and then executes it.
    ```bash
    make run
    ```

---

## License

This project is open source and available under the MIT License. You are free to use, modify, and distribute it.