
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

#include "harmony.h"

#define NOTE_RELABLE_PENALTY (1000.0)

/* The purpuse of the code in this file is to take a metered chord list
   (as computed by build_metered_chord_representation()), and label each
   chord of it with a root, which is a member of a TPC. 

   The labeling is chosen so as to maximize the sum of four components:

     Compatibility of the proposed root label with the notes in it.
     variance penalty of the sequence of root labels.
     strong beat penalty which occurs when a root begins
     ornamental dissonance penaty.
 */

/* This function computes the compatibility score of a note (in terms of
   its TPC) and with respect to a particular root.  This is for 1 second
   of duration. */


/*

variance: half life of 2 seconds....

variance penalty for each "chord span".
Its distance from its center of gravity times
its duration (in seconds) times 3.

strong beat penalty.  Penalizes chord changes that are on weak beats.
(For example, rapid chord changes are penalized.)  It's

      MAX( (2/S) - 1.5 , 0)

where S is the length of the highest level beat of the beginning of this
chord.  (It's zero if there's no change between this and the previous
chord.)

Ornamental dissonance penalty:
   It is applied when a note has a compatibility of 0 with its root.
   Take the time in seconds between a note's onset and the onset of the
   next note whose pitch differs by -2, -1, 1, or 2 from the current note.
   Let that time be D.  The penalty is 2 + 3D + D^2

   If (in subsequent processing) a note is split into several notes or
   chords, the ornamental dissonance penalty is only applied to the
   FIRST such note.  This note-wise hypothetical penalty should be
   computed for all of the notes once at the beginning.


Voice Leading Rule:

  Penalty for spelling two notes that are a half step apart in pitch and
  close together in time, spelling them 7 steps apart on the line of
  fifths.  (Rather than 5 steps apart on the line of fifths.)  But it's
  only bad when the first of these two notes is chromatic.
  (Chromatic here means far from the current center of gravity.)

  Suppose you have G G# A, and you're trying to decide how to spell it.

  Could spell this G G# A or G Ab A.  Assuming that we're in the key of
  Cmajor, then it will prefer the former one. because.....the middle
  note is the chromatic one and if spelled as Ab it's 7 steps from the
  following A.

  Proposed way to implement this:

  Find all notes that are followed closely by a note that's a half step
  away from it.  Say, within a time interval of 2 seconds.  (Similar to
  the ornamental dissonance rule, which uses the IOI and allows 1/2
  steps or whole steps.)

  Actually, look at the end of a note, and look for the beginning notes
  that being after the end of the first note, and apply it.
  (Should apply if one note ends just as the new one begins.)

  If the current note is closely followed by a note 1/2 step above then
  it's a "lower neighbor" ("upper neighbor" is the opposite).  In case
  it's both, then choose the one where the subsequent note is first.  In
  case this is a tie, then make it a lower neighbor.

  Basic idea if it's an upper neighbor, then it should be a flat
  relative to the COG, if it's a lower neighbor it should be sharp.

  So if it's an upper neighbor you impose the penalty if it's more than
  like 3 steps above the current TPC COG.  And vice versa.

*/

int next_prime_up(int start) {
/* return the next prime up from start */
  int i;
  start = start | 1; /* make it odd */
  for (;;) {
    for (i=3; (i <= (start/i)); i += 2) {
      if (start % i == 0) break;
    }
    if (start % i == 0) {
      start += 2;
    } else {
      return start;
    }
  }
}

/* int table_size; */
static int hash_prime_1;
static int hash_prime_2;
static int hash_prime_3;
static int hash_prime_4;

static long int total_hash_cost;
static long int N_hash_lookups;

void initialize_hashing(void) {
    table_size = next_prime_up(4000);
    hash_prime_1 = next_prime_up(100);
    hash_prime_2 = next_prime_up(200);
    hash_prime_3 = next_prime_up(300);
    hash_prime_4 = next_prime_up(400);
    
    total_hash_cost = 0;
    N_hash_lookups = 0;
}

