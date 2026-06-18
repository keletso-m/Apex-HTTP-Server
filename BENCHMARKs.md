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