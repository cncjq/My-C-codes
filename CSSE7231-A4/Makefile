CC=gcc
CFLAGS= -Wall -pedantic -std=gnu99 -g
LDFLAGS= -L/local/courses/csse2310/lib -I/local/courses/csse2310/include -pthread -lcsse2310a3 -lstringmap -lcsse2310a4
LIBCFLAGS= -fPIC -Wall -pedantic -std=gnu99

all: psclient psserver libstringmap.so
psclient: psclient.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< 

psserver: psserver.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< 

stringmap.o: stringmap.c
	$(CC) $(LDFLAGS) -c $<

libstringmap.so: stringmap.o
	$(CC) -shared -o $@ stringmap.o

clean:
	rm -f psclient
	rm -f psserver
	rm -f stringmap.o
	rm -f libstringmap.so


