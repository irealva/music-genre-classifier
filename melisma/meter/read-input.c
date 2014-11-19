
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

#include "meter.h"

/* This file contains the code that reads in the input. */

static int line_no;  /* the line number */
static char line[1000];

typedef struct event_struct {
  /* this structure is just used for reading in the notes */
  int note_on;  /* 1 if this is note on, 0 otherwise */
  Pitch pitch;
  int time;      /* the time of this event */
  int end_time;  /* filled in for events where "note_on" is 1 */
  struct event_struct * next;
} Event;


void free_event_list(Event *e) {
    Event *xe;
    for (; e != NULL; e = xe) {
	xe = e->next;
	
	xfree ((char *) e);
    }
}

void test_event(Event *event) {
    if (event->pitch < 0 || event->pitch > MAX_PITCH-1) {
	fprintf (stderr, "%s: Error (line %d) Found a pitch that is out of range: %d\n",
		 this_program, line_no, event->pitch);
	exit(1);
    }
    if (event->time < 0) {
	fprintf (stderr, "%s: Error (line %d) Found a negative event time: %d.\n",
		 this_program, line_no, event->time);
	exit(1);
    }
    if (event->note_on && event->end_time < 0) {
	fprintf (stderr, "%s: Error (line %d) Found a negative event time: %d.\n",
		 this_program, line_no, event->end_time);
	exit(1);
    }
}

void warn(void) {
    char * x;
    static int first_warning = 1;
    x = strchr(line, '\n');
    if (x != NULL) *x = '\0';
    if (first_warning) {
	fprintf(stderr, "%s: Ignoring the following lines:\n", this_program);
	first_warning = 0;
    }
    fprintf(stderr, "---> %s\n", line);
}

