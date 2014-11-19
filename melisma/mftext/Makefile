CC      =gcc
# CFLAGS  = -g -Wuninitialized -Wall -Wno-implicit -Wno-char-subscripts
CFLAGS =

mftext: mftext.o midifile.o
	$(CC) mftext.o midifile.o -o mftext

mftext.o midifile.o crack.o: midifile.h

clean: 
	rm -f *.o *~
	echo "Project cleaned."
