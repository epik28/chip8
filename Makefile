CC = gcc
CFLAGS = -Wall `sdl2-config --cflags`
LDFLAGS = `sdl2-config --libs`

SRCS = chip8s.c
OBJS = $(SRCS:.c=.o)
EXEC = game.exe

$(EXEC): $(OBJS)
	$(CC) -o $(EXEC) $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJS) $(EXEC)
