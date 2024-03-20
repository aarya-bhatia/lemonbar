CC=gcc
CFLAGS=-c -Wall -g -std=c99 -O0 -D_GNU_SOURCE

SRC=$(wildcard src/*.c)
OBJS=$(SRC:src/%.c=obj/%.o)

all: bin/main bin/topbar bin/bottombar

bin/main: $(OBJS)
	mkdir -p bin/
	$(CC) $^ -o $@

bin/topbar: obj/main.o obj/module.o obj/topbar.o
	mkdir -p bin/
	$(CC) $^ -o $@

bin/bottom_bar: obj/main.o obj/module.o obj/bottombar.o
	mkdir -p bin/
	$(CC) $^ -o $@

obj/%.o: src/%.c
	mkdir -p obj/
	$(CC) $(CFLAGS) $< -o $@

clean:
	/bin/rm -rf obj/ bin/
