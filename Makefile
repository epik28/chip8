CC = gcc
CFLAGS = -Wall -O3 -march=native -flto `sdl2-config --cflags`
LDFLAGS = `sdl2-config --libs` -lsqlite3 -lm

SRCS = game.c chip8.c db.c fontset.c
OBJS = $(SRCS:.c=.o)
EXEC = game

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(EXEC)

.PHONY: all clean