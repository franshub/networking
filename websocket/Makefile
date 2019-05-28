all: wsserver test-sha1 test-base64
clean:
	-rm *.o
	-rm wsserver
	-rm test-sha1
	-rm test-base64

wsserver: wsserver.o sha1.o base64.o
test-sha1: test-sha1.o sha1.o
test-base64: test-base64.o base64.o

wsserver.o: wsserver.c sha1.h base64.h
sha1.o: sha1.c sha1.h
test-sha1.o: test-sha1.c sha1.h
base64.o: base64.c base64.h
test-base64.o: test-base64.c base64.h