int hash(int int_tpc_cog, int int_har_cog, TPC root, TPC window) {
    /* given these parameters, this figures out which bucket it goes in */
    int x;
    x = hash_prime_1 * root
	+ hash_prime_2 * int_tpc_cog
	+ hash_prime_3 * int_har_cog
	+ hash_prime_4 * window;
    return (abs(x) % table_size);
}

void free_hash_table(Bucket ** table) {
    int i;
    Bucket * pn, * xpn;
    
    for (i = 0; i<table_size; i++) {
	for (pn = table[i]; pn != NULL; pn = xpn) {
	    xpn = pn->next;
	    xfree(pn);
	}
    }
    xfree(table);
}

Bucket ** create_hash_table(void) {
    int i;
    Bucket ** table;
    table = (Bucket **) xalloc(table_size * sizeof (Bucket *));
    if (table == NULL) {
	fprintf(stderr, "%s: Could not allocate a table of size %d\n", this_program, table_size);
	my_exit(1);
    }
    for (i = 0; i<table_size; i++) {
	table[i] = NULL;
    }
    return table;
}

Bucket * lookup_in_table(int int_tpc_cog, int int_har_cog, TPC root, TPC window, Bucket ** table) {
    int h;
    Bucket * pn;
    
    N_hash_lookups++;
    h = hash(int_tpc_cog, int_har_cog, root, window);
    for (pn = table[h]; pn != NULL; pn = pn->next) {
	if (pn->int_tpc_cog == int_tpc_cog && pn->int_har_cog == int_har_cog && pn->root == root && pn->window == window) return pn;
	total_hash_cost++;
    }
    return NULL;
}

Bucket * insert_into_table(int int_tpc_cog, int int_har_cog, TPC root, TPC window, Bucket ** table) {
    int h;
    Bucket * pn;
    h = hash(int_tpc_cog, int_har_cog, root, window);
    pn = (Bucket *) xalloc(sizeof(Bucket));
    if (pn == NULL) {
	fprintf(stderr, "%s: malloc failed while allocating a bucket\n", this_program);
	my_exit(1);    
    }
    
    pn->next = table[h];
    table[h] = pn;
    pn->int_har_cog = int_har_cog;
    pn->int_tpc_cog = int_tpc_cog;
    pn->root = root;
    pn->window = window;
    pn->score = 0.0; 
    return pn;
}

Bucket * put_in_table(int int_tpc_cog, int int_har_cog, TPC root, TPC window, Bucket ** table) {
    /* if there's an appropriate bucket, return it, otherwise insert it */
    /* not super efficient cause it computes the hash function twice */
    Bucket *b;
    b = lookup_in_table(int_tpc_cog, int_har_cog, root, window, table);
    if (b != NULL) return b;
    return insert_into_table(int_tpc_cog, int_har_cog, root, window, table);
}


void cleanup_harmonic (void) {
    int i;
    for (i=0; i<N_chords; i++) {
	free_hash_table(column_table[i].table);
    }
    xfree(column_table);
}

double ornamental_dissonance_penalty(double delta) {
    /* this must be an increasing function of delta (for positive delta)  */
    /* or the code won't find the minimum one */
    return odp_constant + odp_linear_factor*delta + odp_quadratic_factor*delta*delta;
}

#define DEFAULT_TIME 10.0
/* the amount of time to give an ornamental dissonance penalty
   when there is no resolving note */

void label_notes_with_ornamental_dissonance_penalties(Note * nl) {
    Note * note, *f_note;
    double delta_t;
    if (verbosity >= 4) printf("Ornamental dissonance penalties:\n");
    for (note = nl; note != NULL; note = note->next) {
	note->orn_dis_penalty = ornamental_dissonance_penalty(DEFAULT_TIME);
	delta_t = DEFAULT_TIME;
	for (f_note = note->next; f_note != NULL; f_note = f_note->next) {
	    /* use this value only if you don't find any real note to define the penalty */
	    if (note->start == f_note->start) continue;  /* skip all notes that start when I do */
	    if (note->pitch == f_note->pitch + 1  ||  note->pitch == f_note->pitch - 1  ||
		note->pitch == f_note->pitch + 2  ||  note->pitch == f_note->pitch - 2) {
		delta_t = (f_note->start - note->start)/(1000.0);  /* time in seconds */
		note->orn_dis_penalty = ornamental_dissonance_penalty(delta_t);
		break;  /* make it find the nearest note with dissonance */
	    }
	}
	if (verbosity >= 4) {
	    printf("pitch = %2d  start = %4d  duration = %4d  delta_t = %6.3f  od = %6.3f\n",
		   note->pitch, note->start, note->duration, delta_t, note->orn_dis_penalty);
	}
    }
}

