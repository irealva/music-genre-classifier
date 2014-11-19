
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

/* This file contains the functions that are used in the lowering algorithm */


/* The new lowering algorithm.

   This differs from the one described in meter.c.  Here we assume that the piece
   is broken down into "measures".  A "measure" is defined as the time interval
   between beats of the base level (the level that we're going down one from).

   For each measure we decide if we are going to break it into 2 or 3 parts.
   (Installing 1 or 2 intermediate beats.)  We compute the score for a measure
   by considering both ways.  We store these in a table.

   There's a score for an arrangement of beats within a measure that's based on 
   how well the beats land on the notes, and also on how regular the beats are. 
   There's an additional penalty incurred when you switch from one meter to another
   in neighboring measures.

   Actually, in order to simplify this, and make it work correctly, we
   need to handle the first and last measures specially.  We add an
   extra tactus beat at the beginning and at the end, if necessary.  We simulate
   having a longer pip array to simplify the code. This is what Xpip does.
*/

Pip dummy;  /* This is the pip before and after the real pip array.
	       It's initialized to have no notes there and a zero base score. */

#define Xpip(p) ((((p)<0)||((p)>=N_pips)) ? &dummy : &pip_array[(p)])

typedef struct {
    double score[3];   /* score[i] is for there being i beats
			  internal to this measure.  [0] element not used */
    double local_score[3];  /* the score due to making one of these
			       two choices for this measure in isolation */

    int one_pip;      /* if there's one beat, it's on this pip */
    int two_pips1, two_pips2;  /* if there's 2 beats they're on these two pips */
                        /* all of these are set by measure_score () */

    int best_beat_count;  /* either 1 or 2 depending on which is chosen for
			     this measure */
    
    int left_pip;      /* The pip number of the beat on the left end of this measure */
    int right_pip;     /* Ditto for the right side */
} Measure;

Measure * measure_array;
int N_measures;

void build_measure_array(int base_level) {
    /* this takes the pip_array at the given beat_level, and constructs
       the measure array --- allocates it and sets the endpoints.
       It also sets N_measures to the number of measures in the piece.
       This is always one more than the number of beats at the base level.
       */
    int pip, m;
    int first_beat_pip=0, last_beat_pip=0, N_beats;
    double d_average_pips_per_beat;
    int average_pips_per_beat;
    int first_pseudo_pip, last_pseudo_pip;  /* should these be globlal?  */

    dummy.is_beat[base_level] = FALSE;  /* we'll only be looking at this level now,
					   so you can ignore the others */
    dummy.base = 0.0;
    dummy.pnl = NULL;

    N_beats = 0;
    for (pip=0; pip < N_pips; pip++) {
	if (pip_array[pip].is_beat[base_level]) {
	    if (N_beats == 0) first_beat_pip = pip;
	    last_beat_pip = pip;
	    N_beats++;
	}
    }

    /* now first_beat_pip, last_beat_pip, and N_beats have been computed */

    if (N_beats <= 1) {
	fprintf(stderr, "%s: Too few beats at level %d to compute a lower level.\n", this_program, base_level);
	exit(1);
    }

    d_average_pips_per_beat = (((double)last_beat_pip) - first_beat_pip)/((double) N_beats-1);
    average_pips_per_beat = (d_average_pips_per_beat + .5);  /* round this off */

    first_pseudo_pip = first_beat_pip;
    if (first_beat_pip > 0) {
	first_pseudo_pip -= average_pips_per_beat;
	if (first_pseudo_pip >= 0) first_pseudo_pip = -1;
    }

    last_pseudo_pip = last_beat_pip;
    if (last_beat_pip < N_pips-1) {
	last_pseudo_pip += average_pips_per_beat;
	if (last_pseudo_pip <= N_pips-1) last_pseudo_pip = N_pips;
    }

    /* when processing all pips we go from [first_pseudo_pip....last_pseudo_pip] inclusive */
    /* we know there is a "beat" at first_pseudo_pip and last_pseudo_pip */

    /* Actually, there's a bug here.  What if when we extrapolate back from the first
       beat, it turns out that we're still in the array.  Then we want to go back to
       to one before the beginning.  Ditto for the end.

       Ok this bug has been fixed in the above code.
     */
    
    
    N_measures = 0;
    for (pip=first_pseudo_pip+1; pip < last_pseudo_pip; pip++) {
	if (Xpip(pip)->is_beat[base_level]) N_measures++;
    }
    N_measures++;  /* one more measure than there are beats */

    measure_array = (Measure *) xalloc(N_measures * sizeof(Measure));

    m = 0;
    measure_array[m].left_pip = first_pseudo_pip;
    for (pip=first_pseudo_pip+1; pip < last_pseudo_pip; pip++) {
	if (Xpip(pip)->is_beat[base_level]) {
	    /* build measure m and m+1, the measures neighboring this pip */
	    measure_array[m].right_pip = measure_array[m+1].left_pip = pip;
	    m++;
	}
    }
    measure_array[m].right_pip = last_pseudo_pip;
    for (m=0; m < N_measures; m++) {
	measure_array[m].best_beat_count = -1; /* a bogus value saying this is uninitialized */
    }
}

