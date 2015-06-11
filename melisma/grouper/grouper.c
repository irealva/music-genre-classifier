
 /***************************************************************************/
 /*                                                                         */
 /*       Copyright (C) 2000 Daniel Sleator and David Temperley             */
 /*           See http://www.link.cs.cmu.edu/music-analysis                 */
 /*        for information about commercial use of this system              */
 /*                                                                         */
 /***************************************************************************/ 

/* This program takes a monophonic stream of notes and groups them
into phrases. As input, it requires a note list with statements of the
form "Note [ontime] [offtime] [pitch]", as well as beat list with
statements of the form "Beat [time] [level]".

A grouping analysis is simply a series of group boundaries between
notes.  The program uses three criteria to evaluate an
analysis. First, it calculates a "gap score" for each pair of
notes. The gap score is the sum of the inter-onset interval and the
offset-to-onset interval for the two notes.  The gap scores are
weighted by the mean IOI of all the notes so far (including the
current one). The gap of the current note (i.e.  before the current
note) is 500 * (IOI+OOI) / (mean IOI of previous notes).

Second, the program assigns a penalty to each group, depending on its
length in terms of number of notes. Groups with 8 notes receive a
penalty of zero; deviations from this value in either direction incur
penalties.  The penalty is logarithmic, so that the penalty for a
16-note group is the same as for a 4-note group, but it's also
weighted by the length of the group. Finally, each group gets a
penalty depending on the metrical position of its beginning, relative
to the metrical position of the previous phrase's beginning. (Metrical
position is defined as the number of beats between the previous level
3 beat and the beat of the phrase beginning.) If the two phrases do
not have the same beginning, a penalty is assigned (the penalty is
all-or-nothing). A similar (smaller) penalty is assigned for "phase"
at metrical level 4.

With verbosity=0, the output of the program simply consists of the
note list, with "phrase" statements indicating the location of phrase
boundaries.  (Each phrase statement has a timepoint - the ontime of
the note following the phrase boundary.)  If verbosity=1, a graphic
display will be printed.  If verbosity=2, both note list and display
will be shown as well as other information.

This program also contains a way of testing the program against phrase
information from another source. This can be controlled with the
global variable "mode". If "mode" is set to 1, testing will be done;
if mode = 0, the output will simply be as described above.  The
testing is done as follows. In the input data, phrase boundaries can
be represented with a line inserted in the note list at the location
of the boundary, containing only "|". The program reads in these
externally-given phrase boundaries. In outputting the note list, the
program prints out its own boundaries with "Phrase" statements as
described above.  In addition, however, if the program locates one of
its own boundaries at a location where there is no external boundary,
it prints out "FP" (false positive). If it omits a boundary where
there is an external boundary, it prints "FN".  (The "testing" mode
will only work when the note list is being printed out: i.e. verbosity
= 0 or 2.)

*/



#include <stdio.h>
#include <string.h>
#include <math.h>

FILE *in_file;
FILE *out_file;
char line[50];

int verbosity = 0;
int mode = 0;
float optimal_length = 10.0;
int gap_weight = 500;
int length_weight = 600;
int phase_4_penalty = 150;
int phase_3_penalty = 300;
int mark_first_phrase = 0;

char eventword[10];
int display_command;
int numnotes, numbeats;
int total_duration;
int final_timepoint;
struct {
  int time;
  int level;
  int phase;
} beat[5000];

typedef struct note_struct {
  int ontime;
  int offtime;
  int duration;
  int pitch;
  int beat;
  int phase;
  int gap;
  int input_phrase;
} blah;

struct note_struct note[10000];

int period;

int best[10000];
double score[10000][20];
double analysis[10000];
int final[10000];

void bad_param(char * line) {
  char * x;
  x = strchr(line, '\n');
  if (x != NULL) *x = '\0';
  printf("Warning: cannot interpret \"%s\" in parameters file -- skipping it.\n", line);
}

