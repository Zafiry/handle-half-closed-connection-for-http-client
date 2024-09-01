# handle-half-closed-connection-for-http-client
This repo contains the following items:
#### 1. connection-pool-demo
A simple demo of a TCP connection pool with logic for checking and clearing half-closed connections. Developed by C.
#### 2. cpython-patch
A patch for python's http lib, which adds the logic for checking and clearing half-closed connections fot HttpConnection Class. It's based on Cpython-3.13.
