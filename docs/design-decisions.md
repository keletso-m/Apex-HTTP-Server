# Apex HTTP Server — Design Decisions

## Why C++ and raw Linux syscalls, not a framework ? 
I wanted to understand what happens beneath the abstractions, how a server is actually built from the ground up, how connections move through the system, and how the different components of the code work together to produce a complete, functioning product.

Rather than relying on a framework to handle these details for me, I chose to work closer to the operating system using C++ and raw Linux syscalls. This forced me to understand the underlying mechanisms involved in networking, sockets, I/O, concurrency, and request handling.

The project was largely inspired by reading about how Nginx works internally and wanting to explore some of those concepts by implementing a server myself.

# Concurrency model: epoll + thread pool
Apex uses epoll for I/O event notification combined with a fixed-size worker thread pool of four threads, rather than creating a dedicated thread for every connection.

- **Why not thread-per-connection**: A dedicated thread for every connection introduces significant memory and scheduling overhead. Each thread requires its own stack and incurs context-switching costs, making the model increasingly inefficient as the number of concurrent connections grows.
- **Why epoll over select/poll**: epoll is designed for handling large numbers of file descriptors efficiently. Instead of repeatedly scanning the entire set of connections for readiness, it provides the application with the descriptors that are ready for I/O. This avoids the O(n) scanning overhead associated with select and poll as the number of connections increases.
- **Thread pool size**: Apex uses four worker threads to process work concurrently without creating and destroying threads for individual connections. Keeping the pool fixed provides predictable resource usage while allowing multiple requests to be processed in parallel.

## Rate limiting
Global atomic counter with a fixed 1-second window (see `RateLimiter`).

- **Known tradeoff**: the shared `std::atomic` counter creates cross-core
  cache-line contention under high concurrency, confirmed via benchmarking
  (see Benchmarks section below).
- **Planned improvement**: per-thread sharded counters (total limit divided
  across threads) to remove the shared atomic entirely, at the cost of the
  limit becoming approximate rather than exact.

  ## Logging
Synchronous logger writing to both a log file and, above a configurable
threshold, stdout.

- **Bug found and fixed**: the logger originally wrote every entry to
  stdout unconditionally, regardless of level. This caused up to a 3x
  throughput drop whenever a terminal was attached and rendering
  output, worker threads blocked on synchronous terminal writes under load.
  Fixed by adding a separate `console_level_` threshold (default WARN),
  decoupling file logging from console logging.
- **Known limitation**: still synchronous, a future iteration should
  move logging off the request-handling thread entirely (queued/async
  writer), so log I/O speed can never affect request latency.

## HTTP parsing
The parser only processes data already present in a single `recv()`
buffer. Requests whose body isn't fully received in one read are
rejected rather than buffered.
- **Why**: The initial implementation assumes that the request body is available within a single read operation, which keeps the parsing and state-management logic simpler and easier to reason about while establishing the core server functionality.
- **Known limitation**: The initial implementation assumes that the request body is available within a single read operation, which keeps the parsing and state-management logic simpler and easier to reason about while establishing the core server functionality.

## Metrics
Exposes `/metrics` in Prometheus text-exposition format
(`apex_requests_total`, `apex_uptime_seconds`, `apex_active_connections`)
rather than a custom JSON schema or a bundled dashboard UI.

- **Why**: matches how real infrastructure (nginx, most production
  services) exposes metrics, plain text scraped by Prometheus, then
  visualized in Grafana,  rather than reinventing observability tooling
  that already exists and is better.


