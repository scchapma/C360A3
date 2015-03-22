CC=gcc
CFLAGS=-g -std=c99 -Wall -pedantic
LDFLAGS=-lpthread 

all: diskinfo  disklist

diskinfo: diskinfo.c 
	$(CC) $(CFLAGS) diskinfo.c $(LDFLAGS) -o diskinfo

disklist: disklist.c 
	$(CC) $(CFLAGS) disklist.c $(LDFLAGS) -o disklist


clean:
	-rm -rf *.o
