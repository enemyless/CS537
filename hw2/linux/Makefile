
CC = gcc
CFLAGS = -Wall -g -std=c99

sqysh: sqysh.o
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) -c -o $@ $(CFLAGS) $<

.PHONY: clean
clean:
	rm -f sqysh.o sqysh