/* Modify the ornamental dissonance according to the level_times
   (that were not available at the time the above function computed
   the odps. */
void modify_ornamental_dissonance_penalties(Chord *m_clist) {
    float odp_modifier;
    Chord * ch;
    Note *note;
    for (ch = m_clist; ch!=NULL; ch=ch->next) {
        /* 9/3 - Davy changed this: the modifying factor is now the max of 1.0 and (1.4)*sqrt(ch->level_time/1000.0) */
	/* the following test avoids updating the ODP twice for a given note. */
	if (ch->is_first_chord) {
	    for (note=ch->note; note != NULL; note=note->next) {     
	        if ((1.4)*sqrt(ch->level_time/1000.0) > 1.0) odp_modifier=1.0; 
		else odp_modifier = (1.4)*sqrt(ch->level_time/1000.0); 
		note->orn_dis_penalty *= odp_modifier;		
	    }
	}
    }
}

void label_notes_with_voice_leading_neighbor(Note *nl) {
    Note * note, *f_note;
    double delta_t;
    if (verbosity >= 4) printf("Voice Leading Neighbors:\n");
    for (note = nl; note != NULL; note = note->next) {
	note->voice_leading_neighbor = 0; /* default unless we find a neighbor */
	for (f_note = note->next; f_note != NULL; f_note = f_note->next) {
	    delta_t = (f_note->start - (note->start + note->duration))/(1000.0);  /* time in seconds */
	    if (delta_t < 0) continue;
	    if (delta_t > voice_leading_time) break;
	    if (note->pitch == f_note->pitch + 1) {
		/* note is an upper neighbor */
		note->voice_leading_neighbor = 1;
		break;
	    }
	    if (note->pitch == f_note->pitch - 1) {
		/* note is a lower neighbor */
		note->voice_leading_neighbor = -1;
		break;
	    }
	}
	if (verbosity >= 4) {
	    printf("pitch = %2d  start = %4d  duration = %4d  voice_leading_neighbor = %2d\n",
		   note->pitch, note->start, note->duration, note->voice_leading_neighbor);
	}
    }
}
	
double compatibility(TPC root, TPC note) {
    /* if the abs of this is > .01 it's considered to be non-zero
       from the perspective of computing ornamental dissonance */
    switch (note - root) {
    case -6: return compat_value[0];
    case -5: return compat_value[1];
    case -4: return compat_value[2];
    case -3: return compat_value[3];
    case -2: return compat_value[4];
    case -1: return compat_value[5];
    case 0: return compat_value[6];
    case 1: return compat_value[7];
    case 2: return compat_value[8];
    case 3: return compat_value[9];
    case 4: return compat_value[10];
    case 5: return compat_value[11];
    default: return -10.0;
    }
}


double note_ornamental_dissonance_penalty(TPC root, TPC tpc, Chord * chord, Note * note, int same_roots) {
    /* same_roots is 1 if the root at this point and the previous point are the same */
    if (same_roots && !chord->is_first_chord) return 0.0;
    if (same_roots && !note->is_first_note) return 0.0;
    if (compatibility(root, tpc) != -10.0) return 0.0;  /* used to be compatibility != 0 */
    else return note->orn_dis_penalty;
}

double tpc_variance(double cog, TPC tpc, double my_mass, double decayed_prior_note_mass) {
    double delta_cog;
    delta_cog = fabs(cog - (double) tpc);
    return (delta_cog * delta_cog * my_mass);
    /* Davy changed this: used to be   return (delta_cog * delta_cog * my_mass * decayed_prior_note_mass); */
}

