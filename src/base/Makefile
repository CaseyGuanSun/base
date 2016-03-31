G++ = g++
CFLAGS = -g 
INCLUDES = -I../../inc/base -I../../lib -I./jsoncpp
BASELIB = ../../lib/libbase.a

OBJ_BASELIB = Sleep.o Lock.o Event.o Condition.o BaseThread.o Debug.o HttpClient.o sha1.o JsonApi.o

all:libbase.a clean

.cpp.o:
	$(G++) -c $< $(CFLAGS) $(INCLUDES)

libbase.a: $(OBJ_BASELIB)
	ar rv $@ $(OBJ_BASELIB)
	mv -f $@ $(BASELIB)

.PHONY:clean

clean: 
	rm *.o




