
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
#include <ctype.h>

#include "meter.h"

/*

Feeding the output of the metrical algorithm into the harmonic algorithm.

1. Run the metrical algorithm as it is currently.

2. The "beat pips" are those pips that have a beat at some level.
   Allocate every note to a beat pip.  If the note is already in a beat
   pip, then that's fine.  If the note is not in a beat pip, then move
   it into the previous beat pip.  (If there is no pip before the note,
   then move it to the next beat pip.)  Also adjust the durations of the
   notes in the same fashion.  These are called the adjusted notes.

3. Before the note list we list all the beats in the following form:

      Beat     0  3
      Beat   100  0
      Beat   200  0
      Beat   300  1
      Beat   400  0
      Beat   500  0
      Beat   600  2

   Followed by notes that look like:

      Note   6303   6575 59
      Note   6557   6850 67
      Note   6840   7057 62
      Note   6845   6932 79
      Note   6937   7014 77
      Note   6996   7109 79
      Note   7065   7323 67
      Note   7090   7345 77
      Note   7160   7249 76

   Every note has to start and end on a time that is labeled with
   a beat.

4. The harmonic analyzer will read in the beats.  For each beat it will
   determine the "time period" of that beat, which is the average of the
   time until the next beat of that level or higher (and the previous
   such beat).  If there's just one beat of a given level, it will go to
   the beginning or end of the piece (whichever is larger).

5. After building the chord list, the harmonic program splits the chords
   up.  In the old harmonic analyzer, it just split them according to
   the beat as defined in the beat level data specification.  In the new
   version, it splits them up according to the list of beats specified
   at the beginning.  As it splits them up it associates the appropriate
   time period (computed in 4) with each chord.

After this the harmonic algorithm is just the same as the old one.

*/

/* Destructively append list b to the reverse of a.  Return a pointer to the result */
Pip_note_list * reverse_append (Pip_note_list *a, Pip_note_list * b) {
    Pip_note_list *slx;
    for (; a != NULL; a = slx) {
	slx = a->next;
	
	a->next = b;
	b = a;
    }
    return b;
}    

/* This moves the notes from pip p2 to pip p1 (if any) */
void move_notes(int p1, int p2) {
    if (p1 == p2) return;
    pip_array[p1].pnl = reverse_append(pip_array[p1].pnl, pip_array[p2].pnl);
    pip_array[p2].pnl = NULL;
}

/* this returns the maximum level of the beat at the given pip.
   This value is -1 if there is no level at all */

int max_level(int p) {
    int i;
    for (i=N_LEVELS-1; i>=0; i--) {
	if (pip_array[p].is_beat[i]) break;
    }
    return i;
}

int prev_beat(int p) {
    for (;p >= 0; p--) {
	if (max_level(p) >= 0) break;
    }
    return p;
}

int next_beat(int p) {
    for (;p < N_pips; p++) {
	if (max_level(p) >= 0) break;
    }
    return p;
}

int use_prev_beat(int pip) {
    /* try to go to the previous beat, but if there is none, use the next one */
    int beat;
    beat = prev_beat(pip);
    if (beat < 0) beat = next_beat(pip);
    if (beat >= N_pips) {
	fprintf(stderr, "%s: Error: can't adjust notes.\n", this_program);
	exit(1);
    }
    return beat;
}

int use_next_beat(int pip) {
    int beat;
    beat = next_beat(pip);
    if (beat >= N_pips) beat = prev_beat(pip);
    if (beat < 0) {
	fprintf(stderr, "%s: Error: can't adjust notes.\n", this_program);
	exit(1);
    }
    return beat;
}

	
/* In pathological cases, the adjust_notes process can cause two notes
   that are not overlapping to end up overlapping.  For example, if you
   have two disjoint notes that start and end within a beat.
   
   We handle this now by deleting any note that is contained within
   another note.  */

Pip_note_list * clean_up_note_list(Pip_note_list *pnlstart) {
    Pip_note_list *xpnl, *pnl, *newpnl;
    /* all these notes start at the same time */

    for (pnl = pnlstart; pnl!=NULL; pnl=pnl->next) pnl->mark = 0;

    for (pnl = pnlstart; pnl!=NULL; pnl=pnl->next) {
	for (xpnl = pnlstart; xpnl!=NULL; xpnl=xpnl->next) {
	    /* see if xpnl dominates pnl */
	    if (xpnl->note->pitch == pnl->note->pitch &&
		!xpnl->mark &&
		xpnl != pnl &&
		xpnl->note->duration >= pnl->note->duration) {
		pnl->mark = 1;
	    }
	}
    }

    newpnl = NULL;
    for (pnl = pnlstart; pnl!=NULL; pnl=xpnl) {
	xpnl = pnl->next;

	if (pnl->mark) {
	    xfree(pnl);
	} else {
	    pnl->next = newpnl;
	    newpnl = pnl;
	}
    }
    return newpnl;
}	    