Event * build_event_list_from_input(void) {
    char first_part[100];
    int second_part;
    int third_part;
    int fourth_part;
    int event_time;
    int N_parts;
    int on_event, both_event, off_event, chord_event;
    int on_time, off_time;
    int i;
    int n=0;           /* Davy needs this */
    Pitch pitch;
    Event * event_list, * new_event;
    
    event_time = 0;
    event_list = NULL;
    
    for (line_no = 1; fgets(line, sizeof(line), instream) != NULL; line_no++) {
	for (i=0; isspace(line[i]); i++);
	if (line[i] == '%' || line[i] == '\0') continue;  /* ignore comment lines */
	N_parts = sscanf(line, "%99s %d %d %d", first_part, &second_part, &third_part, &fourth_part);
	if (N_parts < 2) {
	    warn();
	    continue;
	}
	
	on_event = (strcasecmp(first_part, "Note-on") == 0);
	both_event = (strcasecmp(first_part, "Note") == 0);
	off_event = (strcasecmp(first_part, "Note-off") == 0);
	chord_event = (strcasecmp(first_part, "Prechord") == 0);
	
	if (!(on_event || off_event || both_event || chord_event)) {
	    warn();
	    continue;
	}
	if (on_event || off_event) {
	    if (N_parts != 3) {
		fprintf(stderr, "%s: Error (line %d) a note needs a time and a pitch\n", this_program, line_no);
		exit(1);		
	    }
	    
	    event_time = second_part;
	    pitch = third_part;
	    
	    new_event = (Event *) xalloc(sizeof(Event));
	    new_event->pitch = pitch;
	    new_event->note_on = on_event;
	    new_event->time = event_time;
	    new_event->next = event_list;
	    event_list = new_event;
	}
	if (both_event) {
	    /* it's a both event */
	    if (N_parts != 4) {
		fprintf(stderr, "%s: (line %d) a note needs an on-time an off-time and a pitch\n", this_program, line_no);
		exit(1);		
	    }
	    on_time = second_part;
	    off_time = third_part;

	    pitch = fourth_part;
	    
	    new_event = (Event *) xalloc(sizeof(Event));
	    new_event->pitch = pitch;
	    new_event->note_on = 1;
	    new_event->time = on_time;
	    new_event->next = event_list;
	    event_list = new_event;
	    
	    new_event = (Event *) xalloc(sizeof(Event));
	    new_event->pitch = pitch;
	    new_event->note_on = 0;
	    new_event->time = off_time;
	    new_event->next = event_list;
	    event_list = new_event;
	}
	if (chord_event) {
	  prechord[n].time = second_part;
	  /* printf("prechord time at %d\n", prechord[n].time); */
	  ++n;
	  N_chords = n;
	}

	test_event(event_list);      
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

int comp_event(Event ** a, Event ** b) {
    if ((*a)->time != (*b)->time) return  ((*a)->time - (*b)->time);
    return ((*a)->note_on - (*b)->note_on);
}

int same_register(Pitch a, Pitch b) {
    return ((a-b) <= 9 && (a-b) >= -9);
}


/* This function builds a note list from an event list.  This note list
   is sorted by increasing starting time.

   The following fields of each note are filled in:
       start, duration, pitch, next
       and some more esoteric fields: ioi, rioi, effective_length
*/
Note * build_note_list_from_event_list(Event * e) {
    int event_time;
    int i, event_n;
    Note * note_list, * new_note, *nl, *xnl;
    Event *el;
    Event ** etable;
    int N_events;
    int last_time;
    
    for (N_events=0, el = e; el != NULL; el = el->next) N_events++;
    etable = (Event **) xalloc (N_events * sizeof (Event *));
    for (i=0, el = e; el != NULL; el = el->next, i++) etable[i] = el;
    
    qsort(etable, N_events, sizeof (Event *), (int (*)(const void *, const void *))comp_event);
    
    event_time = 0;
    note_list = NULL;
    
    for (i=0; i<N_events; i++) {
	el = etable[i];
	/*    printf("event: Error: pitch = %d  note_on = %d  time = %d \n", el->pitch, el->note_on, el->time); */
    }
    
    for (event_n=0; event_n<N_events; event_n++) {
	el = etable[event_n];
	if (!(el->note_on)) continue;
	for (i=event_n+1; i<N_events; i++) {
	    if (etable[i]->pitch == el->pitch) {
		el->end_time = etable[i]->time;
		break;
	    }
	}
	if (i == N_events) {
	    fprintf(stderr, "%s: Error: No note off found for pitch %d\n", this_program, el->pitch);
	    exit(1);
	}
    }
    
    /* now the end times for all the on-events are stored */
    /* now run through them and convert them into notes */
    
    note_list = NULL;
    for (event_n=0; event_n<N_events; event_n++) {
	el = etable[event_n];
	if (!(el->note_on)) continue;
	new_note = (Note*) xalloc(sizeof(Note));
	new_note->pitch = el->pitch;
	new_note->start = el->time;
	new_note->duration = el->end_time - el->time;
	new_note->next = note_list;
	note_list = new_note;
    }
    
    xfree(etable);

    note_list = reverse(note_list);


    /* now we need to figure out the time of the end of the piece for
       constructing the rioi fields */
    last_time = -1;
    for (nl = note_list; nl!=NULL; nl = nl->next) {
	if (last_time < nl->start+nl->duration) last_time = nl->start + nl->duration;
    }

    for (nl = note_list; nl!=NULL; nl = nl->next) {
	/* this loop computes the ioi and rioi of each note */
	if (nl->next == NULL) {
	    /* for the last note the ioi is defined to be the duration */
	    nl->ioi = nl->duration;
	    nl->rioi = nl->duration;
	    continue;
	}

	/* compute ioi */
	nl->ioi = nl->next->start - nl->start;
	if (nl->ioi < 0) {
	    fprintf(stderr, "%s: An IOI is negative..should not happen.\n", this_program);
	    exit(1);
	}
	/* compute rioi */
	for (xnl = nl->next; xnl != NULL; xnl=xnl->next) {
	    if (same_register(nl->pitch, xnl->pitch)) {
		nl->rioi = xnl->start - nl->start;
		break;
	    }
	}
	if (xnl == NULL) {
	    /* no following note in this register */
	    nl->rioi = last_time - nl->start;  /* the time till the end of the piece */
	}
    }

    for (nl = note_list; nl!=NULL; nl = nl->next) {
	/* now compute the effective_length of each note, which is
	   just the maximum of the duration and the rioi, but not more than
	   max_effective_length
	   */
	nl->effective_length = MAX(nl->duration, nl->rioi);
	nl->effective_length = MIN((int)(1000 * max_effective_length), nl->effective_length);
    }

#if 0
    for (nl = note_list; nl!=NULL; nl = nl->next) {
	if (nl->next == NULL) {
	    /* for the last note the ioi is defined to be the duration */
	    nl->ioi = nl->duration;
	} else {
	    nl->ioi = nl->next->start - nl->start;
	    if (nl->ioi <= 0) {
		fprintf(stderr, "%s: An IOI is non-positive.\n", this_program);
		exit(1);
	    }
	}
    }
#endif    
    return note_list;
}

void print_note_list(Note *nl) {
    for (; nl != NULL; nl = nl->next) {
	printf("Note %6d %6d %2d\n", nl->start, nl->start + nl->duration, nl->pitch);
    }
}

void build_note_array(Note * nl) {
    Note * xnl;
    int i;
    for (N_notes = 0, xnl = nl; xnl != NULL; xnl = xnl->next) N_notes++;  /* compute N_notes */
    note_array = (Note **) xalloc(N_notes * sizeof(Note *));
    if (note_array == NULL) {
	fprintf(stderr, "%s: Malloc of note_array failed\n", this_program);
	exit(1);
    }
    for (i=0, xnl = nl; xnl != NULL; xnl = xnl->next, i++) {
	note_array[i] = xnl;
    }
}

Note * build_note_list_from_input(void) {
    /* build the note list, also create the note array, also set N_notes */
    Event * e;
    Note * nl;
    e = build_event_list_from_input();
    nl = build_note_list_from_event_list(e);
    build_note_array(nl);
    free_event_list(e);

    if (verbosity > 1) print_note_list(nl);
    return nl;
}

void normalize_notes(Note *nl) {
    /* this just renormalizes the onset times of the notes so that the
       first note starts at time 0 */
    Note *nn;
    int min_time;
    if (nl == NULL) return;
    min_time = nl->start;
    for (nn = nl; nn != NULL; nn = nn->next) {
	if (min_time > nn->start) min_time = nn->start;
    }
    for (nn = nl; nn != NULL; nn = nn->next) {
	nn->start -= min_time;
    }
}
