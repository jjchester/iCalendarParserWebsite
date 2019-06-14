sCC = gcc
CFLAGS = -Wall -std=c11 -g -pedantic
LDFLAGS = -L../ -Wl

#UNAME Shell Variable
UNAME_S := $(shell uname -s)

# OS X Dependant Variables
ifeq ($(UNAME_S), Darwin)
    LIB := libcal.dylib
# GNU/Linux Depedant Variables
else ifeq ($(UNAME_S), Linux)
    LIB := libcal.so
endif

all: parser

main: list parser bin/main.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o main bin/main.o -lcal

bin/main.o: src/main.c
	$(CC) $(CFLAGS) -Iinclude -c -o bin/main.o src/main.c

list: ../liblist.so
parser: ../libcal.so


../liblist.so: bin/LinkedListAPI.o
	$(CC) -shared -o ../liblist.so bin/LinkedListAPI.o

../libcal.so: bin/CalendarParser.o ../liblist.so
	$(CC) $(LDFLAGS) -shared -o ../$(LIB) bin/CalendarParser.o bin/LinkedListAPI.o 

bin/LinkedListAPI.o: include/LinkedListAPI.h src/LinkedListAPI.c
	$(CC) $(CFLAGS) -Iinclude -c -fpic -o bin/LinkedListAPI.o src/LinkedListAPI.c

bin/CalendarParser.o: include/CalendarParser.h src/CalendarParser.c
	$(CC) $(CFLAGS) -Iinclude -c -fpic -o bin/CalendarParser.o src/CalendarParser.c

clean:
	rm -f bin/*.o ../*.so main  ../*.dylib
