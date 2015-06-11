/* This program takes a file consisting of a series of notes and rests
with duration and pitch, like this:

Note 100 60
Note 100 62
Note 200 64
Note 100 67
Rest 100
Note 300 67

("Rest" statements in the input file are rests; they have simply a
duration, no pitch.)  The program "concatenates" them, giving each
note an ontime and offtime and adding it on to the previous note:

Note 0 100 60
Note 100 200 62
Note 200 400 64
Note 400 500 67
Note 600 900 67

The global variable TIME_FACTOR multiplies all the time points.

*/

#include <stdio.h>
#include <string.h>

#define TIME_FACTOR 1

FILE *in_file;
char line[50];
char noteword[10];

struct {
  int ontime;
  int offtime;
  int duration;
  int pitch;
} note[10000]; 
struct {
  int time;
  int level;
} beat[10000];
int rest_duration;

main(argc, argv)
int argc;
char *argv[];
{
  int i, b=0, z=0, total_duration=0;
  in_file = fopen(argv[1], "r");
  while (fgets(line, sizeof(line), in_file) !=NULL) {            /* read in Notes and Beats */
    for (i=0; isspace(line[i]); i++);
    if(line[i] == '\0') continue;
    (void) sscanf (line, "%s", noteword);
    if (strcmp (noteword, "N") == 0) { 
      (void) sscanf (line, "%s %d %d", noteword, &note[z].duration, &note[z].pitch);
      printf("Note %d %d %d\n", total_duration*TIME_FACTOR, (note[z].duration+total_duration)*TIME_FACTOR, note[z].pitch);
      total_duration += note[z].duration;
    }
    if (strcmp (noteword, "R") == 0) { 
      (void) sscanf (line, "%s %d", noteword, &rest_duration);
      total_duration += rest_duration;
    }
  }
}
