# httpserver
A multithreaded HTTP/1.1 file server written in C.

# Usage
`./httpserver <port>`
serve files in the current directory on specified port.

# Supported Features
- persistent (ie: Keep-Alive) TCP connections
- GET and HEAD requests
- Last-Modified and If-Modified-Since headers

# Future Features???
- timeouts
- gzip compression
- redirects
- POST requests (I've held off because response generation details are application specific)
- range requests (the tricky part here will be dealing with multipart ranges)

# Requirements
- gcc, make
- pthreads

I have tested this on Ubuntu 16.04 LTS and the Linux Subsystem for Windows. 
Warning: there are a few parts of the codebase that make use of nonstandard extensions in glibc ex: timegm(3).