void read_parameter_file(char *filename, int file_specified) {
    char line[100];
    char part[3][100];
    int i;
    float value;
    FILE *param_stream;
    
    param_stream = fopen(filename, "r");
    if (param_stream==NULL) {
      if (file_specified) {
	printf("Warning: cannot open \"%s\".  Using defaults.\n", filename);
      } else {
	/*      printf("Cannot open \"%s\".  Using defaults.\n", filename);       */
      }
      return;
    }
    while(fgets(line, sizeof(line), param_stream) !=NULL) {
      for (i=0; isspace(line[i]); i++);
      if (line[i] == '%' || line[i] == '\0') continue;  /* ignore comment and blank lines */
      for (i=0; line[i] != '\0'; i++) if (line[i] == '=') line[i] = ' '; /* kill all '=' signs */
      if (sscanf(line, "%99s %99s %99s", part[0], part[1], part[2]) != 2) {
	bad_param(line);
	continue;
      }
      if (sscanf(part[1], "%f", &value) != 1) {
	bad_param(line);
	continue;
      }
      
      if (strcmp(part[0], "verbosity") == 0) {
	verbosity = value;
      } else if (strcmp(part[0], "mode") == 0) {
	mode = value;
      } else if (strcmp(part[0], "optimal_length") == 0) {
	optimal_length = value;
      } else if (strcmp(part[0], "gap_weight") == 0) {
	gap_weight = value;
      } else if (strcmp(part[0], "length_weight") == 0) {
	length_weight = value;
      } else if (strcmp(part[0], "phase_4_penalty") == 0) {
	phase_4_penalty = value;
      } else if (strcmp(part[0], "phase_3_penalty") == 0) {
	phase_3_penalty = value;
      } else if (strcmp(part[0], "mark_first_phrase") == 0) {
	mark_first_phrase = value;
      } 
    }      
    fclose(param_stream);
}

void assign_beats() {

  /* Here we assign beats to notes. We assign each note a beat number, which is simply the b number of the coinciding
   beat.

   We also assign a phase value to each beat. Each beat of level 4 (3?) or higher has a phase value of 0, then we count
   upwards on subsequent beats until we get to the next high-level beat. This leaves the beats before the first
   high-level beat with no phase value, so we find the second high-level beat, look at the phase value of the beat 
   before that ("upbeatphase") and then we go back to the beat before the first high-level beat and count backwards
   starting with "upbeatphase". */

  int b, z, p=0, firstbigbeat=0, secondbigbeat=0, upbeatphase, i=0;

  /* First assign phase numbers to beats */

  for(b=0; b<numbeats; b++) {
    if(beat[b].level>=4) {
      beat[b].phase=0;
      p=0;
    }
    else {
      p++;
      beat[b].phase=p;
    }
  }
  for(b=0; b<numbeats; b++) {
    if(beat[b].level>=4 && i==0) {
      firstbigbeat=b;
      /* printf("firstbigbeat = %d\n", b); */
      i=1;
    }
    else if(beat[b].level>=4 && i==1) {
      secondbigbeat=b;
      /* printf("secondbigbeat = %d\n", b); */
      i=2;
    }
  }
  upbeatphase=beat[secondbigbeat-1].phase;
  i=0;
  for(b=firstbigbeat-1; b>=0; b--) {
    beat[b].phase=upbeatphase-i;
    i++;
  }
  period=secondbigbeat-firstbigbeat;

  /* Now assign beat numbers and phase numbers to notes. */

  for(b=0; b<numbeats; b++) {
    /* printf("Beat %d at time %d with level %d has phase %d\n", b, beat[b].time, beat[b].level, beat[b].phase); */
    for(z=0; z<numnotes; z++) {
      if(beat[b].time==note[z].ontime) {
	note[z].phase=beat[b].phase;
	note[z].beat=b;    /* This isn't actually used for anything */
      }
    }
  }

  /* We assign note[numnotes].beat the beat at the offtime of the final note. */
  for(b=0; b<numbeats; b++) {
    if(beat[b].time==note[numnotes-1].offtime) note[numnotes].phase=beat[b].phase;
  }
}


void calculate_gap_scores() {

  int average, z;
  /* Now we assign the gap scores */

  for (z=0; z<numnotes; z++) {
    if(z==0) note[z].gap=0;
    if(z>=1) {
      average=note[z].ontime / z+1; 

      /* We're calculating the gap score between note z-1 and note z. "average" is the mean IOI of all notes so far. 
	 This includes the current gap (from z-1 to z). This avoids dividing by zero on the first gap (from note 0 to 1). */

      note[z].gap = gap_weight * ((note[z].ontime-note[z-1].ontime) + (note[z].ontime-note[z-1].offtime)) / average;
    }
    /* printf("%d %d %d: gap score = %d\n", note[z].ontime, note[z].offtime, note[z].pitch, note[z].gap);  */
  }
  note[numnotes].gap=0;
}

