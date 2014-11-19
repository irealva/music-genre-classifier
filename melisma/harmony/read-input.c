
 /***************************************************************************/
 /*                                                                         */
 /*       Copyright (C) 2000 Daniel Sleator and David Temperley             */
 /*           See http://www.link.cs.cmu.edu/music-analysis                 */
 /*        for information about commercial use of this system              */
 /*                                                                         */
 /***************************************************************************/ 


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "harmony.h"

/* This file contains the code that reads in the input.  It
   builds the beatlevel structures and forms a Note list
   from the input. */

static int line_no;  /* the line number */
static char line[1000];

typedef struct event_struct {
    /* this structure is just used for reading in the notes */
    int note_on;  /* 1 if this is note on, 0 otherwise */
    Pitch pitch;
    int time;     /* the time of this event */
    struct event_struct * next;
    DirectNote * directnote;
} Event;

void free_event_list(Event *e) {
    Event *xe;
    for (; e != NULL; e = xe) {
	xe = e->next;
	
	xfree ((char *) e);
    }
}

void test_pitch(Pitch pitch) {
    if (pitch < 0 || pitch > MAX_PITCH-1) {
	fprintf (stderr, "%s: Error (line %d) Found a pitch that is out of range: %d\n", this_program, line_no, pitch);
	my_exit(1);
    }
}

void warn(void) {
    char * x;
    static int first_warning = 1;
    x = strchr(line, '\n');
    if (x != NULL) *x = '\0';
    if (first_warning) {
	printf("Ignoring the following lines:\n");
	first_warning = 0;
    }
    printf("---> %s\n", line);
}

