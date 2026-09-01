# Phase 4: HTTP Compliance

## Keep-Alive and epoll Concurrency Model

[Your keep-alive/epoll documentation here.]

## HTTP Header Normalization

HTTP header field names are case-insensitive according to HTTP semantics.

The parser therefore stores header keys in lowercase as the canonical
representation.

All header lookups must use the lowercase form:

    req.headers.find("content-length")

rather than:

    req.headers.find("Content-Length")

This is an internal representation detail. Callers should not need to know
or preserve the casing sent by the client.

### Invariant

Header keys stored in `req.headers` are always lowercase.

Any future code that accesses headers by name must use lowercase keys.

## Request Body Parsing Limitation

The current parser processes the data supplied by a single `recv()` call.

Therefore, a request body is only fully supported when the complete body is
already present in the received buffer.

A body may be incomplete when:

- the body is larger than the receive buffer;
- the request arrives across multiple TCP segments;
- `recv()` returns before the complete body has arrived.

The parser currently rejects incomplete bodies rather than silently treating
partial data as a complete request.

Proper support requires incremental parsing / buffering across multiple
`recv()` calls.

This is deferred work and should be addressed as part of the HTTP parser
state-machine work.