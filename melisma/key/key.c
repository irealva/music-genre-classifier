
 /***************************************************************************/
 /*                                                                         */
 /*       Copyright (C) 2000 Daniel Sleator and David Temperley             */
 /*           See http://www.link.cs.cmu.edu/music-analysis                 */
 /*        for information about commercial use of this system              */
 /*                                                                         */
 /***************************************************************************/ 

/* _Important comment about how pitches and keys are represented_

  Notes may be inputted in either Note format (with just a pitch) or
  TPCNote format (with a TPC). The user may also specify
  "npc_or_tpc_profile": 0 for npc, 1 for tpc. (A TPC profile is like
  an NPC profile except that a user-settable default value is used for
  TPC's outside the range of b2 to #4.) We assume that Note format
  will be used with an npc profile; if not, a fatal error is
  reported. TPCNote format could be used with either; if you use
  TPCNote input with an npc profile, the notes are simply mapped on to
  one cycle of the LOF.  (Scoring mode 0 or 3 requires an npc profile;
  otherwise a fatal error is reported.)

  Pitches (e.g. note[].tpc values) are always represented in
  line-of-fifths order, with C = 14. (This is done when the pitches
  are first read in). (If there's Note input, or TPCNote input and
  npc_or_tpc_profile = 0, all pitches are shifted into one cycle of
  the line of fifths, from 9 to 20 inclusive; again, this is done when
  the input is read.)

  Keys are also represented in line-of-fifths order. Major keys are 0
  to 27, C=14; minor keys are 28-55, C minor = 42. If
  npc_or_tpc_profile = 0, the search is nominally done on all keys,
  but only keys from 9 to 20 (inclusive) and 37 to 48 (inclusive) are
  looked at; others are given large negative values.

  Although key-profiles are read in in pitch-height order, they are
  then adjusted (in generate_tpc/npc_profiles) to line-of-fifths
  order, with the tonic as 5. */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "key.h"

print_keyname(int f) {
      printf("%c", letter[f % 7]);
      if (f<6 || (f>=28 && f<34)) printf("-");
      if ((f>=6 && f<13) || (f>=34 && f<41)) printf("b");
      if ((f>=20 && f<27) || (f>=48 && f<55)) printf("#");
      if (f==27 || f == 55) printf("x");
      if (f>=28) printf("m");
      if (f<28) printf(" ");
      printf(" "); 
}

create_chords() {                                   /* Now we create the chords; each chord starts and ends at a beat of 
						      beat_printout_level or higher, swallowing up ichords starting at lower 
						      levels */
  int c, b=0;
  for (c=numchords-1; c>0; c--) {
    if (ichord[c].beatlevel<beat_printout_level) {
      ichord[c-1].offtime=ichord[c].offtime;
      ichord[c-1].duration=ichord[c].offtime-ichord[c].ontime;
    }
  }
  for (c=0; c<numchords; c++) {
    if (ichord[c].beatlevel>=beat_printout_level) {
      chord[b].ontime=ichord[c].ontime;
      chord[b].offtime=ichord[c].offtime;
      chord[b].duration=ichord[c].duration;
      chord[b].root=ichord[c].root;
      chord[b].beatlevel=ichord[c].beatlevel;
      b++;
    }
  }
  numchords=b;                                      /* numchords now becomes the number of big chords */
}

