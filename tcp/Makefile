all: tcpserver tcpclient tcpinteractive

clean:
	-rm *.o
	-rm tcpserver
	-rm tcpclient
	-rm tcpinteractive

CFLAGS=-pthread

tcpserver: tcpserver.c
tcpclient: tcpclient.c
tcpinteractive: tcpinteractive.c
