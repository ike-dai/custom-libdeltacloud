CC=gcc
CFLAGS=-g -Wall -I/usr/include/libxml2
OBJS=dcloud-client.o geturl.o links.o instances.o realms.o flavors.o images.o \
	instance_states.o storage_volumes.o storage_snapshots.o
LIBS=-lxml2 -lcurl

all: dcloud-client

dcloud-client: $(OBJS)
	$(CC) $(CFLAGS) -o dcloud-client $(OBJS) $(LIBS)

clean:
	rm -f *~ *.o dcloud-client
