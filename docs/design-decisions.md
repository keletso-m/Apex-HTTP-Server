# Apex HTTP Server — Design Decisions

## Why C++ and raw Linux syscalls, not a framework ? 
I wanted to understand what happens beneath the abstractions, how a server is actually built from the ground up, how connections move through the system, and how the different components of the code work together to produce a complete, functioning product.

Rather than relying on a framework to handle these details for me, I chose to work closer to the operating system using C++ and raw Linux syscalls. This forced me to understand the underlying mechanisms involved in networking, sockets, I/O, concurrency, and request handling.

The project was largely inspired by reading about how Nginx works internally and wanting to explore some of those concepts by implementing a server myself.



