
 /***************************************************************************/
 /*                                                                         */
 /*       Copyright (C) 2000 Daniel Sleator and David Temperley             */
 /*           See http://www.link.cs.cmu.edu/music-analysis                 */
 /*        for information about commercial use of this system              */
 /*                                                                         */
 /***************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "meter.h"

/* This file contains the functions that are used in the raising algorithm */


/* The new raising algorithm.

   For each beat at the base level (the level we're building on top
   of), we consider two scores: one for using the beat 2 ago, one for
   using the beat 3 ago.  This is facilitied by using an extra array,
   each element of which has two scores and a pointer to the pip number
   that this element originated from.

   The score of a choice only depends on if you are changing the number
   of base level beats between beats, and on the value of putting a beat
   at this particular place.  So this algorithm does not use deviation
   penalties.

 */

typedef struct {
    double score[2];   /* score[0] is the 2 beat case, and score [1] is the 3 beat case */
    int best_back;     /* which is the best choice here? going back 2 or 3?  */
    int pip;           /* the pip that this place corresponds to */
} BL;   /* stands for base level */

BL * bl_array;
int N_bl;

void build_bl_array(int base_level) {
    int pip, i;

    N_bl = 0;
    for (pip=0; pip < N_pips; pip++) {
	if (pip_array[pip].is_beat[base_level]) {
	    N_bl++;
	}
    }

    bl_array = (BL *) xalloc(N_bl * sizeof(BL));
    
    i=0;
    for (pip=0; pip < N_pips; pip++) {
	if (pip_array[pip].is_beat[base_level]) {
	    bl_array[i].score[0] = bl_array[i].score[1] = 0.0;
	    bl_array[i].pip = pip;
	    i++;
	}
    }
}

void free_bl_array(void) {
    xfree(bl_array);
}

double best_raising_choice(int base_beat, int back, int *choice, int use_higher_base) {
    /* for this base_beat (index into bl_array) and this back
       (which has value 2 or 3) compute the choice (2 or 3) to make
       at the place back ago that has the best value.  Return this
       value.  If base_beat-back < 0 then *choice is set to -1 */

    int try;
    double score, best_score, base;
    
    if (use_higher_base) {
	base = pip_array[bl_array[base_beat].pip].higher_base;
    } else {
	base = pip_array[bl_array[base_beat].pip].base;
    }

    if (base_beat - back < 0) {
	if (pip_array[bl_array[base_beat].pip].is_first_beat[3]==1) base += 0.5;
	/* The 0.5 here is a bonus if the first beat at level 4 coincides with the first beat at level 3 */
	*choice = -1;
	return base;  
    }

    best_score = 0.0;
    for (try=2; try <=3; try++) {
	score = (1.0 + (try-2)*(triple_bonus-1.0))*base + bl_array[base_beat-back].score[try-2];

	/* The try/2.0 term gives an appropriate bonus for using triple meter,
	   compensating for the fact that there will be fewer beats. For example, 
	   a factor of .4 means that triple-meter note scores are multiplied by 1.4 */

	if (try != back) score -= raising_change_penalty;
	if (try == 2 || best_score < score) {
	    best_score = score;
	    *choice = try;
	}
    }
    return best_score;
}

void compute_higher_level_scores(int base_level, int current_level) {
    /* we've already computed beats at the base level.  We're now computing
       beats at the current_level.  current_level is always going to be
       base_level+1.  No biggie. */
    int beat, back, choice;

    for (beat=0; beat < N_bl; beat++) {
	for (back = 2; back <= 3; back++) {
	    bl_array[beat].score[back-2] = best_raising_choice(beat, back, &choice, current_level == HIGHEST_LEVEL);
	}
    }
}

void label_raise_beats(int beat, int back, int level) {
    /* label the beats in pip_array[level].is_beat,
       also, fill in the best_back fields of the bl_array.
       (Having chosen the best last beat, this traces it back.) */
    int k;
    
    while (1) {
	pip_array[bl_array[beat].pip].is_beat[level] = 1;
	bl_array[beat].best_back = back;
	if (beat-back < 0) {
	    pip_array[bl_array[beat].pip].is_first_beat[level] = 1;
	    break;
	}
	best_raising_choice(beat, back, &k, level == HIGHEST_LEVEL);
	beat -= back;
	back = k;
    }
}

double evaluate_raised_solution(int level, int compute_beats) {
    /* comment from evaluate_solution in meter.c....

       first we find the best solution in the last max_pips pips
       (...that is, the best solution in an interval at the end of the
       piece of length max_pip - deciding on the final beat of the best
       solution) then follow it back if compute_beats=1

       Only call this once with compute_beats==TRUE per level, because
       there's no mechanism for clearing the is_beat.
       */

    int beat, back, best_beat, best_back;
    double score, best;

    best = 0.0;
    best_back = best_beat = -1;
    for (beat = N_bl-1; beat >= N_bl-3; beat--) {
	for (back = 2; back <= 3; back++) {
	    score = bl_array[beat].score[back-2];
	    if (best_back == -1 || best < score) {
		best = score;
		best_back = back;
		best_beat = beat;
	    }
	}
    }
    if (best_back == -1) {
	fprintf(stderr, "%s: Error: no scores to look at.\n", this_program);
	exit(1);
    }
    if (compute_beats) label_raise_beats(best_beat, best_back, level);
    return best;
}

void print_raised_beats(int level) {
    int pip, beat;
    int delta;

    for (beat=0; beat<N_bl; beat++) {
	pip = bl_array[beat].pip;
	if (pip_array[pip].is_beat[level]) {
	    if (beat == 0) {
		delta = 0;
	    } else {
		delta = pip_time * (pip - bl_array[beat-1].pip);
	    }
	    printf("Beat at time %5d  (interval = %5d) (cumulative score = %6.3f)\n",
		   pip*pip_time, delta, bl_array[beat].score[bl_array[beat].best_back-2]);
	}
    }
}

void compute_higher_level(int base_level) {
    int new_level;
    new_level = base_level+1;

    if (verbosity > 1) {
	printf("Computing level %2d\n", new_level);
    }

    if (new_level >= N_LEVELS) {
	fprintf(stderr, "%s: attempting to compute a level that's out of range.\n", this_program);
	exit(1);
    }

    build_bl_array(base_level);

    compute_higher_level_scores(base_level, new_level);

    evaluate_raised_solution(new_level, TRUE);

    if (verbosity > 1) print_raised_beats(new_level);

    free_bl_array();
}
