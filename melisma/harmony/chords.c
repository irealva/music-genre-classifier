
 /***************************************************************************/
 /*                                                                         */
 /*       Copyright (C) 2000 Daniel Sleator and David Temperley             */
 /*           See http://www.link.cs.cmu.edu/music-analysis                 */
 /*        for information about commercial use of this system              */
 /*                                                                         */
 /***************************************************************************/


#include <stdio.h>
#include <stdlib.h>

#include "harmony.h"

/* This file contains code that can builds the chord list from a note
   list and the beatlevel structure.  See the comment near the
   definition of the Chord structure for more info on what a chord is.
   For a description of the properties of the note list see
   documentation above build_note_list_from_event_list().  */


Chord * new_chord(Note *note) {
    Chord * c_chord;
    /* build a new chord with the parameters taken from the given note */
    c_chord = (Chord *) xalloc(sizeof(Chord));
    c_chord->start = note->start;
    c_chord->duration = note->duration;
    c_chord->note = NULL;
    c_chord->next = NULL;
    c_chord->level = c_chord->level_time = 0;  /* not filled in */
    return c_chord;
}

Chord * reverse_ch(Chord * a) {
    Chord *slx;
    Chord *b = NULL;
    
    for (; a != NULL; a = slx) {
	slx = a->next;
	
	a->next = b;
	b = a;
    }
    return b;
}

/* this function builds a chord representation of a piece from the note
   list.  The function does not effect the note list.  The level and
   level_time fields of the chord list are not filled in by this
   function.  */

Chord * build_chord_representation (Note * nl) {
    Note * xnl, *n_copy;
    Chord * c_chord, * head_chord;
    int end_time;
    head_chord = c_chord = NULL;
    
    for (xnl = nl; xnl != NULL; xnl = xnl->next) {
	if (c_chord == NULL) head_chord = c_chord = new_chord(xnl);
	n_copy = (Note *) xalloc(sizeof(Note));
	*n_copy = *xnl;
	if (c_chord->start == xnl->start && c_chord->duration == xnl->duration) {
	    /* add to current chord */
	    n_copy->next = c_chord->note;
	    c_chord->note = n_copy;
	    continue;
	}
	
	end_time = c_chord->start + c_chord->duration;
	
	if (end_time > xnl->start) {
	    fprintf(stderr, "%s: A note begins during another note.\n", this_program);
	    my_exit(1);
	}
	
	if (end_time < xnl->start) {
	    /* add a blank chord */
	    c_chord->next = new_chord(xnl);  /* will throw out these parameters */
	    c_chord = c_chord->next;
	    c_chord->start = end_time;  /* this starts when the previous ends */
	    c_chord->duration = xnl->start - end_time;
	}
	
	/* start a new chord right after the current one */
	c_chord->next = new_chord(xnl);
	c_chord = c_chord->next;
	n_copy->next = NULL;
	c_chord->note = n_copy;
    }
    return head_chord;
}

void print_chord_list(Chord *clist) {
    Note * note;
    int i;
    printf("Chord List:\n");
    
    for (;clist != NULL; clist = clist->next) {
	printf(" ");
	for (i=0; i<N_beatlevel; i++) if (clist->level >= i) printf("x "); else printf("  ");
	
	printf("start = %5d  duration = %5d  lt = %5d  od = %1d  notes: {",
	       clist->start, clist->duration, clist->level_time, clist->is_first_chord);
	for (note = clist->note; note != NULL; note = note->next) {
	    printf("(%2d,%2d)", note->pitch, note->tpc);
	}
	printf("}\n");
    }
}

/* This takes a cord representation as computed by
   build_chord_representation() and returns a new chord representation
   in which each original chord is broken into some integral number of
   shorter chords.  The durations of these short chords should be
   roughly equal to the baseunit.  Furthermore, these chords are
   labelled with the rythmic level and the rythmic time of the note.
   
   The note lists of the new chord list point to the same note lists as
   the original chord list.  Several chords in the new list may point to
   the same note lists.  */
