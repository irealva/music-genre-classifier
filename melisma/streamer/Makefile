
CC=gcc
FLAGS=-g -Wall -Wno-implicit -Wno-unused

OBS = streamer.o
HEADERS = streamer.h

streamer: $(OBS)
	$(CC) $(FLAGS) streamer.c -o streamer

$(OBS): $(HEADERS)

clean:
	/bin/rm streamer
