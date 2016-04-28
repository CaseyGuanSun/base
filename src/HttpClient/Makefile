GCC=gcc
CFLAGS = -g

INCLUDES = -I../../inc/HttpClient
BASELIB=../../lib/libHttpClient.a

OBJ_HTTPCLIENT=HttpClient.o HttpClientTest.o

all:HttpClientTest libHttpClient.a clean
	
.c.o:
	$(GCC) -c $< $(CFLAGS) $(INCLUDES)

libHttpClient.a:$(OBJ_HTTPCLIENT)
	ar rv $@ $(OBJ_HTTPCLIENT)
	mv -f $@ $(BASELIB)

HttpClientTest:$(OBJ_HTTPCLIENT)
	$(GCC) -o HttpClientTest $^

.PHONY:clean

clean:
	rm *.o

