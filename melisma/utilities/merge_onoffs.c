/* This file takes a notelist of "note-on/note-off" format and turns it into one of "note" format.

It goes through the event list, as inputted. For each note-on it finds, it scans the list until it
finds a note-off of a later time, and it creates a "note" statement with those two events. So
this

Gets turned into two events, Note 0 500 60 and Note 400 500 60. On the other hand, this

          Note-on 0 60
	  Note-off 500 60
	  Note-on 400 60
	  Note-off 1000 60

...gets turned into Note 0 500 60 and Note 0 400 60.
*/

#include <stdio.h>
#include <string.h>


FILE *in_file;
char line[50];
char badline[50];
char noteword[10];
int s, z=0;
struct {
  int time;
  int pitch;
  char word[8];
  int now_ignore;
} statement[10000];
struct {
  int ontime;
  int offtime;
  int pitch;
} note[5000]; 

main(argc, argv)
int argc;
char *argv[];
{
    in_file = fopen(argv[1], "r");
    for (s = 0; s <= 10000 && fgets(line, sizeof(line), in_file) !=NULL; ++s) {
      (void) sscanf (line, "%s", badline);
      if (strcmp(badline, "Note-on") == 0 || strcmp(badline, "Note-off") ==0) {
	(void) sscanf (line, "%s %d %d", statement[s].word, &statement[s].time, &statement[s].pitch);
      }
    }
    merge_onoffs();
}

merge_onoffs() {
     int x, y, found;
     for (x = 0; x < s; ++x) {     
       found=0;
       if (strcmp(statement[x].word, "Note-on") == 0 && statement[x].now_ignore == 0) {
	 for (y = x+1; y < s; ++y) {
	   if (statement[y].pitch == statement[x].pitch && strcmp(statement[y].word, "Note-off") == 0 &&
	       statement[y].time > statement[x].time) {           

	     /* Make sure the hypothetical off-time is later than the on-time (otherwise, it might be the offtime of
		a previous event at the same pitch, which is at the same time as the current ontime but listed afterwards) */

	     note[z].ontime = statement[x].time;
	     note[z].offtime = statement[y].time;
	     note[z].pitch = statement[x].pitch;
	     /* if (strcmp(statement[y].word, "Note-on") == 0) {
		statement[y].now_ignore = 1;
	     } */
	     printf("Note %d %d %d\n", note[z].ontime, note[z].offtime, note[z].pitch);
	     ++z;
	     found=1;
	     break;
	   }
	 }
	 if (found==0) printf("No off event found for an on event of pitch %d at time %d\n", statement[x].pitch,
			      statement[x].time);
       }
     }
}