#if 0
OBS /* the following function moves the notes around as described in step 2 above. */
OBS void adjust_notes() {
OBS     int pip, xp;
OBS     Pip_note_list *pnl;
OBS 
OBS     for (pip=0; pip < N_pips; pip++) move_notes(use_prev_beat(pip), pip);
OBS 
OBS     /* Ok, we've moved the notes around.
OBS        Now we adjust the times and durations of the
OBS        notes. */
OBS 
OBS     for (pip=0; pip < N_pips; pip++) {
OBS 	for (pnl = pip_array[pip].pnl; pnl!=NULL; pnl=pnl->next) {
OBS 	    pnl->note->start = pip*pip_time;
OBS 	    xp = quantize(pnl->note->start + pnl->note->duration);
OBS 	    xp++;
OBS 
OBS 	    /* There's a serious problem with all of this.
OBS 	       We're trying to round up the durations so that
OBS 	       the notes end on a beat also.  But what if a
OBS 	       note begins on the last beat of the piece?
OBS 	       Then there's no place to put the end of it.
OBS 	       Simplest solution is simply to delete the note.
OBS 	       This happens in print_standard_note_list below.
OBS 
OBS 	       Modified again, so that it tries to use the previous
OBS 	       beat, but if the duration is 0, then use the next beat;
OBS 	     */
OBS 
OBS 	    if (xp < 0) xp = 0;
OBS 	    if (xp >= N_pips) xp = N_pips-1;
OBS 	    if (use_prev_beat(xp) == pip) {
OBS 		pnl->note->duration = (use_next_beat(xp)-pip) * pip_time;
OBS 	    } else {
OBS 		pnl->note->duration = (use_prev_beat(xp)-pip) * pip_time;
OBS 	    }
OBS 	}
OBS 	pip_array[pip].pnl = clean_up_note_list(pip_array[pip].pnl);
OBS     }
OBS }
#endif

/* 4/24/99 - What the following function does is, it quantizes both the start
time and end time of each note to the nearest pip. - Davy

The function "clean_up_note_list" above removes overlapping notes. If note 1 and note 2
have the same pitch, and note 1 begins first, and ends after note 2 begins, then the
end of note 1 is shifted back to the beginning of note 2. Whichever of the earlier 
offsets is sooner (note 1 or note 2) is then used as the offset for note 2. 

What's a bit confusing about all this is that the graphic output allows overlapping
notes. It's quite happy to have two notes of the same pitch beginning at the same point.
However, the note list (which is used as input to the harmonic program) doesn't allow
overlapping notes; it has the overlaps removed, as just described. - Davy */

/* the following function moves the notes around as described in step 2 above. */
/* now it's different.  We only move them so that each note starts and ends on
   a pip.  We don't require that they start and end on a beat. */

void adjust_notes() {
    int pip;
    Pip_note_list *pnl;

    for (pip=0; pip < N_pips; pip++) {
	for (pnl = pip_array[pip].pnl; pnl!=NULL; pnl=pnl->next) {
	    /* Ok this version rounds the starting time and the ending time to
	       the nearest pip.  It's not quite like the one above.  Davy claims
	       the old one (above) is better.  */
	    /*
	    int duration_pips;
	    duration_pips = quantize(pnl->note->start + pnl->note->duration) - pip;
	    pnl->note->start = pip*pip_time;
	    pnl->note->duration = pip_time * duration_pips;
	    */

	    /* This one tries to duplicate what happens above, which I
               don't think makes all that much sense.  It rounds the duration
	       to the nearest pip, then adds 1.  I don't know why. */
	    /*
	    int xp;
 	    pnl->note->start = pip*pip_time;
 	    xp = quantize(pnl->note->start + pnl->note->duration);
	    xp++;
 	    if (xp < 0) xp = 0;
 	    if (xp >= N_pips) xp = N_pips-1;
	    pnl->note->duration = pip_time * (xp-pip);
	    */

	    /* I think I know why it was done that way.  Because when it was
	       adjusting the notes, it was moving the times back to the previous
	       beat, instead of finding the nearest beat.  It seems most logical
	       to round the duration to the nearest pip, and then round to the
	       nearest beat.  I've done that in the harmony program.

	       Since this arbitrary duration augmentation seemed to help,
	       it's now a parameter.
	       */

	    int xp;
	    xp = quantize(pnl->note->start + pnl->note->duration) + duration_augmentation;
 	    pnl->note->start = pip*pip_time;
	    /* Swap the above two statements to cause the duration to be rounded to
	       the number of number of pips that most closely approximate its length.   */
 	    if (xp < 0) xp = 0;
 	    if (xp >= N_pips) xp = N_pips-1;
	    pnl->note->duration = pip_time * (xp-pip);

	}
	pip_array[pip].pnl = clean_up_note_list(pip_array[pip].pnl);
    }
}


/* This function prints out the beat list, described above */
void print_metronome(void) {
    int p, i;
    for (p=0; p<N_pips; p++) {
	i = max_level(p);
	if (i >= 0) {
	    printf("Beat %6d %2d\n", p * pip_time, i);
	}
    }
}

void print_standard_note_list(void) {
    int p;
    Pip_note_list *pnl;
    for (p=0; p<N_pips; p++) {
	for (pnl = pip_array[p].pnl; pnl != NULL; pnl = pnl->next) {
	    if (pnl->note->duration == 0) continue;
	    printf("Note %6d %6d %3d\n", pnl->note->start, pnl->note->duration + pnl->note->start, pnl->note->pitch);
	}
    }
}