create_segments() {                                 /* Each segment starts at a sbeat and ends at the following sbeat */
    int b, s;
    segment[0].start=firstbeat;                                      /* Always start a segment at the very beginning of the piece (the first beat) */
    s=1;
    for (b=0; b<num_sbeats; b++) {
	if (b==0 && (sbeat[0].time-firstbeat) <((sbeat[1].time-firstbeat) - (sbeat[0].time-firstbeat))/2) continue; 
	/* If it's the first beat of the piece, and the upbeat is
	   less than half of the first beat interval, don't start a segment */
	else {
	    segment[s].start=sbeat[b].time;
	    segment[s-1].end=sbeat[b].time;
	    /* printf("A segment starts at %d and ends at %d\n", segment[s-1].start, segment[s-1].end);  */
	    s++;
	}
    }
    s--;                                        /* Now s = array number of final segment */
    /* If final segment starts at or after final timepoint of piece, ignore it,
       decrementing number of segments by 1; if not, set that segment's ending
       to final timepoint of piece */
    if (segment[s].start>=final_timepoint) {    
	s--;
    }
    else {
	segment[s].end=final_timepoint;
	/* printf("Final segment ends at %d", segment[s].end); */
    }
    segtotal=s; /* array number of final segment  */
    
    if(verbosity>=2) {
	for(s=0; s<=segtotal; s++) {
	    printf("Segment: %d %d\n", segment[s].start, segment[s].end);
	}
    }
}

fill_segments() {
  int s, n, x;
  for (s=0; s<=segtotal; ++s) {                
    for (x=0, n=0; x<numnotes; ++x) {
      if (note[x].ontime >= segment[s].start && note[x].ontime < segment[s].end
	  && note[x].offtime <= segment[s].end) {  /* note begins and ends in segment */
	segment[s].snote[n].pitch=note[x].pitch;                                             
	segment[s].snote[n].tpc=note[x].tpc;
	segment[s].snote[n].duration=note[x].duration;
	++n;
      }
      if (note[x].ontime >= segment[s].start && note[x].ontime < segment[s].end && note[x].offtime > segment[s].end) {  
                                            /* note begins, doesn't end in segment*/
	segment[s].snote[n].pitch=note[x].pitch;                             
	segment[s].snote[n].tpc=note[x].tpc;
	segment[s].snote[n].duration = segment[s].end - note[x].ontime;
	++n;
      }
      if (note[x].ontime < segment[s].start && note[x].offtime > segment[s].start && note[x].offtime <= segment[s].end) {    
             	                                   /* note ends, doesn't begin in segment */
	segment[s].snote[n].pitch=note[x].pitch;                                             
	segment[s].snote[n].tpc=note[x].tpc;
	segment[s].snote[n].duration = note[x].offtime - segment[s].start;
	++n;
      }
      if (note[x].ontime < segment[s].start && note[x].offtime > segment[s].end) {    
	                                          /* note doesn't begin or end in segment */
	segment[s].snote[n].pitch=note[x].pitch;                                               
	segment[s].snote[n].tpc=note[x].tpc;
	segment[s].snote[n].duration = segment[s].end - segment[s].start;
	++n;
      } 
    }
    segment[s].numnotes = n;          /* number of notes in the segment */
    /* printf("Number of notes in segment %d: %d\n", s, n); */
  }
}

count_segment_notes() {                             /* In each segment, tally up the notes of each TPC */
    int s, y, n;
    double total_dur;

    for (s=0; s<=segtotal; ++s) {                        
	pc_tally[s]=0;
	for (y=0; y<28; ++y) {                        /* cycle through the pc's, make sure all the seg_prof values are zero */
	    seg_prof[s][y] = 0;
	}

	total_dur = 0;

	for (n=0; n<segment[s].numnotes; ++n) {                 

	    if(scoring_mode == 0) total_dur += segment[s].snote[n].duration;
	    for (y=0; y<28; ++y) {                        
		
		if (segment[s].snote[n].tpc == y) { 
		    if(seg_prof[s][y]==0) pc_tally[s]++;
		    /* This keeps track of how many different pc's the segment contains. This counts TPCs, not NPCs! */
		    /* If scoring_mode is > 1, set array value to 1. If 0, add the note's duration to the
		       array value (as in the K-S algorithm) */
		    if (scoring_mode > 0) seg_prof[s][y] = 1; 
		    else {
			seg_prof[s][y] += segment[s].snote[n].duration;  
		    }
		}
	    }
	}	

	if(scoring_mode == 0) {
	    if(pc_tally[s]==0) segment[s].average_dur = 0.0; 
	    segment[s].average_dur = total_dur / 12.0;
	    /* printf("Segment %d total dur = %6.3f, average dur = %6.3f\n", s, total_dur, segment[s].average_dur); */
	}			

	if(verbosity>=2) {
	    printf("Segment %d: ", s);
	    for (y=0; y<28; ++y) {                          
		if(npc_or_tpc_profile == 0 && (y<9 || y>20)) continue;
		printf("%d ", seg_prof[s][y]);  
	    }
	    printf("\n"); 
	}
	/* printf("pc_tally = %d\n", pc_tally[s]); */
    }

}

