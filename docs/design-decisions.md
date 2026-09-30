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



