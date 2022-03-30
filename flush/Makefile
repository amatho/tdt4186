.PHONY = all clean

CC = gcc

SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)

all: flush

flush: $(OBJS)
	${CC} $^ -o $@

%.o: %.c
	$(CC) -c $< -o $@

clean:
	rm -v *.o flush