#if 0
OBS Chord * build_metered_chord_representation(Chord *chord) {
    OBS   Chord *ch, *nch, *ret_ch;
    OBS   int parts, i, level, count;
    OBS   double actual_time;
    OBS   double ratio;
    OBS 
	OBS   if (baseunit < 0) {
	    OBS     fprintf(stderr, "BaseUnit has not been set.\n");
	    OBS     exit(1);
	    OBS   }
    OBS 
	OBS   ret_ch = NULL;
    OBS   
	OBS   for (ch = chord; ch!=NULL; ch = ch->next) {
	    OBS     parts = (2*ch->duration + baseunit) / (2 * baseunit);  /* rounded version of duration/baseunit */
	    OBS     actual_time = ((double) ch->duration)/ ((double) parts);
	    OBS     ratio = actual_time/((double)baseunit);
	    OBS     if (ratio < .9 || ratio > 1.1) {
		OBS       fprintf(stderr, "A chord of duration %d is incomensurate with the baseunit (%d).\n", ch->duration, baseunit);
		OBS       exit(1);
		OBS     }
	    OBS     /* now we break the given chord into parts parts */
		OBS     for (i=0; i<parts; i++) {
		    OBS       nch = (Chord *) xalloc (sizeof(Chord));
		    OBS       nch->start = (i * ch->duration)/parts + ch->start;
		    OBS       nch->duration = (((i+1) * ch->duration)/parts) - ((i * ch->duration)/parts);
		    OBS       nch->note = ch->note;
		    OBS       nch->next = ret_ch;
		    OBS       nch->level = nch->level_time = 0; /* will be filled in later */
		    OBS       nch->is_first_chord = (i == 0);
		    OBS       ret_ch = nch;
		    OBS     }
	    OBS   }
    OBS   ret_ch = reverse_ch(ret_ch);
    OBS 
	OBS   /* now we add the rythmic stuff */
	OBS 
	OBS   for (level = 1; level < N_beatlevel; level++) {
	    OBS     count = beatlevel[level].count - beatlevel[level].start;
	    OBS     for (ch = ret_ch; ch!=NULL; ch = ch->next) {
		OBS       if  (ch->level == level-1) {
		    OBS 	if ((count % beatlevel[level].count) == 0) {
			OBS 	  ch->level = level;
			OBS 	}
		    OBS 	count++;
		    OBS       }
		OBS     }
	    OBS   }
    OBS 
	OBS   for (ch = ret_ch; ch!=NULL; ch = ch->next, count++) {
	    OBS     ch->level_time = beatlevel[ch->level].units * baseunit;
	    OBS   }
    OBS   
	OBS   return ret_ch;
    OBS }
#endif

/* We need to be able to take a time t, and lookup which beat
   that time corresponds to.  The beats are sorted, so we can use binary
   search. */

int rlookup_beat(int t, int l, int r) {
    /* search the range [l...r] inclusive. */
    int m;
    if (r < l) return -1;
    m = (r+l)/2;
    if (t < beat_array[m]->start) return rlookup_beat(t, l, m-1);
    if (t > beat_array[m]->start) return rlookup_beat(t, m+1, r);
    return m;
}

int lookup_beat(int t) {
    int x;
    x = rlookup_beat(t, 0, N_beats-1);
    if (x == -1) {
	fprintf(stderr, "%s: Could find no beat at time %d.\n", this_program, t);
	my_exit(1);
    }
    return x;
}

int level_time(int beat) {
    int level, i, xt, xc;
    /* For whatever level this beat is at, return the average time of
       beats at this level in the vicinity of this beat */
    level = beat_array[beat]->level;
    xc = 0;
    xt = 0;
    for (i=beat+1; i<N_beats; i++) {
	if (beat_array[i]->level >= level) {
	    xt += beat_array[i]->start - beat_array[beat]->start;
	    xc++;
	    break;
	}
    }
    for (i=beat-1; i>=0; i--) {
	if (beat_array[i]->level >= level) {
	    xt += beat_array[beat]->start - beat_array[i]->start;
	    xc++;
	    break;
	}
    }
    if (xc == 0) {
        /* fprintf(stderr, "%s: No neighboring beats at level %d.\n", this_program, level); */
        /* There's only one beat at this level; we just guess the beat interval, depending on the level */
	if(level==4) return 2400;
	if(level==3) return 1200;
	else return 600;
    }
    return (xt/xc);
}