void analyze_piece() {
  int z, i, best_score, length_penalty, phase_penalty;
  best[0]=0;
  analysis[0]=0.0;
  for(z=1; z<=numnotes; z++) {
    best_score=-1000000.0;
    for(i=1; z-i>=0 && i<20; i++) {
      length_penalty = fabs(length_weight * (3.32*log10(i/optimal_length))); 

      /* 3.32*log10(x) = log base 2 of X...So this takes the absolute value of log base 2 of the ratio between 
	 the length of the phrase and the optimal phrase length); so the value is 0  if i=optimal length, then gets 
	 higher as you deviate in either direction. f(2*OPL)=1, f(.5*OPL)=1. */

      if(note[z].phase == note[z-i].phase) phase_penalty = 0.0;              /* They're in phase at levels 3 and 4 */
      else {                                                                  
	if (note[z].phase - (period/2) == note[z-i].phase || note[z].phase + (period/2) == note[z-i].phase) {
	   phase_penalty = phase_4_penalty;                                            /* They're in phase at level 3 only */
	}
	else phase_penalty=phase_3_penalty;                                            /* They're not in phase at level 3 or 4 */
      }

      /* The penalty for having a boundary at z and a boundary at z-i is given by: the sum of half the gap scores
	 at z and i, plus the length penalty, plus the phase penalty, all multiplied by sqrt(i) (so as to avoid the
	 problem of analyses with more phrases having more penalties) */ 
      score[z][i]=analysis[z-i] + ((( (note[z-i].gap / 2) + (note[z].gap / 2) - 
				      length_penalty) - phase_penalty) * sqrt(i));

      if(score[z][i]>best_score) {
	best_score=score[z][i];
	analysis[z]=score[z][i];
	best[z]=i;
      }
    }
    /* printf("The best i for note %d (pitch %d) is %d with score %6.3f\n", z, note[z].pitch, best[z], analysis[z]);  */
  }

  for(z=0; z<=numnotes; z++) {
    final[z]=0;
  }

  final[numnotes]=1;
  z=numnotes;
  while(z>0) {
    final[z-best[z]]=1;
    z=z-best[z];
  }
  final[0]=1;
}

void print_notelist() {
  int z, i, pp, falsepos=0, falseneg=0;
  int length_penalty;
  if(verbosity>1) {
    for(z=1; z<=numnotes; z++) {
      if(final[z]==1) {
	length_penalty = fabs(length_weight * (3.32*log10(best[z]/optimal_length)));  
	printf("Phrase ending note %d: ", z); 
	printf("length penalty = %d; ", length_penalty);
	printf("total score = %6.1f\n", analysis[z]-analysis[z-best[z]]);
      }
    }
  } 

  for (z=0; z<numnotes; z++) {

    if(final[z]==0 && note[z].input_phrase==1) {
      falseneg++;
      if(mode==1) printf("(FN)\n");
    }
    if(final[z]==1 && (z!=0 || mark_first_phrase==1)) {
      printf("Phrase %d ", note[z].ontime);
      if(mode==1 && note[z].input_phrase==0) {
	printf("(FP) ");
	falsepos++;
      }
      printf("\n");
    }

    printf("Note %d %d %d", note[z].ontime, note[z].offtime, note[z].pitch);
    if(verbosity>0) printf(": gap score = %d; phase = %d", note[z].gap, note[z].phase);
    printf("\n");
  }
  /* printf("End %d phase %d\n", note[numnotes-1].offtime, note[numnotes].phase); */
  if(mode==1) printf("%d false negatives, %d false positives\n", falseneg, falsepos);
}

