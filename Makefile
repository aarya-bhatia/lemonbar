CC=gcc
CFLAGS=-c -Wall -g -std=c99 -O0 -D_GNU_SOURCE

SRC=src/main.c src/module.c src/log.c
OBJS=$(SRC:src/%.c=obj/%.o)

all: bin/topbar bin/bottombar

bin/topbar: obj/topbar.o $(OBJS)
	mkdir -p bin/
	$(CC) $^ -o $@

bin/bottombar: obj/bottombar.o $(OBJS)
	mkdir -p bin/
	$(CC) $^ -o $@

obj/%.o: src/%.c
	mkdir -p obj/
	$(CC) $(CFLAGS) $< -o $@

clean:
	/bin/rm -rf obj/ bin/