void free_measure_array(void) {
    xfree(measure_array);
}

void measure_score(int m, int n_beats) {
    /* for the given measure in the measure array m, compute the score
       of this measure in isolation assuming that it has the given number
       of beats.  Keeps the score in local_score in the array. */

    int p1, p2, bp1=0, bp2=0, left, right, first;
    double score, bscore=0.0;
    left = measure_array[m].left_pip;
    right = measure_array[m].right_pip;
    
    if (n_beats == 1) {
	/* looking for one beat, we just try all the possibilities */
	first = TRUE;
	for (p1 = left+1; p1 < right; p1++) {
	    score = (Xpip(p1)->base) + duple_bonus +
		-deviation_penalty((p1-left)*pip_time, (right-p1)*pip_time);
	    if (first || (score > bscore)) {
		first = FALSE;
		bp1 = p1;
		bscore = score;
	    }
	}
	measure_array[m].one_pip = bp1;
	if (first) {
	    fprintf(stderr, "%s: Warning: Not enough pips in the measure\n", this_program);
	    measure_array[m].one_pip = left;  /* will cause no beat to be generated */
	}
    } else {
	first = TRUE;
	for (p1 = left+1; p1 < right; p1++) {
	    for (p2 = p1+1; p2 < right; p2++) {
		score = Xpip(p1)->base + Xpip(p2)->base
		    -deviation_penalty((p1-left)*pip_time, (p2-p1)*pip_time)
		    -deviation_penalty((p2-p1)*pip_time, (right-p2)*pip_time);
		if (first || (score > bscore)) {
		    first = FALSE;
		    bp1 = p1;
		    bp2 = p2;
		    bscore = score;
		}
	    }
	}
	measure_array[m].two_pips1 = bp1;
	measure_array[m].two_pips2 = bp2;
	if (first) {
	    fprintf(stderr, "%s: Warning: not enough pips in the measure\n", this_program);
	    measure_array[m].two_pips1 = left;  /* will cause no beat to be generated */
	    measure_array[m].two_pips2 = left;
	}
    }
    
    measure_array[m].local_score[n_beats] = bscore;
    /*    printf(" measure_array[%d].local_score[%d] = %6.5f\n", m, n_beats, bscore); */
}

double best_beat_score(int m, int xb, int *y_beats) {
    /* return the score for this choice of xb, the number of beats in this measure */
    /* m is >= 1; */
    int besty = -1;
    int yb;
    double score, best_score = 0.0;

    for (yb = 1; yb <= 2; yb++) {  /* number of beats in measure m-1 */
	score = 0.0;
	if (xb != yb) score += -meter_change_penalty;

	score += measure_array[m].local_score[xb] + measure_array[m-1].score[yb];

	if (besty == -1 || score > best_score) {
	    besty = yb;
	    best_score = score;
	}
    }
    *y_beats = besty;
    return best_score;
}

