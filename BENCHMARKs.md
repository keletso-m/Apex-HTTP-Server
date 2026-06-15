# Apex HTTP Server — Benchmark Results

## Hardware
- CPU: Intel Core i7-6600U @ 2.60GHz (2 cores, 4 threads)
- RAM: 8GB
- OS: Ubuntu Linux

## Tool
wrk — https://github.com/wg/wrk

## Results (Phase 4)
Command: `wrk -t2 -c100 -d10s --latency http://localhost:8080/`

| Metric | Result |
|--------|--------|
| Requests/sec | 6,470 |
| Transfer/sec | 6.53 MB |
| 50th percentile latency | 1.27ms |
| 75th percentile latency | 1.83ms |
| 90th percentile latency | 2.75ms |
| 99th percentile latency | 339ms |

## Key findings
- Optimal thread count matches logical CPU count (4 threads)
- listen() backlog of 128 reduces dropped connections vs default 10
- epoll event loop replaces blocking accept()
- System RAM pressure (swap usage) severely impacts performance
- File cache adds lock contention overhead on small files — not beneficial
  on this hardware but would help on multi-core servers with large files