Event * build_event_list_and_beat_list_from_input(void) {
    char first_part[100];
    int second_part;
    int third_part;
    int fourth_part;
    int event_time;
    int N_parts;
    int on_event, both_event, off_event;
    int on_time, off_time;
    int i;
    Pitch pitch;
    Event * event_list, * new_event;
    Beat * new_beat;
    
    beatlevel[0].count = 1;
    beatlevel[0].start = 0;
    beatlevel[0].units = 1;
    N_beatlevel = 1;
    
    event_time = 0;
    event_list = NULL;
    global_beat_list = NULL;
    
    for (line_no = 1; fgets(line, sizeof(line), instream) != NULL; line_no++) {
	for (i=0; isspace(line[i]); i++);
	if (line[i] == '%' || line[i] == '\0') continue;  /* ignore comment lines */
	N_parts = sscanf(line, "%99s %d %d %d", first_part, &second_part, &third_part, &fourth_part);
	if (N_parts < 2) {
	    warn();
	    continue;
	}
	
	if (strcasecmp(first_part, "BaseUnit") == 0) {
	    if (N_parts != 2) {
		fprintf(stderr, "%s: Error (line %d) BaseUnit expects one integer parameter.\n", this_program, line_no);
		my_exit(1);
	    }
	    baseunit = second_part;
	    continue;
	}
	
	if (strcasecmp(first_part, "BeatLevel") == 0) {
	    if (N_parts != 4) {
		fprintf(stderr, "%s: Error (line %d) BeatLevel expects three integer parameters.\n", this_program, line_no);
		my_exit(1);
	    }
	    
	    if (N_beatlevel != second_part) {
		fprintf(stderr,"%s: Error (line %d) A BeatLevel numbers must be in increasing order starting from 1.\n", this_program, line_no);
		my_exit(1);	
	    }
	    
	    if (N_beatlevel+1 > MAX_BEAT_LEVEL) {
		fprintf(stderr,"%s: Error (line %d) A BeatLevel cannot be greater than %d\n", this_program, line_no, MAX_BEAT_LEVEL);
		my_exit(1);	
	    }
	    beatlevel[N_beatlevel].count = third_part;
	    beatlevel[N_beatlevel].start = fourth_part;
	    beatlevel[N_beatlevel].units = beatlevel[N_beatlevel].count * beatlevel[N_beatlevel-1].units;
	    
	    if (verbosity >= 3) {
		printf(" beatlevel[%d].count(start)(units) =%d (%d)(%d)\n", N_beatlevel, third_part, fourth_part, beatlevel[N_beatlevel].units);
	    }
	    
	    N_beatlevel++;
	    continue;
	}
	
	if (strcasecmp(first_part, "Beat") == 0) {
	    new_beat = (Beat *) xalloc(sizeof(Beat));
	    new_beat->start = second_part;
	    new_beat->level = third_part;
	    if(print_beats) printf("Beat %6d %2d\n", new_beat->start, new_beat->level);
	    new_beat->next = global_beat_list;
	    global_beat_list = new_beat;
	    
	    if (new_beat->level+1 > N_beatlevel) N_beatlevel = new_beat->level+1;
	    
	    continue;
	}
	
	on_event = (strcasecmp(first_part, "Note-on") == 0);
	both_event = (strcasecmp(first_part, "Note") == 0);
	off_event = (strcasecmp(first_part, "Note-off") == 0);
	
	if (!(on_event || off_event || both_event)) {
	    warn();
	    continue;
	}
	
	if ((on_event || off_event || both_event) && (verbosity>2 || prechord_mode==1)) {   
	                                                                     /* This prints out the inputted Note statements */
	    printf("%s", line);
	}
	if (on_event || off_event) {
	    if (N_parts != 3) {
		fprintf(stderr, "%s: Error (line %d) a note needs a time and a pitch\n", this_program, line_no);
		my_exit(1);		
	    }
	    
	    event_time = second_part;
	    pitch = third_part;
	    
	    test_pitch(pitch);
	    new_event = (Event *) xalloc(sizeof(Event));
	    new_event->pitch = pitch;
	    new_event->note_on = on_event;
	    new_event->time = event_time;
	    new_event->next = event_list;
	    new_event->directnote = NULL;
	    event_list = new_event;
	} else {
	    /* it's a both event */
	    if (N_parts != 4) {
		fprintf(stderr, "%s: Error (line %d) a note needs an on-time an off-time and a pitch\n", this_program, line_no);
		my_exit(1);		
	    }
	    on_time = second_part;
	    off_time = third_part;
	    pitch = fourth_part;
	    
	    test_pitch(pitch);      
	    new_event = (Event *) xalloc(sizeof(Event));
	    new_event->pitch = pitch;
	    new_event->note_on = 1;
	    new_event->time = on_time;
	    new_event->next = event_list;
	    new_event->directnote = NULL;
	    event_list = new_event;
	    
	    new_event = (Event *) xalloc(sizeof(Event));
	    new_event->pitch = pitch;
	    new_event->note_on = 0;
	    new_event->time = off_time;
	    new_event->next = event_list;
	    new_event->directnote = NULL;
	    event_list = new_event;
	}
    }
    
    return event_list;
}


Note * reverse(Note * a) {
    Note *slx;
    Note *b = NULL;
    
    for (; a != NULL; a = slx) {
	slx = a->next;
	
	a->next = b;
	b = a;
    }
    return b;
}

NPC Pitch_to_NPC(Pitch p) {
    return (p+12000000) % 12;
    /* the 12000000 guarantees that it works correctly for negative numbers
       (as long as they're at least -12000000).*/
}

TPC base_TPC(NPC n) {
    /* this returns the base tpc for a NPC */
    /* These are numbered 0...11 */
    return ((2 + 7*n + 12000000) % 12);
}

int comp_event1(Event ** a, Event ** b) {
  if ((*a)->time != (*b)->time) return  ((*a)->time - (*b)->time);
  return ((*a)->note_on - (*b)->note_on);
}

