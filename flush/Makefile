.PHONY = all clean

CC = gcc
CFLAGS = -std=c11 -pedantic -Wall -Wextra -Wmissing-declarations

OBJS := $(patsubst src/%.c, build/%.o, $(wildcard src/*.c))

all: build/flush

build/flush: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

build/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -r build