prepare_profiles() {

  /* We're only here if scoring_mode is 0 (the K-S algorithm). Sum all the profile values, take the mean, 
     and subtract that from each value */
    
    double total;
    double average;
    int i;

    total = 0;
    for(i=0; i<12; i++) {
	total += major_profile[i];
    }
    average = total / 12.0;
    for(i=0; i<12; i++) major_profile[i]=major_profile[i]-average;
    
    total = 0;
    for(i=0; i<12; i++) {
	total += minor_profile[i];
    }
    average = total / 12.0;
    for(i=0; i<12; i++) minor_profile[i]=minor_profile[i]-average;
    
    if(verbosity>2) {
	printf("Adjusted major profile: ");
	for(i=0; i<12; i++) printf("%6.3f ", major_profile[i]);
	printf("\n");
	printf("Adjusted minor profile: ");
	for(i=0; i<12; i++) printf("%6.3f ", minor_profile[i]);
	printf("\n");
    }
}

/* Here we generate the key profiles. (This is assuming tpc input.) Key_profile[key] numbers correspond to 
   the line of fifths, with C = 14.  Major keys are 0-27, minor keys are 28-55. PCs are also numbered
   according to the line of fifths. The major_step_profile has the tonic in step 5. For a given key, the 
   profile value for a given tpc is equal to the line of fifths difference between the tpc and the key, plus
   5. */

generate_tpc_profiles() {
  int key, shift, tpc, i;
  float majp[12];
  float minp[12];

  /* First we rearrange the key profile values (inputted in pitch height order) into lof order, C = 5 */
  for(i=0; i<12; i++) {
    majp[((((i * 7) % 12) + 5) % 12)] = major_profile[i];
    minp[((((i * 7) % 12) + 5) % 12)] = minor_profile[i];
  }
  
  for (key=0, shift=0; key<28; ++key, ++shift) {              
    for (tpc=0; tpc<28; ++tpc) {                                 
      if (tpc-shift >= -5 && tpc-shift <= 6) {
	key_profile[key][tpc] = majp[5 + (tpc-shift)];    /* For example: for key 14 (C major) and tpc 17 (A), 
								        use profile step 5 + (17-14) = 8 */
      }
      if (tpc-shift < -5 || tpc-shift > 6) {
	key_profile[key][tpc] = default_profile_value;
      }
    }
  }
  for (key=28, shift=0; key<56; ++key, ++shift) {             
    for (tpc=0; tpc<28; ++tpc) {                                 
      if (tpc-shift >= -5 && tpc-shift <= 6 ) {                    
	key_profile[key][tpc] = minp[5 + (tpc-shift)];
      }

      if (tpc-shift < -5 || tpc-shift > 6) {
	key_profile[key][tpc] = default_profile_value;
      }
    }
  }

  /*  for(key=0; key<56; ++key) {     This routine just prints out the key profiles  
    for(tpc=0; tpc<28; ++tpc) {

      printf("%1.2f ", key_profile[key][tpc]);
    }
    printf("\n");
  } */
}

