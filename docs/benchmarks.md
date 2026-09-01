# Apex HTTP Server — Benchmark Results

## Hardware
- CPU: Intel Core i7-6600U @ 2.60GHz (2 cores, 4 threads)
- RAM: 8GB
- OS: Ubuntu Linux

## Tool
wrk — https://github.com/wg/wrk

## Results (Phase 4 — Final)

| Scenario | Req/sec | p50 | p90 | p99 |
|----------|---------|-----|-----|-----|
| 100 connections | 11,273 | 0.64ms | 1.48ms | 510ms |
| 400 connections | 11,913 | — | — | — |

## Key finding
Structured logging to stdout+file was the single biggest bottleneck,
costing ~5,000 req/sec. In production, log level should be set to
WARN or ERROR, not DEBUG.

## Perf Analysis (Phase 4)

| Counter | Value | Notes |
|---------|-------|-------|
| CPUs utilized | 1.658 / 4 | I/O bound — not CPU bound |
| Context switches | 12,313/sec | Normal for thread pool |
| Page faults | 0 | No memory pressure |
| Instructions/cycle | 0.45 | CPU waiting on syscalls |
| Branch misses | 4.66% | Acceptable |

### Conclusion
Server is I/O bound, not CPU bound. Bottleneck is kernel network
syscalls (accept, recv, send), not application logic. Architecture
is correct epoll + thread pool is the right approach. Further
gains require kernel bypass (io_uring) or keep-alive connections,
both beyond Phase 4 scope.
