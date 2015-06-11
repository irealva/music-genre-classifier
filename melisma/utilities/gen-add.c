/* This program takes a note-beat file as input.  It assigns an
address to every beat: an integer in which every digit represents a
level, indicating the number of beats at that level that has elapsed
since the most recent higher-level beat.  Each note is also assigned
an address, the address of the beat that coincides. (The last digit of
the address is for notes that don't coincide with any beat; these are
given the address of the previous beat, plus a number indicating their
order. This assumes that there aren't more than 9 event-times between
two beats; if there are, all subsequent ones are just assigned an
order number of 9.)

The program outputs the notes with the addresses added: "Note [ontime]
[offtime] [pitch] [address]". (In compiling it you must include the
-lm flag: "cc -lm gen-add.c -o gen-add".)

On the first beat of the list, the highest-level digit is always set
to 1, not 0. (All other digits are initially set to 0 for that beat
except for the one corresponding to the level of the beat.) This is so
that the first address can serve as an indicator of the number of
levels in the file.

Warning: This program assumes that all beats and note-onsets are in
listed in chronological order. (This is required by the code for
labeling notes not coinciding with beats, and also by the special
handling of beat[0].) */

#include <stdio.h>
#include <string.h>
#include <math.h>

FILE *first_file;
char line[100];
char noteword[20];

int numbeats, numnotes;
int verbosity = 0;

struct {
    int time;
    int level;
    int address;
} beat[10000];

struct {
    int ontime;
    int offtime;
    int pitch;
    int address;
} note[10000];

int lowest_level, highest_level;

main(argc, argv)
int argc;
char *argv[];
{
    int b, n, i, j, c, v, s, earliest;

    if(argc==1) first_file = stdin;
    else {
	first_file = fopen(argv[1], "r");
	if (fopen(argv[1], "r") == NULL) {
	    printf("I can't open that file\n");
	    exit(1);
	}
    }

    b=0;
    n=0;

    if(verbosity>1) printf("First file:\n");
    while (fgets(line, sizeof(line), first_file) !=NULL) {            
	if(line[0]=='\n') continue;
	if(sscanf(line, "%s", noteword) !=1) continue;
	if(strcmp (noteword, "Beat") == 0) {
	    sscanf (line, "%s %d %d", noteword, &beat[b].time, &beat[b].level);
	    if(verbosity>1) printf("Beat %d %d\n", beat[b].time, beat[b].level);
	    if(beat[b].level > 4 || beat[b].level < 0) {
		printf("Can't have a beat level greater than 4 or less than 0\n");
		exit(1);
	    }
	    if(beat[b].level < lowest_level) lowest_level = beat[b].level;
	    if(beat[b].level > highest_level) highest_level = beat[b].level;
	    b++;
	}
	if(strcmp (noteword, "Note") == 0) {
	    sscanf (line, "%s %d %d %d", noteword, &note[n].ontime, &note[n].offtime, &note[n].pitch);
	    if(verbosity>1) printf("Note %d %d %d\n", note[n].ontime, note[n].offtime, note[n].pitch);
	    n++;
	}
    }
    numbeats = b;
    numnotes = n;
    if(numbeats == 0) {
	printf("No beats in input file; no addresses can be assigned\n");
	exit(1);
    }
    if(numnotes == 0) {
	printf("No notes in input file; no addresses can be assigned\n");
	exit(1);
    }

    /* Go through gold file, realign everything to start at time 0 */

    earliest = 100000;
    for(n=0; n<numnotes; n++) {
	if(note[n].ontime < earliest) {
	    earliest = note[n].ontime;
	}
    }
    if(earliest != 0) {
	for(n=0; n<numnotes; n++) {
	    note[n].ontime -= earliest;
	    note[n].offtime -= earliest;
	}
	for(b=0; b<numbeats; b++) beat[b].time -= earliest;
    }

    if(verbosity>0) printf("Goldfile offset = %d\n", earliest);

    if(verbosity>1) printf("Highest level found in gold file = %d; lowest level = %d\n", highest_level, lowest_level);

    generate_addresses();

}

generate_addresses() {

    /* Each time you get to a beat, you reset the counter to zero for
    all lower levels, and you increment the counter by 1 for the level
    of the current beat. The address of the beat is then the sum of
    the counter values for all levels, each one multiplied by the
    appropriate power of ten. After assigning an address to every beat,
    we go through the notes and assign each note the address of the
    coinciding beat. */

    int b, v, v2, i, n;
    int c[5];
    int last_note_ontime, last_beat_time, next_beat_time, nonbeat_note_counter;

    for(i=lowest_level; i<=highest_level; i++) c[i]=0;
    for(b=0; b<numbeats; b++) beat[b].address = 0;

    for(b=0; b<numbeats; b++) {
	for(v=lowest_level; v<=highest_level; v++) {
	    if(beat[b].level > v) {
		c[v]=0;
	    }
	    if(beat[b].level == v) {
		c[v]++;
		i=10;
		for(v2=lowest_level; v2<=highest_level; v2++) {
		    beat[b].address += i * c[v2];
		    i*=10;
		}
	    }
	}
	if(b==0 && c[highest_level]==0) {
	    c[highest_level]=1;
	    beat[0].address += pow(10, highest_level+1); 
	}

	/*	printf("Address: %d ", beat[b].address);
		if(beat[b].address < 100000) printf("0");
	if(beat[b].address < 10000) printf("0");
	if(beat[b].address < 1000) printf("0");
	if(beat[b].address < 100) printf("0");
	if(beat[b].address < 10) printf("0");
	printf("%d\n", beat[b].address); */
    }

    for(n=0; n<numnotes; n++) {
	for(b=0; b<numbeats; b++) {
	    if(note[n].ontime == beat[b].time) note[n].address = beat[b].address;
	}
    }

    /* Go through and find any notes that have not yet been assigned addresses (i.e. notes that don't
       coincide with beats). */
    for(b=-1; b<numbeats; b++) {
	if(b==-1) last_beat_time = -1;
	else last_beat_time = beat[b].time;
	if(b==numbeats-1) next_beat_time = 10000000;
	else next_beat_time = beat[b+1].time;
	last_note_ontime = -1;
	nonbeat_note_counter = 0;
	for(n=0; n<numnotes; n++) {
	    if(note[n].ontime > last_beat_time && note[n].ontime < next_beat_time) {
		if(note[n].ontime > last_note_ontime) nonbeat_note_counter++;
		if(nonbeat_note_counter > 9) nonbeat_note_counter = 9;
		note[n].address = beat[b].address + nonbeat_note_counter;
		last_note_ontime = note[n].ontime;
		if(verbosity>0) printf("Nonbeat note found: Note %d %d %d %d\n", note[n].pitch, note[n].ontime, note[n].offtime, note[n].address);
	    }
	}
    }

    for(n=0; n<numnotes; n++) {
	printf("ANote %d %d %d %6d\n", note[n].ontime, note[n].offtime, note[n].pitch, note[n].address);
    }
}

