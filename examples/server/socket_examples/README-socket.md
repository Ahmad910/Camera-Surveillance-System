# Introduction to socket programming in C

This mimimal example consists of two pars of files for TCP and UDP:
a server which replies with a message to the first client that
connects and then exits, and a client which connects to a server and
outputs whatever it receives. (In the UDP case, the client sends a
datagram and the server echoes it back, as there is no connection
established, so the "client" must start by sending a datagram.
Note that the terms "client" and "server" in the UDP case have a
more vague meaning, where the "client" is the one that takes the initiative
by sending the first datagram.)

See "Socket programming in C" for more details.

To build and run the example, do `make run`. See the Makefile for details

An alternative to using simple_tcp_client is to use the telnet program to
connect to the server. To do that, start `simple_tcp_server` in one terminal,
and then type `telnet localhost 5000` in another.

Sometimes, `telnet` is a useful tool to look at or debug text-based protocols
over TCP.


Please note that TCP is connection-based, but not UDP. Thus, when using
UDP, there is nothing like `listen()` or `accept()`. Instead, you simply
send datagrams using `sendto()` or `sendmsg()`, and receive datagrams
with `recvfrom()` or `recvmsg()`. See the manual pages for further information.