int comp_event2(Event ** a, Event ** b) {
    if ((*a)->time != (*b)->time) {
	return  ((*a)->time - (*b)->time);
    } else if ((*a)->note_on != (*b)->note_on) {
	return ((*a)->note_on - (*b)->note_on);
    } else if ((*a)->note_on == 0) {
	return 0;
    } else {
	/* We know they're both note_on events.

	   The one corresponding to the longer event is first.  This is
	   so that if one note contains another (due to rounding) the
	   longer one is kept.
	 */
	return ((*b)->directnote->duration) - ((*a)->directnote->duration);
    }
}

DirectNote * build_direct_note_list_from_event_list(Event * e) {
    Event *el;
    Event ** etable;
    int N_events, event_n, k, i;
    DirectNote *dn, *dnx, *dn_new;
    
    
    for (N_events=0, el = e; el != NULL; el = el->next) N_events++;
    etable = (Event **) xalloc (N_events * sizeof (Event *));
    for (i=0, el = e; el != NULL; el = el->next, i++) etable[i] = el;
    
    qsort(etable, N_events, sizeof (Event *), (int (*)(const void *, const void *))comp_event1);
    
    dn = NULL;
    
    for (event_n=0; event_n<N_events; event_n++) {
	el = etable[event_n];
	if (el->note_on) {
	    /* find the corresponding note off event, and generate the
	       note */
	    for (k = event_n+1; k < N_events; k++) {
		if (el->pitch == etable[k]->pitch && !(etable[k]->note_on)) break;
	    }
	    if (k == N_events) {
		fprintf(stderr, "%s: Error: pitch %d turned on but never got turned off.\n", this_program, el->pitch);
		my_exit(1);
	    }
	    /* now we've found the on and off events for this note.  generate the note */
	    dnx = (DirectNote *) xalloc(sizeof(DirectNote));
	    dnx->start = el->time;
	    dnx->duration = etable[k]->time - el->time;
	    dnx->tpc = UNINITIALIZED_TPC;
	    dnx->pitch = el->pitch;
	    dnx->next = dn;
	    dn = dnx;
	    el->directnote = dn;
	}
    }
    /* now reverse the list and return it */
    
    dn_new = NULL;
    for (;dn != NULL; dn = dnx) {
	dnx = dn->next;
	
	dn->next = dn_new;
	dn_new = dn;
    }
    return dn_new;
}

/* This function builds a note list from an event list.  This note list
   is sorted by increasing starting time.  And notes are broken up so
   that each pair of notes either cover the identical time interval or
   they cover disjoint intervals. 
   
   The following fields of each note are filled in:
   start, duration, pitch, npc, base_tpc, next
   */