TPC apply_window(TPC base_tpc, TPC window) {
    /* Translate the base_tpc into the [window, window+12) */
    return (base_tpc - window + 12000000)%12 + window;  /* 12000000 is to get C to to mods properly */
}

int is_canonical_window(Note *note, TPC window) {
    /* it's "canonical" if one of the notes of the chord is mapped to 
       window by the apply_window function.  In case the chord has no notes, only
       the window=0 is considered canonical */
    if (note == NULL) return (window == 0);
    for (; note != NULL; note = note->next) {
	if (window == apply_window(note->base_tpc, window)) return TRUE;
    }
    return FALSE;
}

int windows_differ_in_chord(TPC window1, TPC window2, Chord *chord) {
    /* Return TRUE if the two given windows give different TPCs for some note of this chord */
    /* Applies if this chord is a continuation, or to notes that are continuations */
    Note * note;
    for (note = chord->note; note != NULL; note = note->next) {
	if ((!note->is_first_note || !chord->is_first_chord)
	    && apply_window(note->base_tpc, window1) != apply_window(note->base_tpc, window2)) return TRUE;
    }
    return FALSE;
}

double compute_voice_leading_penalty(Chord *chord, double tpc_cog, int window) {
    Note * note;
    double total;
    total = 0.0;
    if (!chord->is_first_chord) return 0.0;
    for (note = chord->note; note != NULL; note = note->next) {
	if (!note->is_first_note) continue;
	if ((note->voice_leading_neighbor == 1) && (apply_window(note->base_tpc, window) > 4.0 + tpc_cog)) {
	    total += voice_leading_penalty;
	}
	if ((note->voice_leading_neighbor == -1) && (apply_window(note->base_tpc, window) < -4.0 + tpc_cog)) {
	    total += voice_leading_penalty;
	}
    }
    return total;
}

void tpc_choice_score(TPC root, TPC window, int same_roots, Chord *ch, double my_mass, double decayed_prior_note_mass, double tpc_cog) {
    /* tpc_cog is the PRIOR center of gravity. */
    
    int i, nnotes;
    Note *note;
    TPC tpc;
    double score, compat, orn_diss_penalty, variance;
    double average_tpc;
    
    for (nnotes=0, note=ch->note; note != NULL; note = note->next) nnotes++;
    
    side_effect.compatibility = 0.0;
    side_effect.orn_diss_penalty = 0.0;
    side_effect.tpc_variance = 0.0;
    
    /* strong beat penalty */
    if (same_roots) {
	side_effect.strong_beat_penalty = 0.0;
    } else {
	if (prechord_mode) {
	    side_effect.strong_beat_penalty = ((sbp_weight * 1000.0)/ch->level_time) - sbp_constant;  
            /* Alternative penalty in "prechord" mode */
	    /* Actually this is identical to the below ---DS */
	} else {
	    side_effect.strong_beat_penalty = ((sbp_weight * 1000.0)/ch->level_time) - sbp_constant;
	}
	if (side_effect.strong_beat_penalty < 0.0) side_effect.strong_beat_penalty = 0.0;
    }
    
    for (i=0, note=ch->note; note != NULL; note = note->next, i++) {
	tpc = apply_window(note->base_tpc, window);
	    
	/* compatibility */
	if(compatibility(root, tpc) == -10.0) compat = 0;
	else compat = compatibility(root, tpc) * my_mass * compat_factor;
	
	/* orn dis penalty */
	orn_diss_penalty = note_ornamental_dissonance_penalty(root, tpc, ch, note, same_roots);
	
	/* tpc variance */
	variance = tpc_variance(tpc_cog, tpc, my_mass, decayed_prior_note_mass);
	
	score = compat - orn_diss_penalty - tpc_var_factor * variance;
	
	side_effect.tpc_choice[i] = tpc;

	side_effect.compatibility += compat;
	side_effect.orn_diss_penalty += orn_diss_penalty;
	side_effect.tpc_variance += variance;
    }
    /* now compute the tpc_cog, and put it in side_effect.tpc_cog */
    
    average_tpc = 0;
    for (i=0; i<nnotes; i++) {
	average_tpc += (double)(side_effect.tpc_choice[i])/nnotes;
    }
    side_effect.tpc_cog =
	(tpc_cog * decayed_prior_note_mass + average_tpc * nnotes * my_mass)/(nnotes * my_mass + decayed_prior_note_mass);
}

