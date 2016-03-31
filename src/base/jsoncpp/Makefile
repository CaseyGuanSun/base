G++ = g++
CFLAGS = -g 
INCLUDES = -I.
BASELIB = ../../../lib/libjsoncpp.a

OBJ_BASELIB = json_reader.o json_value.o json_writer.o

all:libjsoncpp.a clean

.cpp.o:
	$(G++) -c $< $(CFLAGS) $(INCLUDES)

libjsoncpp.a: $(OBJ_BASELIB)
	ar rv $@ $(OBJ_BASELIB)
	mv -f $@ $(BASELIB)

.PHONY:clean

clean: 
	rm *.o
