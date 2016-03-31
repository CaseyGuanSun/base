GCC=gcc
CFLAGS = -g

INCLUDES = -I../../inc/HttpServer -I/usr/local/include/
BASELIB=../../lib/libHttpServer.a

OBJ_HTTPSERVER=HttpServer.o

all:libHttpServer.a HttpServerTest clean
	
.c.o:
	$(GCC) -c $< $(CFLAGS) $(INCLUDES)

libHttpServer.a:$(OBJ_HTTPSERVER)
	ar rv $@ $(OBJ_HTTPSERVER)
	mv -f $@ $(BASELIB)

HttpServerTest:HttpServerTest.o $(OBJ_HTTPSERVER)
	$(GCC) -o $@ $(CFLAGS) $(INCLUDES)  $^  -lpthread -lrt -lnsl -lz -ldl /usr/local/lib/libevent.a /usr/local/lib/libevent_core.a  /usr/local/lib/libevent_extra.a /usr/local/lib/libevent_openssl.a 

.PHONY:clean

clean:
	rm *.o