Note * build_note_list_from_event_list(Event * e) {
    int event_time, previous_event_time;
    int on_event, i, xet, event_n;
    Pitch pitch;
    Note * note_list, * new_note;
    char pitch_in_use[MAX_PITCH];
    DirectNote * directnote[MAX_PITCH];  /* for each pitch in use we need the
					    direct note that created it */
    char already_emitted[MAX_PITCH];  /* to compute the is_first_note fields */
    int pitch_count[MAX_PITCH];  /* the number of times the pitch is turned on */
    Event *el;
    Event ** etable;
    int N_events;
    
    for (N_events=0, el = e; el != NULL; el = el->next) N_events++;
    etable = (Event **) xalloc (N_events * sizeof (Event *));
    for (i=0, el = e; el != NULL; el = el->next, i++) etable[i] = el;
    
    qsort(etable, N_events, sizeof (Event *), (int (*)(const void *, const void *))comp_event2);
    
    for(i=0; i<MAX_PITCH; i++) {
	pitch_in_use[i] = 0;
	already_emitted[i] = 0;
	directnote[i] = NULL;
	pitch_count[i] = 0;
    }
    
    event_time = 0;
    note_list = NULL;
    
    /*
      for (i=0; i<N_events; i++) {
      el = etable[i];
      printf("event: pitch = %d  note_on = %d  time = %d \n", el->pitch, el->note_on, el->time);
      }
      */
    
    for (event_n=0; event_n<N_events; event_n++) {
	el = etable[event_n];
	xet = el->time;
	pitch = el->pitch;
	on_event = el->note_on;

	/* we ignore an event that doesn't change which pitches are currently on,
	   that is, a situation where two notes are overlapping */
	if (on_event) {
	    pitch_count[pitch]++;
	    if (pitch_count[pitch] > 1) continue;
	} else {
	    pitch_count[pitch]--;
	    if (pitch_count[pitch] > 0) continue;
	}

	previous_event_time = event_time;
	event_time = xet;

	/* these two tests are now irrelevant -- cannot happen */
	if (on_event && pitch_in_use[pitch]) {
	    fprintf (stderr, "%s: Pitch %d is already on, but it just got turned on at time %d.\n", this_program, pitch, event_time);
	    my_exit(1);
	}
	
	if (!on_event && !pitch_in_use[pitch]) {
	    fprintf (stderr, "%s: Pitch %d is not on, but it just got turned off.\n", this_program, pitch);
	    my_exit(1);
	}
	
	/* now, for each pitch that's turned on, we generate a note */
	/* the onset time is previous_event_time, the duration is */
	/* the diff between that and event_time */
	
	if (previous_event_time > event_time) {
	    fprintf(stderr, "%s: Events are not sorted by time.\n", this_program);
	    my_exit(1);
	}
	
	if (previous_event_time != event_time) {
	    /* Don't create any notes unless some time has elapsed */
	    for(i=0; i<MAX_PITCH; i++) {
		if (pitch_in_use[i]) {
		    new_note = (Note*) xalloc(sizeof(Note));
		    new_note->pitch = i;
		    new_note->start = previous_event_time;
		    new_note->duration = event_time - previous_event_time;
		    new_note->npc = new_note->base_tpc = new_note->tpc = 0;
		    new_note->orn_dis_penalty = 0.0;
		    new_note->next = note_list;
		    new_note->is_first_note = (!already_emitted[i]);
		    new_note->directnote = directnote[i];
		    already_emitted[i] = 1;
		    note_list = new_note;
		}
	    }
	}
	pitch_in_use[pitch] = on_event;
	directnote[pitch] = el->directnote;  /* NULL if not an on_event */
	already_emitted[pitch] = 0;
    }
    
    for(i=0; i<MAX_PITCH; i++) {
	if (pitch_in_use[i]) {
	    fprintf(stderr, "%s: Pitch %d was still in use at the end.\n", this_program, i);
	    my_exit(1);
	}
    }
    
    for (new_note =  note_list; new_note != NULL; new_note = new_note->next) {
	new_note->npc = Pitch_to_NPC(new_note->pitch);
	new_note->base_tpc = base_TPC(new_note->npc);
    }
    
    xfree(etable);
    
    return reverse(note_list);
}

#if 0
OBS void build_note_array(Note * nl) {
OBS   Note * xnl;
OBS   int i;
OBS   for (N_notes = 0, xnl = nl; xnl != NULL; xnl = xnl->next) N_notes++;  /* compute N_notes */
OBS   note_array = (Note **) xalloc(N_notes * sizeof(Note *));
OBS   if (note_array == NULL) {
OBS     fprintf(stderr, "Malloc of note_array failed\n");
OBS     exit(1);
OBS   }
OBS   for (i=0, xnl = nl; xnl != NULL; xnl = xnl->next, i++) {
OBS     note_array[i] = xnl;
OBS   }
OBS }
#endif

void build_beat_array(void) {
    Beat *xnl;
    int i;
    for (N_beats = 0, xnl = global_beat_list; xnl != NULL; xnl = xnl->next) N_beats++;
    beat_array = (Beat **) xalloc(N_beats * sizeof(Beat *));
    if (beat_array == NULL) {
	fprintf(stderr, "%s: Malloc of beat_array failed\n", this_program);
	my_exit(1);
    }
    for (i=N_beats-1, xnl = global_beat_list; xnl != NULL; xnl = xnl->next, i--) {
	beat_array[i] = xnl;
    }
    /* The beats should now be in increasing order.  Let's check this. */
    for (i = 1; i<N_beats; i++) {
	if (beat_array[i-1]->start >= beat_array[i]->start) {
	    fprintf(stderr, "%s: The input contains beats that are not in sorted order.\n", this_program);
	    my_exit(1);
	}
    }
}


