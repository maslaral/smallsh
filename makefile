C = gcc
CFLAGS = -g -Wall -std=c99

all: smallsh

smallsh: smallsh.o commands.o process.o
	$(CC) $(CFLAGS) -o $@ $^

smallsh.o: smallsh.c

commands.o: commands.c commands.h

process.o: process.c process.h

memCheck:
	valgrind --tool=memcheck --leak-check=yes main

clean:
	rm *.o
	rm smallsh

