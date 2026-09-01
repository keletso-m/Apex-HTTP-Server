# Apex HTTP Server

A high-performance, production-grade HTTP/1.1 server built from scratch in C++ using Linux system calls.Inspired by Nginx.

> **Status:** v1.0.0 released
> Core HTTP compliance, concurrency, testing, and deployment are complete. See the [Development Roadmap](#development-roadmap) below. TLS/SSL and reverse proxy mode are planned for a future release.

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

- [x] TCP socket server with epoll-based connection handling
- [x] HTTP/1.1 request parsing (case-insensitive headers, `Content-Length`-based body parsing)
- [x] Static file serving with content-type detection
- [x] Multi-threaded request handling (thread pool)
- [x] Configuration file support
- [x] Structured logging
- [x] HTTP correctness: HEAD support, accurate `Content-Length`, correct status codes
- [x] Keep-alive with epoll-based connection reuse (no thread pinned per idle connection)
- [x] Connection idle timeout
- [x] Graceful shutdown (SIGINT/SIGTERM close idle connections, drain in-flight requests)
- [x] SIGPIPE-safe (a client disconnecting mid-response can't crash the server)
- [x] Explicit resource limits (max header size, URI length, body size)
- [x] Global rate limiting (`429 Too Many Requests`)
- [x] Prometheus-compatible `/metrics` endpoint
- [ ] TLS/SSL support *(planned, next release)*
- [ ] Reverse proxy mode *(planned, future release)*

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
│  ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐        │
│  │ W1  │ │ W2  │ │ W3  │ │ W4  │        │
│  └─────┘ └─────┘ └─────┘ └─────┘        │
└──────────────┬──────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────┐
│      Request Handler                    │
│  • Parse HTTP                           │
│  • Rate limit check                     │
│  • Route request                        │
│  • Generate response                    │
└──────────────┬──────────────────────────┘
               │
               ▼
       keep-alive? ── yes → re-arm connection fd in epoll
                            (EPOLLONESHOT), worker returns to pool
                    └─ no  → close connection
```

Key design decisions:

- **epoll-based keep-alive**, not a blocking-`recv()`-per-thread model — an
  idle keep-alive connection doesn't pin a worker thread, so the thread
  pool's size bounds concurrent *active* work, not concurrent *open*
  connections.
- **Global (not per-IP) rate limiting** — a fixed-window atomic counter
  shared across all connections. Simpler and lock-light; the tradeoff is
  that one aggressive client can affect the limit for others. See
  [docs/phase4.md](docs/phase4.md) for more detail on this and other Phase 4
  design decisions.
- **Header keys are normalized to lowercase internally**, so lookups are
  case-insensitive regardless of what the client sends.

---

## Quick Start

### Prerequisites
- Linux (Ubuntu 20.04+ or similar)
- GCC 9+ (C++17 support)
- Clang (only required for `make fuzz-run`)
- Make
- Docker (optional, for containerized deployment)

### Build
```bash
git clone https://github.com/keletso-m/Apex-HTTP-Server.git
cd Apex-HTTP-Server
make
```

### Run
```bash
./bin/apex-server config/server.conf
```

If no config path is given, or the file isn't found, built-in defaults are
used. Default server runs on `http://localhost:8080`.

### Test it
```bash
curl http://localhost:8080/health
curl http://localhost:8080/metrics
```

---

## Endpoints

| Path | Method | Description |
|---|---|---|
| `/health` | GET, HEAD | Liveness check, returns `200 OK` |
| `/metrics` | GET, HEAD | Prometheus exposition-format metrics |
| `/*` | GET, HEAD | Static file serving from `document_root` |

---

## Project Structure
```
apex-http-server/
├── src/
│   ├── core/           # Server core (socket, epoll, thread pool, router, config)
│   ├── http/            # HTTP protocol handling (parser, request/response)
│   ├── handlers/        # Request handlers (static file serving)
│   ├── utils/            # Logging
│   └── main.cpp          # Entry point
├── include/              # Header files
├── tests/                # Unit + integration tests (GoogleTest)
│   └── fuzz/              # libFuzzer harness for the HTTP parser
├── corpus/               # Seed inputs for fuzz testing
├── config/               # Configuration files
├── www/                  # Static files to serve
├── docs/                 # Phase writeups, benchmarks, fuzzing results
├── Dockerfile
├── .dockerignore
├── apex-http-server.service   # systemd unit file
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
backlog = 10
max_connections = 1000

[paths]
document_root = ./www
log_file = ./logs/server.log

[performance]
keep_alive_timeout = 60
request_timeout = 30
rate_limit_per_second = 1000
```

---

## Performance

See [docs/benchmarks.md](docs/benchmarks.md) for full `wrk` results and
`perf` analysis. Summary: the server is I/O-bound, not CPU-bound —
kernel network syscalls dominate over application logic, and structured
logging was found to be the largest single throughput cost (~5,000
req/sec) at DEBUG level.

---

## Testing

```bash
make test        # unit + integration tests (GoogleTest)
make fuzz-run     # fuzz test the HTTP parser (requires clang, 60s by default)
make benchmark    # load testing (requires wrk)
```

See:
- [docs/fuzzing.md](docs/fuzzing.md) — fuzz testing methodology and results
  (1.8M+ mutated inputs, zero crashes or memory-safety violations)
- [docs/benchmarks.md](docs/benchmarks.md) — load testing and perf analysis

---

## Deployment

### Docker

Multi-stage build — compiles in a full build image, ships only the
compiled binary and runtime assets in a slim final image, runs as a
non-root user.

```bash
docker build -t apex-http-server .
docker run -p 8080:8080 apex-http-server
```

### systemd

Runs the native binary as a managed system service under a dedicated
system user, with automatic restart on failure and filesystem hardening.

```bash
sudo useradd --system --no-create-home --shell /usr/sbin/nologin apex
sudo mkdir -p /opt/apex-http-server
sudo cp -r bin config www /opt/apex-http-server/
sudo mkdir -p /opt/apex-http-server/logs
sudo chown -R apex:apex /opt/apex-http-server

sudo cp apex-http-server.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now apex-http-server
```

### Monitoring

`/metrics` exposes Prometheus exposition-format text — point a Prometheus
scrape config at `http://<host>:8080/metrics`.

---

## Known Limitations

- **Body parsing requires the full body in one `recv()` call.** Requests
  whose bodies span multiple TCP reads are currently rejected rather than
  buffered. See [docs/phase4.md](docs/phase4.md) for detail — proper
  support requires incremental/streaming parsing, tracked as future work.
- **No TLS/SSL yet.** Plaintext HTTP only, for now.
- **Rate limiting is global, not per-client.** One high-volume client can
  affect the limit available to others.

---

## Development

### Build Options
```bash
make              # Build release version
make debug        # Build with debug symbols, no optimization
make clean        # Clean build artifacts
make test         # Run unit + integration tests
make fuzz-run      # Fuzz test the HTTP parser
make benchmark    # Run load tests (wrk)
```

### Code Style

- C++17 standard
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

## Development Roadmap

### Phase 1: Foundation
- [x] Project setup
- [x] Basic TCP server
- [x] Accept connections
- [x] Echo server test
- [x] HTTP request parsing
- [x] Static file serving

### Phase 2: Concurrency
- [x] Thread pool implementation
- [x] Request queue
- [x] Thread-safe logging
- [x] Graceful shutdown

### Phase 3: Production Features
- [x] Configuration system
- [x] Structured logging
- [x] Error handling
- [x] Request routing

### Phase 4: HTTP Compliance
- [x] HTTP status code correctness
- [x] HEAD support
- [x] Keep-alive support
- [x] Connection timeout
- [x] SIGPIPE handling
- [x] Content-Length-respecting body parsing, lowercase header normalization
- [x] Explicit request size limits
- [x] Graceful shutdown
- [x] Global rate limiting

See [docs/phase4.md](docs/phase4.md) for detailed design notes on the
keep-alive/epoll model, header normalization, and body parsing.

### Phase 5: Performance
- [x] epoll integration
- [x] Benchmarking suite
- [x] Performance profiling
- [x] Memory optimization

See [docs/benchmarks.md](docs/benchmarks.md) for full results.

### Phase 6: Testing
- [x] Unit tests for router
- [x] HTTP parser tests
- [x] Integration tests
- [x] Load tests
- [x] Fuzz testing

See [docs/fuzzing.md](docs/fuzzing.md) for fuzz testing results.

### Phase 7: Deployment
- [x] Docker container
- [x] systemd service
- [x] Monitoring (Prometheus)
- [x] Documentation

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

**Keletso Monyamane**
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