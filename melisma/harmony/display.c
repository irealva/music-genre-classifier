
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

#include "harmony.h"


/*   Cb  Gb   Db   Ab  Eb  Bb  F   C   G   D   A   E   B   F#  C#  G#   D#   A#   E#   B
                   ... -1  0   1   2   3   4   5   6   7   8  .....
 */

char letters[7] = {'F', 'C', 'G', 'D', 'A', 'E', 'B'};

char * tpc_string(TPC t) {
    static char answer[20];
    int i;
    int let;
    int sharps;
    char accidental;
    
    let = (t-1) % 7;
    if (let < 0) let += 7;   /* let = t-1 mod 7 */
    sharps = (t-1-let)/7;
    
    if  (sharps < 0) {
	accidental = 'b';
	sharps = -sharps;
    } else {
	accidental = '#';
    }
    
    answer[0] = letters[let];
    for (i=0; i<sharps && i+1<sizeof(answer)-1; i++) answer[i+1] = accidental;
    answer[i+1] = '\0';
    return answer;
}

char * limited_tpc_string(TPC t, int *sharps) {
    /* returns the number of sharps (negative if there are flats). Use "A-"
       to indicate Abb and it uses Bx to indicate B##.  With more than 2
       sharps or flats, it uses "B?". */
    
    static char answer[20];
    int i;
    int let;
    
    let = (t-1) % 7;
    if (let < 0) let += 7;
    *sharps = (t-1-let)/7;

    for (i=0; i<sizeof(answer); i++) answer[i] = '\0';
    
    answer[0] = letters[let];
    switch(*sharps) {
    case  1:    answer[1] = '#';   break;
    case  2:    answer[1] = 'x';   break;
    case -1:    answer[1] = 'b';   break;
    case -2:    answer[1] = '-';   break;
    case  0:    break;
    default:    answer[1] = '?';   break;
    }
    return answer;
}

int count_in_side_effect_tpc_choice(int j, int nnotes) {
    int k, count = 0;
    for (k=0; k<nnotes; k++) count += (j == side_effect.tpc_choice[k]);
    return count;
}

int list_has_starting_note_with_this_tpc(Chord *ch, TPC j) {
    int k;
    Note * note;
    for (k=0, note = ch->note; note!=NULL; note = note->next, k++) {
	if (ch->is_first_chord && note->is_first_note && side_effect.tpc_choice[k] == j) return TRUE;
    }
    return FALSE;
}

int jth_note_is_starting_note(Chord *ch, int j) {
    int k;
    Note * note;
    for (k=0, note = ch->note; note!=NULL; note = note->next, k++) {
	if (ch->is_first_chord && note->is_first_note && k == j) return TRUE;
    }
    return FALSE;
}

int is_beat_note(Note *note) {
    /* return TRUE if this note occurs on a beat */
    if (!note->is_first_note) return FALSE;
    return ((rlookup_beat(note->directnote->start, 0, N_beats-1)) != -1);
}

int comp_notes(void *p, void *q) {
    return (*((int *)p) - *((int *)q));
}

int find_footnote(int note, int * footnote, int N_footnotes) {
    int i;
    for (i=0; i<N_footnotes; i++) if (note == footnote[i]) return i;
    return -1;
}

void ASCII_display(Chord *long_ch) {
    Chord *chord;
    Note *note;
    int i, j, jj, nprinted, nskipped, k, min_root = 0, max_root = 0, h, max_notes = 0;
    int min_pitch = 0, max_pitch = 0;
    int max_tpc=0, min_tpc=0, sharps;
    int parts, xparts, nnotes;
    Bucket ** bucket_choice;
    Bucket * best_b, *bb, *bu;
    char * harm_rep = "Harmonic Rep";
    char * tpc_rep = "TPC Rep";
    char * anal = "Analysis:";
    char str[10];
    int footnote[99], N_footnotes = 0, fn;
    
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
    
    /*
      Analysis        Harmonic rep   TPC rep
      0  x x x x  G      |       G    | 
      */
    
    
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
	
	if (i==0 || bb->root < min_root) min_root = bb->root; 
	if (i==0 || bb->root > max_root) max_root = bb->root;
	for (j=0, note = chord->note; note != NULL; note = note->next, j++) {
	    if ((j==0 && i==0) || side_effect.tpc_choice[j] < min_tpc) min_tpc = side_effect.tpc_choice[j];
	    if ((j==0 && i==0) || side_effect.tpc_choice[j] > max_tpc) max_tpc = side_effect.tpc_choice[j];
	    if ((j==0 && i==0) || note->pitch < min_pitch) min_pitch = note->pitch;
	    if ((j==0 && i==0) || note->pitch > max_pitch) max_pitch = note->pitch;
	}
	if (j > max_notes) max_notes = j;
    }
    
    printf("\n");
    
    printf("%s", anal);
    for (i=0; i<(N_beatlevel*2 + 8 - strlen(anal)); i++) printf(" ");
    
    printf("%s", harm_rep);
    for (i=0; i<(4+7+3*(max_root-min_root+1) - strlen(harm_rep)); i++) printf(" ");
    
    printf("%s", tpc_rep);
    for (i=0; i<(4*(max_notes) + 3*(max_tpc-min_tpc+1) - strlen(tpc_rep)); i++) printf(" ");
    
    printf("\n\n");
    
    
    
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
	
	
#if 0
	OBS    parts = (2*chord->duration + baseunit) / (2 * baseunit);  /* rounded version of duration/baseunit */