/* This is an alternative function for generating profiles given NPC
   profile. It's similar to the TPC version, except that only keys on
   a certain range of the line are assigned non-zero values. (This is
   really an unnecessary step, as steps outside the range will be
   disqualified in "match_profiles" in any case.) Also, for those keys
   considered, only key profile slots within the 9-to-20 range are
   assigned non-zero values. (This is necessary, since the input profile
   is within this range as well.) We begin by assigning default values
   of ZERO to everything. */

generate_npc_profiles() {

  int key, shift, tpc, tpc_to_use, i;
  float majp[12];
  float minp[12];

  /* First we rearrange the key profile values (inputted in pitch height order) into lof order, C = 5 */
  for(i=0; i<12; i++) {
    majp[((((i * 7) % 12) + 5) % 12)] = major_profile[i];
    minp[((((i * 7) % 12) + 5) % 12)] = minor_profile[i];
  }
  for (key=0; key<56; ++key) {
    for(tpc=0; tpc<28; ++tpc) {
      key_profile[key][tpc]=0;
    }
  }

  for (key=9, shift=9; key<21; ++key, ++shift) {              
    for (tpc=0; tpc<28; ++tpc) {                                 
	/* tpc_to_use is the profile slot to use for a given value of the key-profile. In this way we keep
	   all profile slots within the 9-to-20 range */
	if(tpc<9) tpc_to_use=tpc+12;
      else if(tpc>20) tpc_to_use=tpc-12;	
      else tpc_to_use = tpc;
      if (tpc-shift >= -5 && tpc-shift <= 6) {

	key_profile[key][tpc_to_use] = majp[5 + (tpc-shift)];    /* For example: for key 14 (C major) and tpc 17 (A), 
								    read from profile step 5 + (17-14) = 8. For degree 
								    6 of B major (key 19), degree 6 (22) is outside the
								    9-to-20 range, so tpc_to_use is 22-12=10; still 
								    read from profile step 5 + (22-19) = 8. */
      }
    }
  }
  for (key=37, shift=9; key<49; ++key, ++shift) {             
    for (tpc=0; tpc<28; ++tpc) {                                 
      if(tpc<9) tpc_to_use=tpc+12;
      else if(tpc>20) tpc_to_use=tpc-12;	
      else tpc_to_use = tpc;
      if (tpc-shift >= -5 && tpc-shift <= 6 ) {                    
	key_profile[key][tpc_to_use] = minp[5 + (tpc-shift)];
      }
    }
  }

  /*
  for(key=0; key<56; ++key) {   
    for(tpc=0; tpc<28; ++tpc) {

      printf("%1.2f ", key_profile[key][tpc]);
    }
    printf("\n");
  } */

}

