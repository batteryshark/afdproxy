# AfdProxy

### A SOCKS5-configured syscall hook that allows transparent TCP proxying on Windows for IPv4 and IPv6.

### Why not an LSP/WFP?
#### LSPs and WFP are ok for a lot of cases, unless your library calls the base provider in which case your filter won't work.

### Why not a driver-level or virtual network device?
#### Sometimes losing that context or otherwise being able to negotiate special sockets (e.g. TLS) in specific ways is a dealbreaker.

### Why not IAT hooks on connect(), etc.
#### Not all processes call those Berkeley socket functions. In addition, other patches might screw with your results. It's likely that this hook should minimize invasiveness of your process address space.

### Why in general?
#### For the lulz? Maybe because how sockets work on Windows is a pretty fascinating katamari?

### What next?
#### Unix AF socket support (should be an easy add), HyperV Socket Support, Named Pipe support, etc.

### WAT
#### WAAAT