Event * build_event_list_from_direct_notes(void) {
    DirectNote *dn;
    Event *new_event, *event_list;

    event_list = NULL;
    for (dn = global_DN_list; dn != NULL; dn = dn->next) {
	new_event = (Event *) xalloc(sizeof(Event));
	new_event->pitch = dn->pitch;
	new_event->note_on = 1;
	new_event->time = dn->adjusted_start_time;
	new_event->directnote = dn;
	new_event->next = event_list;
	event_list = new_event;

	new_event = (Event *) xalloc(sizeof(Event));
	new_event->pitch = dn->pitch;
	new_event->note_on = 0;
	new_event->time = dn->adjusted_end_time;
	new_event->directnote = NULL;
	new_event->next = event_list;
	event_list = new_event;
    }
    return event_list;
}


/* We need to be able to take a time t, and lookup the time of the next
   beat less than or equal to it.   Return -1 if there is no beat
   before this.  */

int rlookup_prev_beat(int t, int l, int r) {
    /* search the range [l...r] inclusive. */
    /* we know that time[l] <= t < time[r] */
    int m;
    if (r <= l) return -1;  /* should not happen */
    if (l == r-1) return l;
    m = (r+l)/2;
    if (t < beat_array[m]->start) return rlookup_prev_beat(t, l, m);
    return rlookup_prev_beat(t, m, r);
}

int lookup_prev_beat(int t) {
    if (t >= beat_array[N_beats-1]->start) return N_beats-1;
    if (t < beat_array[0]->start) return -1;  /* signals the beat before the start */
    return rlookup_prev_beat(t, 0, N_beats-1);
}

int lookup_nearest_beat(int t) {
    /* find the nearest beat to the given time */
    int b;
    if (t >= beat_array[N_beats-1]->start) return N_beats-1;
    if (t <= beat_array[0]->start) return 0;
    b = lookup_prev_beat(t);
    if ((t - beat_array[b]->start) <= (beat_array[b+1]->start - t)) {
	return b;
    } else {
	return b+1;
    }
}

#if 0
OBS /* this adjusts the note list so that every note starts and ends on a beat */
OBS /* the original_start and original_duration fields are filled in */
OBS void adjust_note_list(Note *nl) {
OBS     for (; nl != NULL; nl = nl->next) {
OBS 	nl->original_start = nl->start;
OBS 	nl->original_duration = nl->duration;
OBS 	nl->start = lookup_beat_time(nl->original_start);
OBS 	nl->duration = lookup_beat_time(nl->original_start + nl->original_duration) - nl->start;
OBS     }
OBS }

OBS void adjust_event_list_times(Event *e) {
OBS     for (; e != NULL; e = e->next) {
OBS 	printf("adjusting: %d --> %d\n", e->time, lookup_beat_time(e->time));
OBS 	e->time = lookup_beat_time(e->time);
OBS     }
OBS }
#endif