/* This version does the same thing, except that it uses the 
   beat structure to label levels of the chords */

/* Our input is the chords constructed from the notes we've read in.
   We're supposed to chop these up according to the beat system that
   we've read in.  These chords may need to be broken up so that
   each resulting chord spans exactly one beat. */

Chord * build_metered_chord_representation(Chord *chord) {
    Chord *ch, *nch, *ret_ch;
    int start_beat, end_beat, beat;
    
    if (N_beats == 0) {
	fprintf(stderr, "%s: There are no beats in the input.\n", this_program);
	my_exit(1);
    }
    
    ret_ch = NULL;
    
    /*
      Ok...this will work as follows....we look up the start time of the
      chord in the beat list. we look up the end time of the chord in the
      beat list (these should both be there).  We break it up into a
      piece for the first of these, and for each beat strictly between
      these (not the last one).  These are the chords generated for a
      single chord.  Of course they inherit the proper beat level, and we
      can label them with their durations.
      */
    for (ch = chord; ch!=NULL; ch = ch->next) {
	start_beat = lookup_beat(ch->start);
	end_beat = -1 + lookup_beat(ch->start+ch->duration);
	for (beat=start_beat; beat <= end_beat; beat++) {
	    nch = (Chord *) xalloc (sizeof(Chord));
	    nch->start = beat_array[beat]->start;
	    nch->duration = beat_array[beat+1]->start - nch->start;
	    nch->note = ch->note;
	    nch->level = beat_array[beat]->level;
	    nch->level_time = level_time(beat);
	    nch->is_first_chord = (beat == start_beat);
	    nch->next = ret_ch;
	    ret_ch = nch;
	}
    }
    ret_ch = reverse_ch(ret_ch);
    return ret_ch;
}

/* This takes a metered chord representation, computed by the function
   above, and constructs a new one.  It works by coalescing successive
   chords that have the same notes into a single chord.  It only does
   this if the beat level of the new one is lower (less important) than
   the beat level of the chord you started this group with.
   */

#if 1

Chord * compact_metered_chord_representation(Chord *chord) {
    Chord *ch, *nch, *ret_ch, *current_chord;
    ret_ch = NULL;
    if (chord == NULL) return NULL;
    
    current_chord = (Chord *) xalloc(sizeof(Chord));
    *current_chord = *chord;
    current_chord->next = NULL;
    for (ch = chord->next; ch!=NULL; ch = ch->next) {
	/* current_chord is the chord we're building onto at the moment */
	if (ch->note == current_chord->note &&    /* they have the same notes */
	    ch->level < current_chord->level) {   /* the new chord is at a less significant beat level */
	    /* glue them together */
	    current_chord->duration += ch->duration;
	} else {
	    /* don't glue them together */
	    /* emit the current chord, and start a new one */
	    
	    nch = (Chord *) xalloc (sizeof(Chord));
	    *nch = *ch;
	    nch->next = current_chord;
	    current_chord = nch;
	}
    }
    return reverse_ch(current_chord);
}

#else

Chord * compact_metered_chord_representation(Chord *chord) {
    Chord *ch, *nch, *ret_ch, *current_chord;
    ret_ch = NULL;
    
    current_chord = NULL;
    for (ch = chord; ch!=NULL; ch = ch->next) {
	nch = (Chord *) xalloc (sizeof(Chord));
	*nch = *ch;
	nch->next = current_chord;
	current_chord = nch;
    }
    return reverse_ch(current_chord);
}

#endif

void free_chords(Chord * ch) {
    Chord *xch;
    for (; ch != NULL; ch = xch) {
	xch = ch->next;
	xfree(ch);
    }
}