void compute_measure_scores(void) {
    int m;
    int n_beats, xb, yb;

    /* first compute the local scores */
    for (m=0; m<N_measures; m++) {
	for (n_beats = 1; n_beats <= 2; n_beats++) {
	    measure_score(m, n_beats);
	}
    }

    /* for the first measure, the score is the local score */
    for (n_beats = 1; n_beats <= 2; n_beats++) {
	measure_array[0].score[n_beats] = measure_array[0].local_score[n_beats];
    }    

    for (m=1; m<N_measures; m++) {
	for (xb = 1; xb <=2; xb++) {
	    measure_array[m].score[xb] = best_beat_score(m, xb, &yb);
	}
    }
}

void insert_beats(int new_level) {
    int m, n_beats, bb, pip, newb, base_level;
    double best_score=0.0, score;

    base_level = new_level+1;
    for (pip = 0; pip < N_pips; pip++) {
	pip_array[pip].is_beat[new_level] = pip_array[pip].is_beat[base_level];
    }

    /* Find which choice in the last measure is the best choice */

    m = N_measures-1;
    bb = -1;
    for (n_beats = 1; n_beats <=2; n_beats++) {
	score = measure_array[m].score[n_beats];
	if (bb == -1 || score > best_score) {
	    bb = n_beats;
	    best_score = score;
	}
    }
    /* bb is now the best number of beats to chose in the last measure */
    
    for (m=N_measures-1; m>=0; m--) {
	/* for the current measure m we've already got the choice
	   of number of beats, namely bb.  Stuff those beats into
	   the is_beat array right now. */

	measure_array[m].best_beat_count = bb;
	
	if (bb == 1) {
	    Xpip(measure_array[m].one_pip)->is_beat[new_level] = TRUE;
	} else {
	    Xpip(measure_array[m].two_pips1)->is_beat[new_level] = TRUE;	    
	    Xpip(measure_array[m].two_pips2)->is_beat[new_level] = TRUE;	    
	}
	if (m == 0) break;
	best_beat_score(m, bb, &newb);
	bb = newb;
    }
}

void print_measures(void) {
    int m;
    Measure *mp;

    for (m=0; m<N_measures; m++) {
	mp = &measure_array[m];
	if (mp->best_beat_count == -1) {
	    printf("Measure %3d: [%6d %6d] local_score = (%5.2f %5.3f)\n",
		   m, (mp->left_pip)*pip_time, (mp->right_pip)*pip_time, mp->local_score[1], mp->local_score[2]);
	} else {
	    printf("Measure %3d: [%6d %6d] %2d beats",
		   m, (mp->left_pip)*pip_time, (mp->right_pip)*pip_time, mp->best_beat_count+1);
	    if (mp->best_beat_count == 1) {
		printf("(%6d)", mp->one_pip * pip_time);
	    } else if (mp->best_beat_count == 2) {
		printf("(%6d %6d)", mp->two_pips1 * pip_time, mp->two_pips2 * pip_time);
	    }
	    printf(" ls = %6.2f  score = %6.2f\n",
		   mp->local_score[mp->best_beat_count], mp->score[mp->best_beat_count]);
	}
    }
}


void compute_lower_level(int base_level) {
    int new_level;
    new_level = base_level-1;

    if (verbosity > 1) printf("Computing level %2d\n", new_level);

    build_measure_array(base_level);

    compute_measure_scores();
    if (verbosity > 1) print_measures();

    insert_beats(new_level);

    if (verbosity > 1) {
	printf("Beat group scores\n");
	print_measures();
    }

    free_measure_array();
}
