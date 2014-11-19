
 /***************************************************************************/
 /*                                                                         */
 /*       Copyright (C) 2000 Daniel Sleator and David Temperley             */
 /*           See http://www.link.cs.cmu.edu/music-analysis                 */
 /*        for information about commercial use of this system              */
 /*                                                                         */
 /***************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "meter.h"

FILE * instream;
int N_notes;

int N_pips;
Pip * pip_array;

Note ** note_array;
/* This is an array of pointers to notes.
   it allows us to find the note itself from
   the number of the note */

char this_program[20]; /* the name of this program */

/* settable parameters of the algorithm */

int verbosity = 0;
int terse = 0;                   /* if this equals 1, terse has been declared on command line */
int pip_time = 35;                  /* the pip time in milliseconds */
double beat_interval_factor = 3.0;  /* multiplier on the penalty for the
				       difference between successive beats */
double note_factor = 1.0;           /* multiply the ioi by this to score
				       a note. */


double tactus_min   = 400;            /* the bottom of the lowest tactus range */
double tactus_width = 1.8;            /* the width of the tactus window (max/min) */
double tactus_step  = 1.1;            /* The next range up is obtained by multiplying the
					 previous range by this (both max and min) */
double tactus_max   = 1600;           /* never exceed this as the top end */

int beat_slop       = 35;             /* the beat interval pentalty is zero if the
					 times of the beats differ by less than
					 this.  It's in milliseconds.  */

double meter_change_penalty = 0.3;    /* charge for changing the number of
					 beats in a measure */

double raising_change_penalty = 3.0;  /* charge for changing the number of base level
					 beats between two consecutive higher level beats
					 Used by level raising algoritm. */

double duple_bonus = .2;              /* For the level lowering algorithtm.
					 Give duple beat choice this much extra
					 over triple choice */

double max_effective_length = 1.0;    /* The maximum effecive length that a note
					 can have, in seconds */

int graphic = 0;                      /* output in graphic form.  If graphic=0 then
					 output the notes */

int duration_augmentation = 0;        /* add this many pips to the duration of notes
					 after rounding off to integral number of pips. */

double triple_bonus = 1.4;            /* For upper levels; multiply triple scores by this
					 much relative to duple scores */

double note_bonus = .2;               /* This value gets added to the note score of a pip
					 once for every note that starts there */
int highest_level_to_compute = HIGHEST_LEVEL;
                                      /* The highest level that will be computed by the program */
int lowest_level_to_compute = 0;
                                      /* The lowest level that will be computed by the program */