void prune_table(Bucket ** table, int column) {
    int h;
    Bucket * bu, *xbu, *best, *nbu;
    int count=0, badcount=0;
    
    best = NULL;
    for (h=0; h<table_size; h++) {
	for (bu = table[h]; bu != NULL; bu = bu->next) {
	    if(best == NULL || best->score < bu->score) best = bu;
	    count++;
	}
    }
    
    for (h=0; h<table_size; h++) {
	nbu = NULL;
	for (bu = table[h]; bu != NULL; bu = xbu) {
	    xbu = bu->next;
	    
	    if (bu->score < best->score - pruning_cutoff) {
		badcount++;
		xfree(bu);
	    } else {
		bu->next = nbu;
		nbu = bu;
	    }
	}
	table[h] = nbu;
    }
    if (verbosity >= 1) {
	printf("Finished unit %3d.  (keeping %5d options out of %5d)\n", column, count-badcount, count);
    }
}

void initialize_harmonic(Chord *nl) {
    Chord * xnl;
    Note * note;
    double delta_t, decay;
    int i, n;
    for (N_chords = 0, xnl = nl; xnl != NULL; xnl = xnl->next) N_chords++;  /* compute N_chords */
    
    column_table = (Column*) xalloc (N_chords * sizeof(Column));
    if (column_table == NULL) {
	fprintf(stderr, "%s: Malloc failed\n", this_program);
	my_exit(1);
    }
    for (i = 0, xnl = nl; xnl != NULL; xnl = xnl->next, i++) {
	for (n=0, note=xnl->note; note != NULL; note = note->next) n++; /* count number of notes */
	
	column_table[i].chord = xnl;
	column_table[i].table = create_hash_table();
	
	column_table[i].my_mass = ((double) xnl->duration)/(1000.0);
	
	/* this part computes the chord masses */
	if (i == 0) {
	    column_table[i].chord_mass = column_table[i].my_mass;
	    column_table[i].note_mass = n*column_table[i].my_mass;
	    column_table[i].decayed_prior_note_mass = 0.0;
	    column_table[i].decayed_prior_chord_mass = 0.0;
	} else {
	    delta_t = (double) xnl->start - column_table[i-1].chord->start;
	    decay = exp(-alpha * delta_t);
	    column_table[i].chord_mass = decay * column_table[i-1].chord_mass + column_table[i].my_mass;
	    column_table[i].note_mass = decay * column_table[i-1].note_mass + n*column_table[i].my_mass;
	    column_table[i].decayed_prior_note_mass = column_table[i-1].note_mass * decay;
	    column_table[i].decayed_prior_chord_mass = column_table[i-1].chord_mass * decay;
	}
	if (verbosity >= 4) {
	    printf("chord_mass= %6.3f  note_mass= %6.3f  dpnm= %6.3f  dpcm= %6.3f\n", 
		   column_table[i].chord_mass,
		   column_table[i].note_mass,
		   column_table[i].decayed_prior_note_mass,
		   column_table[i].decayed_prior_chord_mass);
	}
    }
}

int discrete_cog(double cog) {
    return (int) (cog * buckets_per_unit_of_cog);
}

