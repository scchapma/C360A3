CC=gcc
CFLAGS=-g -std=c99 -Wall -pedantic
LDFLAGS=-lpthread 

all: diskinfo  disklist  diskget

diskinfo: diskinfo.c 
	$(CC) $(CFLAGS) diskinfo.c $(LDFLAGS) -o diskinfo

disklist: disklist.c 
	$(CC) $(CFLAGS) disklist.c $(LDFLAGS) -o disklist

diskget: diskget.c 
	$(CC) $(CFLAGS) diskget.c $(LDFLAGS) -o diskget

clean:
	-rm -rf *.o
