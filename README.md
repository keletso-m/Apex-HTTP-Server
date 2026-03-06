# Apex HTTP Server

A high-performance, production-grade HTTP/1.1 server built from scratch in C++ using Linux system calls.

> ⚠️ **Status:** Active Development  
> This project is being built incrementally. Check the [Development Roadmap](#roadmap) for current progress.

---

## Project Goals

Build a professional HTTP server that demonstrates:
- Deep understanding of network programming and TCP/IP
- Modern C++ development practices
- Linux system internals (epoll, threading, signals)
- Production-grade observability and reliability
- Performance optimization and benchmarking

**This is not a framework wrapper — every line is written from scratch.**

---

## Features

### Current Features
- [ ] TCP socket server with connection handling
- [ ] HTTP/1.1 request parsing
- [ ] Static file serving
- [ ] Multi-threaded request handling
- [ ] Configuration file support
- [ ] Structured logging

### Planned Features
- [ ] Thread pool with work queue
- [ ] epoll-based event loop
- [ ] Prometheus metrics endpoint
- [ ] Rate limiting
- [ ] TLS/SSL support
- [ ] Reverse proxy mode
- [ ] Docker deployment
- [ ] Graceful shutdown

---

## Architecture
```
┌─────────────────────────────────────────┐
│         Client Requests                 │
└──────────────┬──────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────┐
│     Connection Acceptor                 │
│     (epoll event loop)                  │
└──────────────┬──────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────┐
│         Thread Pool                     │
│  ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐      │
│  │ W1  │ │ W2  │ │ W3  │ │ W4  │      │
│  └─────┘ └─────┘ └─────┘ └─────┘      │
└──────────────┬──────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────┐
│      Request Handler                    │
│  • Parse HTTP                           │
│  • Route request                        │
│  • Generate response                    │
└─────────────────────────────────────────┘
```

---

## Quick Start

### Prerequisites
- Linux (Ubuntu 20.04+ or similar)
- GCC 9+ or Clang 10+ (C++17 support)
- Make
- OpenSSL (for TLS support, optional)

### Build
```bash
git clone https://github.com/yourusername/apex-http-server.git
cd apex-http-server
make
```

### Run
```bash
./bin/apex-server --config config/server.conf
```

Default server runs on `http://localhost:8080`

### Test
```bash
# In another terminal
curl http://localhost:8080/

# Or use your browser
open http://localhost:8080/
```

---

## Project Structure
```
apex-http-server/
├── src/
│   ├── core/           # Server core (socket, epoll)
│   ├── http/           # HTTP protocol handling
│   ├── handlers/       # Request handlers
│   ├── utils/          # Logging, config, etc.
│   └── main.cpp        # Entry point
├── include/            # Header files
├── tests/              # Unit tests
├── config/             # Configuration files
├── www/                # Static files to serve
├── docs/               # Documentation
├── Makefile
├── LICENSE
└── README.md
```

---

## Configuration

Example `config/server.conf`:
```ini
[server]
port = 8080
host = 0.0.0.0
threads = 4
max_connections = 1000

[paths]
document_root = ./www
log_file = ./logs/server.log

[performance]
keep_alive_timeout = 60
request_timeout = 30
```

---

## Performance

*Benchmarks will be added as development progresses*

Target performance:
- 50,000+ requests/sec on modern hardware
- < 1ms median latency for static files
- Support 10,000+ concurrent connections

---

## Testing
```bash
# Run unit tests
make test

# Run integration tests
make integration-test

# Load testing
make benchmark
```

---

## Development

### Build Options
```bash
make              # Build release version
make debug        # Build with debug symbols
make clean        # Clean build artifacts
make test         # Run tests
make benchmark    # Run performance tests
```

### Code Style

- C++17 standard
- Follow Google C++ Style Guide
- Use smart pointers (RAII)
- Comprehensive error handling

---

## Learning Resources

This project was built by learning from:
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- [The Linux Programming Interface](https://man7.org/tlpi/)
- HTTP/1.1 RFC 7230-7235
- [epoll man pages](https://man7.org/linux/man-pages/man7/epoll.7.html)

---

## Development Roadmap {#roadmap}

### Phase 1: Foundation
- [x] Project setup
- [x] Basic TCP server
- [x] Accept connections
- [x] Echo server test
- [x] HTTP request parsing
- [x] Static file serving

### Phase 2: Concurrency 
- [ ] Thread pool implementation
- [ ] Request queue
- [ ] Thread-safe logging
- [ ] Graceful shutdown

### Phase 3: Production Features 
- [ ] Configuration system
- [ ] Structured logging
- [ ] Error handling
- [ ] Request routing

### Phase 4: Performance 
- [ ] epoll integration
- [ ] Benchmarking suite
- [ ] Performance profiling
- [ ] Memory optimization

### Phase 5: Deployment 
- [ ] Docker container
- [ ] systemd service
- [ ] Monitoring (Prometheus)
- [ ] Documentation

---

## Contributing

This is a learning project, but feedback and suggestions are welcome!

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

---

## License

MIT License - see [LICENSE](LICENSE) file for details

---

## Author

**[keletso monyamane]**
- GitHub: [@keletso-m](https://github.com/keletso-m)

---

## Acknowledgments
Built as a learning project to understand:
- Network programming fundamentals
- Linux system programming
- High-performance server architecture
- Production engineering practices

---

**⭐ If you find this project interesting, consider starring the repo!**
