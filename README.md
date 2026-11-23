# HT_web_server

A multithreaded HTTP/1.1 static file server written from scratch in C++ using raw
sockets. No networking library or framework is used.

I made this project to learn how the web works: how a file travels from a server to a
client, how HTTP works on top of TCP, and how a backend handles many clients at the
same time. I had already built a data structures library from scratch in C++, so I
decided to build this one from scratch in C++ as well.

## Features

- Parses HTTP requests (method, URI, headers, body)
- Supports `GET` and `HEAD`; other methods get `405`
- Serves files from the `public/` folder with the correct MIME type
- Worker thread pool with a bounded connection queue
- Sends `503 Service Unavailable` when the queue is full, so the server does not
  take unlimited load
- Graceful shutdown on Ctrl+C: stops accepting, finishes queued connections, then
  prints how many connections were accepted and rejected
- Limits on request size (`431`, `413`) and `400` for bad requests
- Blocks path traversal (`../` in the URL)

## Build and run

You need a C++14 compiler, CMake and Linux (it uses pthreads).

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/server
```

Run the server from the project folder, because it looks for the `public/` folder
in the place where you start it. By default it listens on port 8080, so open
http://localhost:8080 in your browser or try `curl -i http://localhost:8080/`.

You can change the settings with flags:

```bash
./build/server --port 9000 --workers 2 --queue 16
```

- `--port`: the port to listen on (default 8080)
- `--workers`: how many worker threads handle requests (default 4)
- `--queue`: how many connections can wait for a free worker (default 128). If more
  clients come, they get a 503

Press Ctrl+C to stop it. It finishes the connections already in the queue and then
prints how many connections it accepted and rejected.

## How it works

### Sockets

A socket is an endpoint for communication. Actually it is a data structure stored in
the kernel. When a program calls `socket()`, the OS creates this structure in the
kernel and returns a file descriptor to the process, which the process uses to refer
to the socket. The protocol (TCP or UDP) is chosen in this call. This server uses TCP.

The server then does these steps:

1. `socket()` creates the socket.
2. `bind()` attaches an IP address and a port number to it.
3. `listen()` marks it as a socket that waits for incoming connections.
4. `accept()` takes one incoming client and returns a **new** file descriptor for
   that client. The first socket keeps listening, so one server can talk to many
   clients, on the same PC or on another one.

### Multithreading

If one thread did everything, a slow client would block all the others. So the main
thread only accepts connections, and worker threads handle them. They run
concurrently, and in parallel if the CPU has more than one core.

The main thread does not give a connection directly to a worker. It puts the
connection in a **bounded queue**, and the workers take connections from it:

```
 clients --> accept() --> bounded queue --> worker threads
              (main)       (mutex +          recv -> parse -> find file -> send
                            condition var)
                |
                +-- queue full: send 503 and close
```

For each connection a worker:

1. reads the request from the socket,
2. parses it (`HTTPRequest`),
3. finds the file and builds the response (`StaticFileHandler`, `HTTPResponse`),
4. sends the response and closes the connection.

The queue is bounded so memory does not grow without limit. When it is full, the
client gets a clear `503` and can try again later.

### Project structure

| File | What it does |
|---|---|
| `main.cpp` | Command line flags and signal handling |
| `src/HT_server.cpp` | Sockets, accept loop, queue, worker threads, shutdown |
| `src/HTTPRequest.cpp` | Request parsing |
| `src/HTTPResponse.cpp` | Building the response |
| `src/StaticFileHandler.cpp` | Maps URL to file, MIME types, path check |
| `include/ScopedSocket.hpp` | Closes the socket automatically (RAII) |

## Testing and results

I tested with ApacheBench on my own machine (Intel i5-12450H, Fedora, GCC 15.2.1).
Client and server were on localhost and the page was 209 bytes. Full tables and raw
outputs are in [TESTING.md](testing.md) and the `benchmarks/` folder.

**Speed:** about 38K requests/s with 100 concurrent connections and 1 worker, p99
latency 3 ms, 0 failed requests (median of 3 runs). More workers made it slower, about
18% lower at 16 workers, because each request is so small that extra threads only add
waiting for the lock.

**Overload (2 workers, 200 clients at once):** the queue size decides how many
requests get a 503. Speed stayed about the same.

| Queue size | 1 | 4 | 8 | 32 | 128 | 512 |
|---|---|---|---|---|---|---|
| Requests rejected (503) | 45.4% | 23.4% | 22.4% | 5.2% | 0.5% | 0% |

**Other checks:**
- Ran with AddressSanitizer, UBSan and ThreadSanitizer (also sending Ctrl+C during load). No errors were reported.
- `tests/test_server.sh` checks status codes, HEAD, path traversal, and bad or too large requests.

These numbers are for a very small file on localhost, so they show how the server
handles connections and overload. They do not show real network or large file speed.

## Limitations

- No keep-alive: the connection is closed after every response
- The full file is read into memory before sending
- A worker handles one connection at a time, so many slow clients can block all workers (`epoll` would fix this)
- Only HTTP is implemented, no HTTPS/TLS
- Symlinks inside `public/` are followed
- Not tested yet: large files, slow clients, long runs
