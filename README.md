# edison-sockets
very simple example of how to use sockets in C

# building
``` bash
mkdir build
cd build
cmake ..
cmake --build .
```

# running
start the server at a particular port and then run the client. you should see the client's message echoed back by the server.

after building the server and client programs will exist in the build directory.

*server*
```./server <portnumber> [--hostname <hostname>]```
```./server 42310```
```./server 42310 --hostname localhost```

*client
```./client <portnumber> [--hostname <hostname>] [--message <message>]```
```./client 42310```
```./client 42310 --hostname localhost```
```./client 42310 --message "this is a much bigger and longer message to send than just \"hello world\". isn't that neat?"```
```./client 42310 --message "using dots in my hostname 0_0" --hostname 127.0.0.1```