void initialize_first_harmonic_column(void) {
    int b;
    TPC root, window;
    int tpc_prime;
    double cog;
    Bucket *buck;
    for (window = LOWEST_TPC; window <= HIGHEST_TPC-11; window++) {
	if (!is_canonical_window(column_table[0].chord->note, window)) continue;
	for (root = -4; root <= 7; root++) {
	    for (tpc_prime = root-5; tpc_prime <= root+6; tpc_prime++) {
		cog = (double) root;
		b = discrete_cog(cog);

		tpc_choice_score(root, window, 0, column_table[0].chord, column_table[0].my_mass, 1.0, (double)tpc_prime);
		/* we prime the initial column with an artifical decayed_prior_mass of 1.0, and
		   an artificial tpc_cog of tpc_prime. */

		buck = insert_into_table(discrete_cog(side_effect.tpc_cog), b, root, window, column_table[0].table);
		buck->tpc_prime = tpc_prime;
		buck->har_cog = cog;

		buck->tpc_cog = side_effect.tpc_cog;
		buck->har_variance = 0.0;
		buck->tpc_variance = side_effect.tpc_variance;
		buck->score =
		    + side_effect.compatibility
		    - side_effect.orn_diss_penalty
		    - compute_voice_leading_penalty(column_table[0].chord, side_effect.tpc_cog, window);
		/* we include all costs that don't depend on the previous choices */
		buck->prev_bucket = NULL;
	    }
	}
    }
}

void compute_harmonic_table(void) {
    int column;
    int my_root;
    TPC window;
    double har_variance, score, note_relable_penalty, local_voice_leading_penalty;
    double current_cog;
    double new_cog;
    double delta_cog;
    Bucket * bu, *bu1;
    int int_har_cog, int_tpc_cog, new_guy, h;
    
    initialize_first_harmonic_column();
    for (column = 1; column<N_chords; column++) {
	/* compute this column of the table */
	for (window = LOWEST_TPC; window <= HIGHEST_TPC-11; window++) {
	    if (!is_canonical_window(column_table[column].chord->note, window)) continue;
	    for (h=0; h<table_size; h++) {
		for (bu = column_table[column-1].table[h]; bu != NULL; bu = bu->next) {
		    for (my_root = floor(bu->har_cog-12.0); my_root <= ceil(bu->har_cog+12.0); my_root++) {
			current_cog = (double) my_root;

			/* compute variance */
			delta_cog = (current_cog - bu->har_cog);
			/*	  har_variance = delta_cog * column_table[column].my_mass * column_table[column-1].chord_mass; */
			har_variance = fabs(delta_cog * column_table[column].my_mass * har_var_factor); 
			/* instead of total mass, use har_var_factor parameter value (normally 3.0) */

			/* the following computes the compatibility, strong_beat_penalty, and ornamental dissonance penalty */
			tpc_choice_score(my_root, window, bu->root == my_root, column_table[column].chord,
					 column_table[column].my_mass, column_table[column].decayed_prior_note_mass, bu->tpc_cog);

			note_relable_penalty = 0.0;
			if (windows_differ_in_chord(bu->window, window, column_table[column].chord)) {
			    note_relable_penalty = NOTE_RELABLE_PENALTY;
			}

			local_voice_leading_penalty = compute_voice_leading_penalty(column_table[column].chord, side_effect.tpc_cog, window);

			score = bu->score
			    + side_effect.compatibility
			    - side_effect.orn_diss_penalty
			    - har_variance
			    - tpc_var_factor * side_effect.tpc_variance
			    - side_effect.strong_beat_penalty
			    - note_relable_penalty
			    - local_voice_leading_penalty;

			new_cog = (bu->har_cog * column_table[column].decayed_prior_chord_mass
				   + current_cog * column_table[column].my_mass) /column_table[column].chord_mass;

			int_tpc_cog = discrete_cog(side_effect.tpc_cog);
			int_har_cog = discrete_cog(new_cog);
			bu1 = lookup_in_table(int_tpc_cog, int_har_cog, my_root, window, column_table[column].table);
			new_guy = (bu1 == NULL);
			if (bu1 == NULL) bu1 = insert_into_table(int_tpc_cog, int_har_cog, my_root, window, column_table[column].table);

			/* now we look in that bucket and update what's there if this is a better solution */
			if (new_guy || score > bu1->score) {
			    bu1->score = score;
			    bu1->har_variance = har_variance;
			    bu1->tpc_variance = side_effect.tpc_variance;
			    bu1->prev_bucket = bu;
			    bu1->har_cog = new_cog;
			    bu1->tpc_cog = side_effect.tpc_cog;
			}
		    }
		}
	    }
	}
	prune_table(column_table[column].table, column);
    }
}

