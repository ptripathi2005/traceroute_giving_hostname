CC=gcc
CFLAGS=-Wall
#CFLAGS=-std=c99 -Wall -pedantic

traceroute: traceroute.c networking.o utils.o
	$(CC) $(CFLAGS) -o $@ $^ -lrt

utils.o: utils.c
	$(CC) $(CFLAGS) -c -o $@ $<

networking.o: networking.c utils.o
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o

distclean: clean
	rm -f traceroute