/* Rounding.......  (Sun Jul 12 08:51:04 EDT 1998, --DS)
   
   Here's how the rounding currently works.  The original notes are
   rounded to pips by the meter program.  It rounds the starting time of
   the note to the nearest pip.  Then it rounds the duration to an
   integral number of pips.  Then it computes the end time of the note
   by adding these two together.

   Now the notes are "adjusted" by this program.  We round the starting
   and ending times of each note to the nearest beat.  This could *not*
   cause two notes that were formerly disjoint to now overlap.  But now
   some note may have zero length.  To deal with this we move the end of
   the note to the next beat.  The *can* now cause two notes to overlap.
   For example, considedr this:

      Beat  49875  2
      Beat  50050  0
      Beat  50260  1

      Note: start = 50155, duration = 105, adjusted_start_time = 50050, adjusted_end_time = 50260
      Note: start = 49980, duration = 105, adjusted_start_time = 50050, adjusted_end_time = 50260

   These two notes, which used to be disjoint are now identical.

   There are several ways to deal with this problem:

    (1) Allow the two notes to overlap and have the harmonic algorithm
        handle overlapping notes.
    
    (2) Change the rounding so that zero length notes are just deleted
        instead of having their endpoint moved.
	
    (3) Coalesce two overlapping notes into one note.

    (4) Choose one of the notes and throw out the other one
    
   In the current program, I adopted (roughly) the 4th choice.  This
   means that sometimes a note will be effectively deleted from the
   note list.  When this happens, it will print a wanring about it.

   The reason I say I've "roughly" adopted it is that my way of dealing
   with it is a bit of a kludge, because I didn't want to do too much
   rewriting of the program.  While it's running through the sequence
   of events, it ignores any event that turns on a pitch that is already
   on, and ignores an event that turns a pitch off which still leaves
   that pitch on.  Ok, but then it must pick one of the direct notes
   to correspond to this longer composite note.  Neither one may be
   exactly right.  The event sorting is set up so that it chooses the
   longer one (if they both start at the same time) and keeps that one.


   Some corrections to the above comment...   (Mon Aug  3 22:27:06 EDT 1998, --DS)

   In adjust-notes.c in the meter program, it now rounds the starting
   and ending times of the notes, each to the nearest pip.  As opposed
   to what is described above (which rounds the duration off to the
   nearest number of pips).

   Also, in the code below there are two different rounding modes
   allowed.  if round_to_beat=1, then the rounding to beats is as
   described above.  If round_to_beat=0 then we really truncate each
   starting and ending time to the nearest prior beat.  the rest of the
   above comment still holds.

 */

int lookup_right_beat(int t) {
    if (round_to_beat) {
	return lookup_nearest_beat(t);
    } else {
	return lookup_prev_beat(t);	
    }
}

void compute_adjusted_direct_notes(void) {
    DirectNote *dn;
    int ind1, ind2 ;
    for (dn = global_DN_list; dn != NULL; dn = dn->next) {
	ind1 = lookup_right_beat(dn->start);
	ind2 = lookup_right_beat(dn->start + dn->duration);
	if (ind1 == ind2) {
	    /* avoid 0 length notes by moving the right endpoint, unless there's no room, so move the left endpoint */
	    if (ind2 < N_beats-1) {
		ind2++;
	    } else {
		ind1--;
	    }
	}
	dn->adjusted_start_time = beat_array[ind1]->start;
	dn->adjusted_end_time = beat_array[ind2]->start;
    }
}

/* 
   Here's the procedure for allowing the program to deal with the "real" notes.
   These are the notes directly inferred from the input.  Call them the direct notes.

   First read the event list in.

   Now build the direct note list from the event list.  Every on event
   generates exactly one direct note. (Actually, we probably don't want to
   generate direct notes of zero length.)

   The note structure has a field that points to a direct note.  With every
   note that is used in the program we keep a pointer to the corresponding
   direct note.

   To label the direct notes with TPC labels, we compute these labels for all the
   notes of all the chords, and for each we follow the link to the direct note and
   install the TPC label there.

 */

Note * build_note_list_from_input(void) {
    /* build the note list, also create the note array, also set N_notes */
    Event * e;
    Note * nl;
    e = build_event_list_and_beat_list_from_input();
    if (e == NULL) {
	fprintf(stderr, "%s: No notes in the input.\n", this_program);
	my_exit(1);
    }

    build_beat_array();
    
    global_DN_list = build_direct_note_list_from_event_list(e);

    compute_adjusted_direct_notes();

    free_event_list(e);

    e = build_event_list_from_direct_notes();
    
    nl = build_note_list_from_event_list(e);

    free_event_list(e);
    
    return nl;
}