match_profiles() {

    /* Here we generate the "key scores" - the local score for each key */

    /* Notice that we generate profiles for all 56 keys and all 28
       tpc's, even in the case where npc profiles are being used. In
       this case, though, all keys outside the allowable NPC range are
       given large negative values.  And within both the key profiles
       and the segment profiles, only TPC's within the range 9 to 20
       have been given nonzero values. */

  int key, tpc, s, best_key, i;
  double major_sumsq, minor_sumsq, input_sumsq;
  double kprob[56], best_score;

  for (s=0; s<=segtotal; ++s) {                        
      for(key=0; key<56; ++key) {
	  key_score[s][key]= 0.0;
      }
  }

  if(scoring_mode==0) {
      major_sumsq = 0.0;
      minor_sumsq = 0.0;
      for(i=0; i<12; i++) major_sumsq += major_profile[i]*major_profile[i];
      for(i=0; i<12; i++) minor_sumsq += minor_profile[i]*minor_profile[i];
      if(verbosity==3) printf("major_sumsq = %6.3f, minor_sumsq = %6.3f\n", major_sumsq, minor_sumsq);
  }

  for (s=0; s<=segtotal; ++s) {                        

      if(scoring_mode==0) {
	  input_sumsq=0.0;
	  for(i=9; i<=20; i++) {
	      input_sumsq += pow((seg_prof[s][i]-segment[s].average_dur), 2.0);		
	      /* printf("%d X %6.3f squared is %6.3f\n", seg_prof[s][i], segment[s].average_dur, pow((seg_prof[s][i]-segment[s].average_dur), 2.0)); */
	  }
	  if(verbosity==3) printf("For segment %d: average_dur = %6.3f; input_sumsq = %6.3f\n", s, segment[s].average_dur, input_sumsq); 
      }

      best_key=0;

      for (key=0; key<56; ++key) {
	kprob[key] = 0.0;
	key_score[s][key]=-1000000.0;      
	if(npc_or_tpc_profile==0 && (key<9 || (key>20 && key<37) || key>48)) continue;
	kprob[key] = 1.0;
	key_score[s][key] = 0.0;
	for (tpc=0; tpc<28; ++tpc) {

	    /* 
	       If scoring mode is 0, this is the K-S algorithm (this works for npc mode only). Segment
	       profile values represent total duration of each pc (in all other cases, they're just 1
	       for present pc's and 0 for absent ones). Key-profiles have been normalized linearly 
	       around the average key-profile value. We normalize the input values similarly by taking
	       (seg_prof[s][tpc]-segment[s].average_dur). Then we multiply each normalized KP value by
	       the normalized input value, and sum these products; this gives us the numerator of the 
	       correlation expression (as commented below). We've summed the squares of the normalized 
	       key-profile value (major_sumsq and minor_sumsq above) and the normalized input values 
	       (input_sumsq above), so this allows us to calculate the denominator also.

	       If scoring_mode is 1, the key score is the sum of key-profile values for all pc's present 
	       (this is the algorithm used in CBMS) 

	       If scoring_mode is 2, calculate key scores as above, but divide each one by the number 
	       of pc's in the segment

	       If scoring_mode is 3: for each key, add the log of the key-profile value for all present pc's;
	       subtract values for all absent pc's. (This is the Bayesian approach; assume key-profiles 
	       represent pc distribution's in a corpus, i.e. the number of segments containing each scale 
	       degree)
	    */

	    if(scoring_mode == 0) {
		if(tpc<9 || tpc>20) continue; 
		/* calculate numerator */
		key_score[s][key] += key_profile[key][tpc] * (seg_prof[s][tpc]-segment[s].average_dur);		
		/* printf("x-X=%6.3f, y-Y=%6.3f, product=%6.3f, new total=%6.3f\n", key_profile[key][tpc], seg_prof[s][tpc]-segment[s].average_dur, key_profile[key][tpc] * (seg_prof[s][tpc]-segment[s].average_dur), key_score[s][key]); */

	    }

	    if(scoring_mode==1 || scoring_mode==2) key_score[s][key] += (key_profile[key][tpc] * seg_prof[s][tpc]);
	    
	    if(scoring_mode == 3) {
		/* if(tpc>11) continue; */
		/* if(tpc<9 || tpc>20) continue; */

		if(seg_prof[s][tpc]==0) {
		    key_score[s][key] += log(1.000 - key_profile[key][tpc]);
		    /* printf("kp value = %6.3f: log(1-p) = %6.3f: score = %6.3f\n", key_profile[key][tpc], log(1.000 - key_profile[key][tpc]), key_score[s][key]); */
		    if(tpc>=9 && tpc<=20) kprob[key] *= (1.000 - key_profile[key][tpc]);
		}
		else {
		    key_score[s][key] += log(key_profile[key][tpc]);
		    if(tpc>=9 && tpc<=20) kprob[key] *= key_profile[key][tpc];
		}

		/* printf("kp value = %6.3f: log(p) = %6.3f: score = %6.3f\n", key_profile[key][tpc], log(key_profile[key][tpc]), key_score[s][key]); */
	    }	    
	}

	if(scoring_mode == 0) {
	    /* printf("sqrt(major_sumsq * input_sumsq) = %6.3f\n", sqrt(major_sumsq * input_sumsq)); */
	    /* calculate denominator */
	    if(key<28) key_score[s][key] = key_score[s][key] / sqrt(major_sumsq * input_sumsq);
	    else key_score[s][key] = key_score[s][key] / sqrt(minor_sumsq * input_sumsq);
	}

	if(scoring_mode == 2) {
	    if(pc_tally[s] == 0) key_score[s][key] = 0;
	    else key_score[s][key] = key_score[s][key] / pc_tally[s];
	}
	
	/* if(s==0) printf("local score for key %d on segment %d: %6.3f\n", key, s, key_score[s][key]); */
	if (key_score[s][key] > key_score[s][best_key]) best_key = key;
    }

    if(verbosity>=2) {
	printf("The best local key for segment %d at time %d is ", s, segment[s].start);
	print_keyname(best_key);
	printf("with score %6.3f\n", key_score[s][best_key]);
    }

    if(scoring_mode==3) {
	total_prob[s]=0.0;
	for(key=0; key<56; key++) {
	    total_prob[s] += kprob[key] / 24.0;
	    /* printf("  Prob of segment %d given key %d: %6.8f\n", s, key, kprob[key]);  */
	}

	/* Now total_prob[s] is the total probability of the segment occurring: its probability given
	   a key, summed over all keys. But suppose we want to know the probability of ANY major triad
	   occurring? Then we have to multiply this by 12. (But not for something like a diminished
	   seventh which is symmetrical! */

	if(verbosity>=3) {
	    printf("Best key for segment %d = %d, score = %6.8f\n", s, best_key, kprob[best_key]);
	    printf("Total (local) probability of segment %d: %6.8f\n", s, total_prob[s]); 
	}
    }
  }
}

