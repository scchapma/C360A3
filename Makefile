CC=gcc
CFLAGS=-g -std=c99 -Wall -pedantic
LDFLAGS=-lpthread 

all: diskinfo

diskinfo: diskinfo.c 
	$(CC) $(CFLAGS) diskinfo.c $(LDFLAGS) -o diskinfo

clean:
	-rm -rf *.o
