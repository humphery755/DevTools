#
# Makefile for the trace utility.
# 
# M. Tim Jones <mtj@mtjones.com>
#  $ ls
#  instrument.c    test.c
#  $ $ gcc -g -finstrument-functions -finstrument-functions-exclude-file-list=/usr/lib/gcc/x86_64-linux-gnu,/usr/include -lmyinstrument test.c -o test
#  $ ./test
#  $ ls
#  test.c           test           trace.txt
#  $ pvtrace test
#  $ ls
#  graph.dot        test           trace.txt
#  test.c
#  $ dot -Tjpg graph.dot -o graph.jpg
#  $ ls
#  graph.dot        test.c		   test
#  graph.jpg        trace.txt
#  $

CC = g++
CFLAGS=-mcmodel=large

OBJS = trace.o symbols.o stack.o

EXE = pvtrace
LIB = libmyinstrument.so

$(EXE): $(OBJS) $(LIB)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(LIB): instrument.o
	$(CC) $(CFLAGS) -shared -o $@ instrument.o

.c.o:
	$(CC) $(CFLAGS) -Wall -c $<

.cpp.o:
	$(CC) $(CFLAGS) -Wall -c $<

install: pvtrace
	cp $(LIB) /usr/lib
	cp $(EXE) /usr/bin

clean:
	rm -f $(EXE) $(LIB) *.o

