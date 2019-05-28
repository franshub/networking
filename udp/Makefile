all: udpserver udpclient udpinteractive

clean:
	-rm *.o
	-rm udpserver
	-rm udpclient
	-rm udpinteractive

CFLAGS=-pthread

udpserver: udpserver.c
udpclient: udpclient.c
udpinteractive: udpinteractive.c
