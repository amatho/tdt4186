.PHONY = all clean

CC = gcc
CFLAGS = -std=c11 -pedantic -Wall -Wextra -Wmissing-declarations

SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)

all: flush

flush: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -v *.o flush