#endif
	
	/* Above is the old way to see how many parts there are.
	   in an explicit beat list we need to use the beat list
	   to see how many parts there are */
	
	parts = lookup_beat(chord->start + chord->duration) - lookup_beat(chord->start);
	
	for (nnotes=0, note=chord->note; note != NULL; note = note->next) nnotes++;
	/*
	  qsort(side_effect.tpc_choice, nnotes, sizeof(side_effect.tpc_choice[0]),
	  (int(*)(const void *, const void *))comp_notes);
	  */
	for (xparts = 0; xparts < parts; xparts++, long_ch = long_ch->next) {
	    /*      printf("%5d ", chord->start + (2 * xparts * chord->duration + 1)/(2*parts)); */
	    printf("%6d  ", long_ch->start);
	    for (j=0; j<N_beatlevel; j++) {
		if (long_ch->level >= j) printf("x "); else printf("  ");
	    }
	    
#if 0
	    printf(" %-4s  ", tpc_string(bb->root));
	    
	    for (j=min_root; j <= max_root; j++) {
		if (j == bb->root) printf(" | "); else printf("   "); 
	    } 
	    
#else
	    
	    for (j=min_root; j <= max_root; j++) {
		if (j == bb->root) {
		    /*
		      if (i>0 && bb->root == bucket_choice[i-1]->root) {
		      printf(" | ");
		      } else {
		      printf("%-3s", tpc_string(bb->root)); 
		      }
		      */
		    printf("%-3s", tpc_string(bb->root)); 
		} else {
		    printf("   ");
		}
	    }
	    
#endif
	    
	    printf("     <");
	    nprinted = 0;
	    nskipped = 0;
	    for (jj=min_pitch; jj <= max_pitch; jj++) {
		Note * jnote;
		for (jnote = chord->note, j=0; j<nnotes; j++, jnote = jnote->next) {
		    if (jnote->pitch != jj) continue;
		    sprintf(str, "%s%s", (is_beat_note(jnote))?" ":"*", limited_tpc_string(side_effect.tpc_choice[j], &sharps));
		    if (sharps < -2 || sharps > 2) {
			fn = find_footnote(side_effect.tpc_choice[j], footnote, N_footnotes);
			if (fn == -1) {
			    if (N_footnotes == (sizeof(footnote)/sizeof(footnote[0]))) {
				fprintf(stderr, "%s: Can't display -- too many footnotes.\n", this_program);
				my_exit(1);
			    }
			    footnote[N_footnotes] = side_effect.tpc_choice[j];
			    fn = N_footnotes;
			    N_footnotes++;
			}
			sprintf(str, "%d", fn+1);
		    }
		    if (xparts == 0 && jth_note_is_starting_note(chord, j)) {
			if (nprinted > 0) printf(" ");  /* here's where is could print a , */
			printf("%-3.3s", str);
			nprinted++;
		    } else {
			nskipped++;
		    }
		}
	    }
	    for (j=0; j<nskipped; j++) {
		if (nprinted > 0) printf(" ");
		printf("%-3.3s", "");
		nprinted++;
	    }
	    for (j = nnotes; j<max_notes; j++) {
		if (nprinted > 0) printf(" ");
		printf("%-3.3s", "");
		nprinted++;
	    }
	    
	    printf(">   ");
	    
	    for (j=min_tpc; j <= max_tpc; j++) {
		if (xparts == 0 && list_has_starting_note_with_this_tpc(chord, j)) {
		    printf(" + ");
		} else {
		    k = count_in_side_effect_tpc_choice(j, nnotes);
		    if (k==0) {
			printf("   ");
		    } else if (k==1) {
			printf(" | ");
		    } else {
			printf(" %1d ", k);
		    }
		}
	    }
	    printf("\n");
	}
    }
    
    if (N_footnotes > 0) {
	printf("\n");
	printf("Notes:\n");
	for (i=0; i<N_footnotes; i++) {
	    printf("    %2d -- %s\n", i+1, tpc_string(footnote[i]));
	}
    }
    xfree(bucket_choice);
}

void print_direct_notes(void) {
    DirectNote *dn;
    int ignored_count = 0;
    for (dn = global_DN_list; dn != NULL; dn = dn->next) {
	if (dn->tpc != UNINITIALIZED_TPC) {
	    printf("TPCNote %6d %6d %3d %3d\n", dn->start, dn->start + dn->duration, dn->pitch, dn->tpc);
	} else {
	    ignored_count++;
	}
    }
    if (ignored_count > 0) {
	fprintf(stderr, "%s: warning: %d note%s ignored cause of overlap.\n", this_program, ignored_count, (ignored_count != 1)?"s":"");
    }
}