make_first_table() {
  int i, j, s;

  double seg_factor, mod_factor, nomod_factor;

  if(scoring_mode==3) {
      mod_factor = log(change_penalty);
      nomod_factor = log(1.0 - change_penalty);
      seg_factor = 1.0;
  }

  else {
      mod_factor = -change_penalty;
      nomod_factor = 0.0;
      seg_factor = seglength;
  }

  for(s=0; s<=segtotal; s++) {
      for(i=0; i<56; ++i) {
	  for(j=0; j<56; ++j) {
	      analysis[s][i][j]=-1000.0;
	  }
      }
  }

  for(i=0; i<56; ++i) {
    for(j=0; j<56; ++j) {
      if (j != i) analysis[1][i][j] = ((key_score[0][i] + key_score[1][j]) * seg_factor) + mod_factor;
      else analysis[1][i][j] = ((key_score[0][i] + key_score[1][j]) * seg_factor) + nomod_factor;
    }
  }
  choose_best_i();
}

choose_best_i() {
  int i, j, k;
  for(j=0; j<56; ++j) {
    for(i=0, k=0; i<56; ++i) {
      if (analysis[seg][i][j] > analysis[seg][k][j]) k=i;
    }
    best[seg][j]=k;                  /* For a given segment seg, and the key j at that segment, the best previous key is k */ 
    /* printf("For segment-%d-key %d, best segment-%d-key is %d, with score %d\n", seg, j, seg-1, k, analysis[seg][k][j]); */
  }
}

make_tables() {

  double seg_factor, mod_factor, nomod_factor;

  /* When scoring_mode = 3, the change_penalty represents the probability of changing key. So raising
     the penalty actually _increases_ the likelihood of modulations. */

  if(scoring_mode==3) {
      mod_factor = log(change_penalty / 23.0);
      nomod_factor = log(1.0 - change_penalty);
      seg_factor = 1.0;
  }

  else {
      mod_factor = -change_penalty;
      nomod_factor = 0.0;
      seg_factor = seglength;
  }

  for (seg=2; seg<=segtotal; ++seg) {                       
    int i, j, n;
    /* printf("mod_factor = %6.6f; ; nomod_factor = %6.6f\n", mod_factor, nomod_factor);  */
    for(j=0; j<56; ++j) {
      for(i=0; i<56; ++i) {
	n=best[seg-1][i];
	if (j != i) analysis[seg][i][j] = analysis[seg-1][n][i] + (key_score[seg][j] * seg_factor) + mod_factor;
	else analysis[seg][i][j] = analysis[seg-1][n][i] + (key_score[seg][j] * seg_factor) + nomod_factor;
      }      
    }
    choose_best_i();
  }
}

