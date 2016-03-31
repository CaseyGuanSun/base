G++ = g++
CFLAGS = -g -Wall
LIB = -lpthread -lrt -lnsl -lz -lcurl
INCLUDES = -I../../lib -I../../inc/base -I../../inc/CentralMonitorServer -I../base/jsoncpp
IPCSOCKLIB = ../../lib/libipcsock.a
BASELIB = ../../lib/libbase.a
JSONCPPLIB =../../lib/libjsoncpp.a

OBJ_CENTRALMONITORSERVER = CentralMonitorServer.o KeepAliveThread.o ProcessThread.o \
			ReceiveThread.o ServerEndPoint.o ServerEndpointManager.o 
					
all:CentralMonitorServer clean

.cpp.o:
	$(G++) -c $< $(CFLAGS) $(INCLUDES)

CentralMonitorServer: $(OBJ_CENTRALMONITORSERVER)
	$(G++) -o ../../bin/CentralMonitorServer $(OBJ_CENTRALMONITORSERVER) $(CFLAGS) $(IPCSOCKLIB) $(LIB) $(BASELIB) $(JSONCPPLIB)

.PHONY:clean

clean: 
	rm *.o