void print_harmonic(void) { 
    Note * note;
    Chord *chord;
    double loc_voice_leading_penalty;
    int i, j, h;
    Bucket ** bucket_choice;
    Bucket * best_b, *bb, *bu;
    
    bucket_choice = (Bucket **) xalloc(N_chords * sizeof (Bucket *));
    
    best_b = NULL;
    
    for (h=0; h<table_size; h++) {
	for (bu = column_table[N_chords-1].table[h]; bu != NULL; bu = bu->next) {
	    if(best_b == NULL || best_b->score < bu->score) best_b = bu;
	}
    }
    
    if (best_b == NULL) {
	fprintf(stderr, "%s: No bucket used\n", this_program);
	my_exit(1);
    }
    
    for (i = N_chords-1; i >= 0; i--) {
	bucket_choice[i] = best_b;
	best_b = best_b->prev_bucket;
    }
    
    printf("Harmonic analysis:\n");

    
    for (i=0; i<N_chords; i++) {
	chord = column_table[i].chord;
	bb = bucket_choice[i];
	
	printf(" ");
	for (j=0; j<N_beatlevel; j++) if (chord->level >= j) printf("x "); else printf("  ");

	
	if (i == 0) {
	    tpc_choice_score(bb->root, bb->window, 0, chord,
			     column_table[i].my_mass, 1.0, (double) bb->tpc_prime);
	} else {
	    tpc_choice_score(bb->root, bb->window, bucket_choice[i-1]->root == bb->root, chord,
			     column_table[i].my_mass, column_table[i].decayed_prior_note_mass, bucket_choice[i-1]->tpc_cog);
	}

    /*
      Use the score that's in the bucket already, so when this changes, the below
      code still works.
OBS 	score =   side_effect.compatibility
OBS 	    - side_effect.orn_diss_penalty
OBS 	    - bb->har_variance
OBS 	    - tpc_var_factor * bb->tpc_variance
OBS 	    - side_effect.strong_beat_penalty;
    */
	
	printf("start = %5d  duration = %5d  notes: {", chord->start, chord->duration);
	for (j=0, note = chord->note; note != NULL; note = note->next, j++) {
	  printf("(%2d,%2d)", note->pitch, side_effect.tpc_choice[j]);
	}
	printf("}\n");
	
	/* note side_effect.tpc_variance == bb->tpc_variance  */

	loc_voice_leading_penalty = compute_voice_leading_penalty(chord, side_effect.tpc_cog, bb->window);
	
	printf("    root=%d(%-3s) score=%6.3f  score/sec=%6.3f com=%6.3f  har_cog=%6.3f  har_var=%6.3f\n"
	       "    sbp=%6.3f  odp=%6.3f  tpc_cog=%6.3f  scaled tpc_var=%6.3f  vlp = %3.1f\n",
	       bb->root, tpc_string(bb->root), bb->score, (bb->score*1000) / chord->duration,
	       side_effect.compatibility, bb->har_cog, bb->har_variance,
	       side_effect.strong_beat_penalty, side_effect.orn_diss_penalty, bb->tpc_cog,
	       tpc_var_factor * bb->tpc_variance, loc_voice_leading_penalty);
	
    }
    
    if (verbosity >= 3)  {
	printf("Scores chord by chord:\n");
	for (i=0; i<N_chords; i++) {
	    chord = column_table[i].chord;
	    bb = bucket_choice[i];
	    printf("  chord = %3d  Score = %50.30f\n", i, bb->score);
	}
    }
    
    xfree(bucket_choice);
}