best_key_analysis() {
  int s, j, k, n, m, f, lof, tie1=-1, tie2=-1;
  s=segtotal;

  for(j=0, k=0; j<56; ++j) {
    n=best[s][j];
    m=best[s][k];

    if (analysis[s][n][j] < analysis[s][m][k] + .001 && analysis[s][n][j] > analysis[s][m][k] - .001 && j!=k) {
      tie1=j;
      tie2=k;
    }

    if(verbosity>1 && !(npc_or_tpc_profile == 0 && (j<9 || (j>20 && j<37) || j>48))) {
      printf("Final score for ");
      print_keyname(j);
      /* printf("is %6.3f\n", analysis[s][n][j] * 1000 / (segment[segtotal].end - segment[0].start));   */
      printf("is %6.3f\n", analysis[s][n][j]);
    }
    if (analysis[s][n][j] > analysis[s][m][k] + .000001) { 
      /* The .000001 is to fix a strange bug: sometimes it thinks the conditional is satisfied in the case of ties */
      k=j;   /* compute best key of final segment */
    }
  }

  /* To force a key choice at the final segment, insert key number here */

  final[s] = k; 
  if(verbosity>1) if(k==tie1 || k==tie2) printf("Tie at the end between %d and %d\n", tie1, tie2); 

  /* Here's where we take the best key choices and put them into final[s] */

  for(s=segtotal; s>=1; --s) {
    final[s-1] = best[s][k];
    k=final[s-1];
  }

  if(verbosity>=2) {
      printf("Segment 0: key choice %d; total score %6.3f; segment score %6.3f\n", final[0], key_score[0][final[0]],
key_score[0][final[0]]);
      printf("Segment 1: key choice %d; total score %6.3f; segment score %6.3f\n", final[1], analysis[1][final[0]][final[1]], analysis[1][final[0]][final[1]] - key_score[0][final[0]]);
      for(s=2; s<=segtotal; s++) {
	  printf("Segment %d: key choice %d; total score %6.3f; segment score %6.3f\n", s, final[s], analysis[s][final[s-1]][final[s]], analysis[s][final[s-1]][final[s]] - analysis[s-1][final[s-2]][final[s-1]]);
      }
  }

  if(verbosity>1) printf("'Key-fit' scores for preferred analysis:\n");
  for(s=0; s<=segtotal; ++s) {                        /* This routine calculates the key-fit scores for the final analysis. These are really per-second scores. Key-profile scores are not multiplied by
  seglength (as they would be in actually computing the analyses); change penalties are divided by
  seglength. */
    f=final[s];
    if(s>0 && final[s]!=final[s-1]) final_score[s]=(key_score[s][f]) - (change_penalty / seglength);  
    else final_score[s]=key_score[s][f];
    if(verbosity>1) {
      printf(" segment %d: %6.3f\n", s, final_score[s]);
    }
  }

  if(verbosity>=1) {
    for (s=0; s<=segtotal; ++s) {                        /* This routine prints out the chosen key of each segment */
      f=final[s];
      /* printf("Key of segment %d starting at time %d is %d: ", s, segment[s].start, f); */
      
      /* if ((s+1) % 10 == 0) printf("%d.", s+1); */
      if (s % 10 == 0) printf("\n"); 
      print_keyname(f);
    } 
    printf("\n");
  }
}

