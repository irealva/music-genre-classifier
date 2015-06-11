/* This file takes a note list as input, and adjusts the tempo. 

Run it like this: tempo-adjust [-m/-t] [value] [input-file]. If the
first flag is "-m" (multiply mode), the next value is a multiplicative
value; the events are then outputted with all the time points
multiplied by the tempo adjustment value. If the first flag is "-t",
the following value represents a metronome number; the input is
assumed to have tactus=60 (1000 milliseconds), and in the output, the
timepoints are adjusted to have the new tactus. */

#include <stdio.h>
#include <string.h>

FILE *in_file;
char line[50];
char noteword[10];
char mode_command[10];
int total_duration;
struct {
  int ontime;
  int offtime;
  int duration;
  int pitch;
  int pc;
  int overlap_viol;
} note[3000]; 
struct {
  int time;
  int level;
} beat[3000];

int pc_dur[12];

main(argc, argv)
int argc;
char *argv[];
{
  int i, b=0, z=0, metronome_value, mode, command_value;
  float multiply_value, adj_value;
  mode = 1;

  if(argc<4) {
    printf("Usage: Specify mode (1 or 0), command value, then file name\n");
    exit(1);
  }

  (void) sscanf (argv[1], "%s", mode_command);    
  printf("mode command: %s\n", mode_command);
  if(strcmp(mode_command, "-m")==0) mode=0;
  else if(strcmp(mode_command, "-t")==0) mode=1;
  else {
    printf("Usage: Specify mode (1 or 0), command value, then file name\n");
    exit(1);
  }
  
  (void) sscanf (argv[2], "%d", &command_value);
  if(mode==0) adj_value = command_value;
  if(mode==1) adj_value = 60.0 / command_value;

  in_file = fopen(argv[3], "r");
  while (fgets(line, sizeof(line), in_file) !=NULL) {            /* read in Notes and Beats */
    for (i=0; isspace(line[i]); i++);
    if(line[i] == '\0') continue;
    (void) sscanf (line, "%s", noteword);
    if (strcmp (noteword, "Note") == 0) { 
      (void) sscanf (line, "%s %d %d %d", noteword, &note[z].ontime, &note[z].offtime, &note[z].pitch);
      note[z].ontime = note[z].ontime * adj_value;
      note[z].offtime = note[z].offtime * adj_value;
      printf("Note %d %d %d\n", note[z].ontime, note[z].offtime, note[z].pitch);
      z++;
    }
    if (strcmp (noteword, "Beat") == 0 && line[0] == 'B') {   
      (void) sscanf (line, "%s %d %d", noteword, &beat[b].time, &beat[b].level);
      beat[b].time = beat[b].time * adj_value;
      printf("Beat %d %d\n", beat[b].time, beat[b].level);
      b++;
    }
  }
}
