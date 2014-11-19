CC      =gcc
CFLAGS  = -g -O -Wuninitialized -Wall
LIBS    = -lm

OBS	= main.o read-input.o chords.o harmony.o display.o globals.o misc.o
HEADERS	= harmony.h

harmony: $(OBS)
	$(CC) $(CFLAGS) $(OBS) $(LIBS) -o harmony

$(OBS):	$(HEADERS)

clean: 
	rm -f *.o *~
	echo "Project cleaned."
