 /***************************************************************************/
 /*                                                                         */
 /*       Copyright (C) 1996  Daniel Sleator and David Temperley            */
 /*  See file "README" for information about commercial use of this system  */
 /*                                                                         */
 /***************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "harmony.h"

FILE * instream;
int N_notes;
int N_chords;

/* Note ** note_array; */
/* This is an array of pointers to notes.
   it allows us to find the note itself from
   the number of the note

   This is no longer used.
 */

char this_program[20];

Column * column_table;

int table_size;  /* the size of the hash tables */

int baseunit = -1;  /* number of milliseconds in the basic beat */
                    /* -1 indicates it has not been initialized */

Beatlevel beatlevel[MAX_BEAT_LEVEL+1];
int N_beatlevel;

double alpha;  /* exp(-alpha * deltat) is .5 after the half life.
		  when deltat is written in milliseconds */

struct side_effect_struct side_effect;
   /* this structure contains the results returned by tpc_choice_score() */


Beat * global_beat_list;  /* global list of beats */
int N_beats = 0;    /* number of beats */
Beat ** beat_array; /* the array where the beats are stored */

DirectNote * global_DN_list;  /* Global direct note list */

/* settable parameters of the algorithm */


double buckets_per_unit_of_cog = 5.;
   /* discretize cog to this level */

double har_var_factor = 3.0;
/* multiplies the harmonic variance scores */

double half_life = 2.0;
   /* In this many seconds, the relevance of a note to the current note
      goes down by half. This affects the harmonic and tpc variance penalties */

double odp_linear_factor = 3.0;
/* multiplies the stepwise IOI in the ornamental dissonance penalty */

double odp_quadratic_factor = 1.0;
/* multiplies the square of the stepwise IOI in the ornamental dissonance penalty */

double odp_constant = 2.0;
/* constant added to ornamental dissonance penalties */

double sbp_weight = 2.0;
/* multiplies the inverse of the beat interval in the strong beat penalty */

double sbp_constant = 1.5;
/* gets subtracted from the strong beat penalty */

double compat_factor = 1.0;
/* multiplies the strong compatibility scores */

double tpc_var_factor = .3;
   /* multiplies the tpc variance penalties */

double voice_leading_time = 1.0;
/* When the end of a note is followed within this time
   by the start of another note that's 1/2 step away from it,
   then the voice-leading-rule is activated. */

double voice_leading_penalty = 3.0;
/* This is the magnitude of the voice leading penalty,
   when it is incurred. */

double pruning_cutoff = 10.0;
   /* Partial solutions with a current score worse than the optimal
      by more than this are thrown away. */

float compat_value[12] = {-5.0, -5.0, -10.0, 1.0, -3.0, -10.0, 5.0, 3.0, -10.0, -10.0, 2.0, -10.0};
  /*  These are the compatibility values. Each position (0-11) represents a line of fifths position relative
      to the root: position 6 is the root (^1), position 7 is ^5, position 3 is ^3, and so on. A value of
      0.0 is the default value (ornamental) */

int verbosity = 1;
   /* 0 = just the graphical analysis
      1 = above plus note by note feedback
      2 = above plus dump of all data at the end plus beatlevel info at start
      3 = above plus last chord list plus scores chord by chord
      4 = above plus orn dis penalty values plus all chord lists plus masses
    */

int print_tpc_notes = 1;
/* should I print out the special TPC notes on the output */

int print_beats = 1;
/* should I print out the beats (that I read in) */

int print_chords = 1;
/* should I print the chord information */

int prechord_mode = 0;
/* When this is 1, run in prechord mode which prints out prechords,
   and also adjusts the strong beat penalty computation. */

int round_to_beat = 0;
/* should I round notes to the nearest beat?  
   if 0, then truncate to the previous beat */