void print_graphic() {
  int z, i, b, L, currentnote, currentpitch, newnote;
  currentnote=-1;
  currentpitch=-1;

  printf("                C1          C2          C3          C4          C5          C6          C7\n");

  for(b=0; b<numbeats; b++) {
    printf("%6d ", beat[b].time);
    for(L=0; L<5; L++) {
      if(L<=beat[b].level) printf("x ");
      else printf("  ");
    }
    for(z=0; z<numnotes; z++) {
      if(note[z].beat==b) {
	currentnote=z;
	currentpitch = note[z].pitch;
	newnote=1;
	break;
      }
    }
    if(note[currentnote].offtime <= beat[b].time) currentpitch=-1;
    for(i=24; i<=108; i++) {
      if(i==currentpitch) {
	if(newnote==1) {
	  printf("+");
	}
	else printf("|");
      }
      else if(i%12==0) printf(".");
      else {
	if (newnote==1 && final[currentnote]==1 && i%2==0) printf("-");
	else printf(" ");
      }
    }
    printf("\n");
    if(newnote==1) newnote=0;
  }
}

int main(argc, argv)
int argc;
char *argv[];
{
    int j, i, N_parts, z=0, b=0, newphrase=0;

    char *parameter_file, *input_file = NULL;
    int param_file_specified = 0;
    
    parameter_file = "parameters";
    
    for (j=1; j<argc; j++) {
      if (strcmp(argv[j], "-p") == 0) {
	parameter_file = argv[j+1];
	param_file_specified = 1;
	j++;
      } else if (input_file == NULL) {
	/* assume it's a file */
	input_file = argv[j];
      }
    }
    
    read_parameter_file (parameter_file, param_file_specified);

    if (input_file != NULL) {
      in_file = fopen(input_file, "r");
      if (in_file == NULL) {
	printf("I can't open that file\n");
	exit(1);
      }
    } else {
      in_file = stdin;
    }
    
    while (fgets(line, sizeof(line), in_file) !=NULL) {            /* read in Notes and Beats */
      for (i=0; isspace(line[i]); i++);
      if(line[i] == '\0') continue;
      (void) sscanf (line, "%s", eventword);
      if (strcmp (eventword, "Note") == 0) {  
	N_parts = sscanf (line, "%s %d %d %d", eventword, &note[z].ontime, &note[z].offtime, &note[z].pitch);
	if(N_parts != 4) {
	  printf("Bad line: %s", line);
	  exit(1);
	}
	if(z>0 && note[z].ontime < note[z-1].ontime) {
	  printf("Error: notes at %d and %d are not listed in chronological order\n", note[z].ontime, note[z-1].ontime);
	  exit(1);
	}
	if(z>0 && note[z].ontime == note[z-1].ontime) {
	  printf("Error: two simultaneous note-onsets at %d\n", note[z].ontime);
	  exit(1);
	}
	if(z>0 && note[z].ontime < note[z-1].offtime) note[z-1].offtime=note[z].ontime;
	/* If two notes overlap, adjust the offtime of the first one back to the ontime of the second one */
	note[z].duration = note[z].offtime - note[z].ontime;
      
	/* printf("%d %d %d\n", note[z].ontime, note[z].offtime, note[z].pitch);  */
	total_duration = total_duration + note[z].duration;
      
	if (newphrase==1) note[z].input_phrase=1;
	else note[z].input_phrase=0;
	newphrase=0;
	
	++z;
	
      }
      if (strcmp (eventword, "|") == 0 && line[0] == '|') {
	newphrase=1;
      }    
      if (strcmp (eventword, "Beat") == 0) { 
	N_parts = sscanf (line, "%s %d %d", eventword, &beat[b].time, &beat[b].level);
	if(N_parts != 3) {
	  printf("Bad line: %s", line);
	  exit(1);
	}
	if(b>0 && beat[b].time <= beat[b-1].time) {
	  printf("Error: beats at %d and %d are not listed in chronological order\n", beat[b].time, beat[b-1].time);
	  exit(1);
	}
	b++;
      }
    }
    /*    printf("the number of events is %d\n", z);           */
    /*	  printf("total duration is %d\n", total_duration);    */
    
    numnotes = z;
    numbeats = b;

    /* Initialize all phase values to zero. If there are no beats in the input, phase values will remain at zero and
       no phase penalties will be assigned */
    for(z=0; z<numnotes; z++) note[z].phase = 0;  

    if(numbeats > 0) assign_beats();
    calculate_gap_scores();
    analyze_piece();
    if(verbosity!=1) print_notelist();
    if(verbosity!=0) {
      if(numbeats==0) printf("The graphic display cannot be shown; no beats in input\n");
      else print_graphic();
    }
    return 0;
}
