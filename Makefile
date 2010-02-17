CC=gcc
CFLAGS=-g -Wall -I/usr/include/libxml2 #-DDEBUG
OBJS=dcloud-client.o geturl.o link.o instance.o realm.o flavor.o image.o \
	instance_state.o storage_volume.o storage_snapshot.o common.o
LIBS=-lxml2 -lcurl

all: dcloud-client

dcloud-client: $(OBJS)
	$(CC) $(CFLAGS) -o dcloud-client $(OBJS) $(LIBS)

clean:
	rm -f *~ *.o dcloud-client
