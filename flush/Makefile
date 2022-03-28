.PHONY = all clean

CC = gcc

SRCS := $(wildcard *.c)

all: flush

%: %.o
	${CC} $< -o $@

%.o: %.c
	$(CC) -c $< -o $@

clean:
	rm -v *.o flush

