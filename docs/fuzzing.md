# Apex HTTP Server — Fuzz Testing

## Target

`HttpParser::parse()` — the entry point for all untrusted network input.
Every request the server processes passes through this function first,
making it the highest-value fuzz target in the codebase.

## Tooling

- **libFuzzer** — coverage-guided fuzzing, built into Clang
  (`-fsanitize=fuzzer`)
- **AddressSanitizer** — compiled in alongside libFuzzer
  (`-fsanitize=address`) to catch memory-safety violations (buffer
  overreads/overwrites, use-after-free) that wouldn't otherwise crash
  or produce visibly wrong output

## Harness

`tests/fuzz/http_parser_fuzzer.cpp` hands libFuzzer's mutated byte input
directly to `HttpParser::parse()`:

    extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
        std::string raw(reinterpret_cast<const char*>(data), size);
        HttpRequest req = HttpParser::parse(raw);
        (void)req;
        return 0;
    }

The harness doesn't assert on the parse result — a malformed input is
expected to come back with `req.valid == false`. What's being tested is
that no input, however malformed, can trigger a crash, hang, or
sanitizer-detected memory violation.

## Seed corpus

Three valid-ish starting inputs (`corpus/parser/`) give the fuzzer a
sensible base to mutate from, rather than starting purely from noise:

- a basic `GET` request
- a `POST` with a body and matching `Content-Length`
- an `HTTP/1.0` request with an explicit `Connection: keep-alive` header

## Running it

    make fuzz-run

Builds with `clang++ -fsanitize=fuzzer,address` and runs against the seed
corpus for a fixed duration (`-max_total_time=60` by default).

## Results

| Run | Duration | Executions | Crashes | ASan violations |
|---|---|---|---|---|
| Initial | 61s | 1,826,089 | 0 | 0 |

No crashes, hangs, or memory-safety violations were found across 1.8M+
mutated inputs, including binary garbage, malformed/negative-looking
`Content-Length` values, truncated headers, and corrupted method/path/
version fields.

## What this does and doesn't prove

Fuzzing gives strong confidence against a specific class of bug: crashes
and memory-safety violations triggered by malformed input structure. It
does **not** verify logical correctness (that's what the unit tests in
`tests/http_parser_test.cpp` cover), and a clean fuzzing run is not a
formal proof of safety — it means no crashing input was *found* in the
time given, not that none exists.

## Known limitation surfaced by this work

The parser only processes data already present in a single `recv()`
buffer (see `docs/phase4.md`). A request whose body is not fully received
in one read is currently rejected rather than buffered — this is a
correctness/completeness limitation, not a fuzzing-discovered crash, and
is documented separately as deferred work.

## Future work

- Longer fuzzing runs (hours, not seconds) as part of CI, to increase
  code-path coverage
- A second harness targeting `Router::route()` with pre-parsed
  `HttpRequest` structs, to fuzz routing logic independently of parsing