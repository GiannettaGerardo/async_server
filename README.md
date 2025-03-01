A single-threaded asynchronous web server for Linux, written in pure C and C++, using:
- epoll
- custom Promise

ASCII only.

## Run locally
Clone the project and build with CMake

```bash
  git clone https://github.com/GiannettaGerardo/async_server.git
  cd async_server
  cmake --build ./build/
```

Start the server. Parameters are:
1. **<host> -> string**: can be either a domain name or an IPv4 address; 
2. **<port> -> integer**: service port number;
3. **<max_events_number> -> integer**: maximum number of events that epoll can handle.

Examples:
```bash
  ./AsyncBackend locahost 8080 64
  ./AsyncBackend 127.0.0.1 8080 64
```

## License

MIT