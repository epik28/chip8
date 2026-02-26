CC = gcc
CFLAGS = -Wall `sdl2-config --cflags -O3 -march=native -flto`
LDFLAGS = `sdl2-config --libs `

SRCS = game.c 
EXTERNAL_SRC = sql/sqlite.c
OBJS = $(SRCS:.c=.o)
EXEC = game

$(EXEC): $(OBJS)
	$(CC) -o $(EXEC) $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJS) $(EXEC)