void print_prechords(void) { 
    Chord *chord;
    int i, h;
    int oldroot=1000;        /* Davy needs */
    Bucket ** bucket_choice;
    Bucket * best_b, *bb, *bu;
    
    bucket_choice = (Bucket **) xalloc(N_chords * sizeof (Bucket *));
    
    best_b = NULL;
    
    for (h=0; h<table_size; h++) {
	for (bu = column_table[N_chords-1].table[h]; bu != NULL; bu = bu->next) {
	    if(best_b == NULL || best_b->score < bu->score) best_b = bu;
	}
    }
    
    if (best_b == NULL) {
	fprintf(stderr, "%s: No bucket used\n", this_program);
	my_exit(1);
    }
    
    for (i = N_chords-1; i >= 0; i--) {
	bucket_choice[i] = best_b;
	best_b = best_b->prev_bucket;
    }
    
    for (i=0; i<N_chords; i++) {
	chord = column_table[i].chord;
	bb = bucket_choice[i];
	
	if (oldroot != bb->root && oldroot != 1000) {
	  printf("Prechord %5d\n", chord->start);
	}
	oldroot = bb->root;
    }   
    
    xfree(bucket_choice);
}

char * note_string(Note *note) {
    static int i = 0;
    static char str[2][100];
    char * answer;
    i = (i+1) % 2;
    answer = str[i];
    sprintf(answer, "[%d %d] pitch = %d  is_first_note = %d directnote = %lu",
	    note->start, note->start+note->duration, note->pitch, note->is_first_note,
	    (long int) note->directnote);
    return answer;
}

void compute_direct_notes_TPCs(int should_print_chords) { 
    /* This function assignes a TPC value to all of the direct notes in the 
       global_DN_list */
    Note * note;
    Chord *chord;
    int i, j, h;
    Bucket ** bucket_choice;
    Bucket * best_b, *bb, *bu;
    
    bucket_choice = (Bucket **) xalloc(N_chords * sizeof (Bucket *));
    
    best_b = NULL;
    
    for (h=0; h<table_size; h++) {
	for (bu = column_table[N_chords-1].table[h]; bu != NULL; bu = bu->next) {
	    if(best_b == NULL || best_b->score < bu->score) best_b = bu;
	}
    }
    
    if (best_b == NULL) {
	fprintf(stderr, "%s: No bucket used\n", this_program);
	my_exit(1);
    }
    
    for (i = N_chords-1; i >= 0; i--) {
	bucket_choice[i] = best_b;
	best_b = best_b->prev_bucket;
    }
    
    for (i=0; i<N_chords; i++) {
	chord = column_table[i].chord;
	bb = bucket_choice[i];
	
	if (i == 0) {
	    tpc_choice_score(bb->root, bb->window, 0, chord,
			     column_table[i].my_mass, 1.0, (double) bb->tpc_prime);
	} else {
	    tpc_choice_score(bb->root, bb->window, bucket_choice[i-1]->root == bb->root, chord,
			     column_table[i].my_mass, column_table[i].decayed_prior_note_mass, bucket_choice[i-1]->tpc_cog);
	}

	/* debugging stuff
	if (chord->start >= 49000 && chord->start <= 50800) {
	    for (j=0, note = chord->note; note != NULL; note = note->next, j++) {
		printf("----> j=%d   chord = %lu [%d %d]   %s\n", j, (long int) chord, chord->start, chord->start + chord->duration, note_string(note));
	    }
	}
	*/

	for (j=0, note = chord->note; note != NULL; note = note->next, j++) {
	    if (note->directnote->tpc != UNINITIALIZED_TPC && note->directnote->tpc != side_effect.tpc_choice[j]) {
		fprintf(stderr, "%s: Conflicting TPC values for a note.  time = %d  note = %d\n", this_program, note->start, note->pitch);
		fprintf(stderr, "----> j=%d   chord = %lu [%d %d]   %s\n",
			j, (long int) chord, chord->start, chord->start + chord->duration, note_string(note));
		my_exit(1);
	    }
	    note->directnote->tpc = side_effect.tpc_choice[j];
	}
    }    

    if (should_print_chords) {
	for (i=0; i<N_chords; i++) {
	    chord = column_table[i].chord;
	    bb = bucket_choice[i];
	    printf("Chord %6d %6d %2d\n", chord->start, (chord->start)+(chord->duration), bb->root);
	}
    }

    xfree(bucket_choice);
}

