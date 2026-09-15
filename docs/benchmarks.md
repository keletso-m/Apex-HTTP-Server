# Apex HTTP Server — Benchmark Results

## Hardware
- CPU: Intel Core i7-6600U @ 2.60GHz (2 cores, 4 threads)
- RAM: 8GB
- OS: Ubuntu Linux

## Tool
wrk — https://github.com/wg/wrk

## Results (Final — with keep-alive)

| Scenario | Req/sec | p50 | p75 | p90 | p99 |
|----------|---------|-----|-----|-----|-----|
| 100 connections | 24,823 | 3.83ms | 4.11ms | 4.58ms | 7.22ms |

## Key findings
- Keep-alive connection reuse more than doubled throughput vs Phase 4
  baseline (11,273 → 24,823 req/sec)
- Zero errors, zero timeouts at 100 concurrent connections
- Latency distribution is extremely tight — p99 only 1.9x p50,
  indicating consistent performance with no outliers
- Structured logging at DEBUG level costs ~5,000 req/sec —
  production deployments should use WARN or ERROR

## Perf Analysis

| Counter | Value | Notes |
|---------|-------|-------|
| CPUs utilized | 1.658 / 4 | I/O bound — not CPU bound |
| Context switches | 12,313/sec | Normal for thread pool |
| Page faults | 0 | No memory pressure |
| Instructions/cycle | 0.45 | CPU waiting on syscalls |
| Branch misses | 4.66% | Acceptable |

### Conclusion
Server is I/O bound, not CPU bound. Keep-alive connection reuse is
the single biggest performance lever — eliminating TCP handshake
overhead per request more than doubled throughput. Further gains
would require kernel bypass (io_uring) or HTTP/2 multiplexing.