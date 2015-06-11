/* This file takes a list of notes and (optionally) beats, and adds a time
value to each timepoint. Run it this way: time-offset [time-value] [input-file]. */

#include <stdio.h>
#include <string.h>

FILE *in_file;
char line[50];
char noteword[10];
int total_duration;
struct {
  int ontime;
  int offtime;
  int duration;
  int pitch;
  int pc;
  int overlap_viol;
} note[5000]; 
int pc_dur[12];
struct {
  int time;
  int level;
} beat[3000];

main(argc, argv)
int argc;
char *argv[];
{
  int i, z=0, b=0;
  float adj_value;

  (void) sscanf (argv[1], "%f", &adj_value);

  in_file = fopen(argv[2], "r");

  while (fgets(line, sizeof(line), in_file) !=NULL) {            /* read in Notes and Beats */
    for (i=0; isspace(line[i]); i++);
    if(line[i] == '\0') continue;
    (void) sscanf (line, "%s", noteword);
    if (strcmp (noteword, "Note") == 0) { 
      (void) sscanf (line, "%s %d %d %d", noteword, &note[z].ontime, &note[z].offtime, &note[z].pitch);
      note[z].ontime = note[z].ontime + adj_value;
      note[z].offtime = note[z].offtime + adj_value;
      printf("Note %d %d %d\n", note[z].ontime, note[z].offtime, note[z].pitch);
      z++;
    }
    if (strcmp (noteword, "Beat") == 0) {   
      (void) sscanf (line, "%s %d %d", noteword, &beat[b].time, &beat[b].level);
      beat[b].time = beat[b].time + adj_value;
      printf("Beat %d %d\n", beat[b].time, beat[b].level);
      b++;
    }
  }
}
