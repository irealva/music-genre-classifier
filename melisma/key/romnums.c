
 /***************************************************************************/
 /*                                                                         */
 /*       Copyright (C) 2000 Daniel Sleator and David Temperley             */
 /*           See http://www.link.cs.cmu.edu/music-analysis                 */
 /*        for information about commercial use of this system              */
 /*                                                                         */
 /***************************************************************************/ 

#include <stdio.h>
#include <string.h>
#include "key.h"

generate_chord_info() {
  int c, k, s, n, lowest_pitch, bass_tpc;
  for (c=0; c<numchords; c++) {
    for (s=0; s<=segtotal; s++) {
      if (segment[s].start <= chord[c].ontime && segment[s].end > chord[c].ontime) {
	chord[c].key=final[s];
	if (final[s]<28) {                                 /* It's a major key */
	  chord[c].function=chord[c].root-final[s];
	}
	if (final[s]>=28) {                                             /* It's a minor key */
	  chord[c].function=chord[c].root-(final[s]-28);
	}
      }
    }
/* printf("In a segment with key %d, a chord of root %d has a function of %d\n", final[s], chord[c].root, chord[c].function); */
    chord[c].mode=0;                      
    chord[c].ext=0;                      
    chord[c].fifth=0;
    chord[c].inversion=0;
    lowest_pitch=100;
    for (n=0; n<numnotes; n++) {                           /* label chords with mode, seventh, and fifth */
      if (note[n].ontime < chord[c].offtime && note[n].offtime > chord[c].ontime) {
	if (note[n].tpc-chord[c].root==4) chord[c].mode=1;
	if (note[n].tpc-chord[c].root==-3) chord[c].mode=2;
	if (note[n].tpc-chord[c].root==-2) chord[c].ext=1;
	if (note[n].tpc-chord[c].root==-6) chord[c].fifth=2;
	if (note[n].tpc-chord[c].root==1) chord[c].fifth=1;
	if (note[n].pitch<lowest_pitch) {
	  lowest_pitch=note[n].pitch;
	  bass_tpc=note[n].tpc;
	}
      }
    }
    if (lowest_pitch!=100) {
      if (bass_tpc-chord[c].root==0) chord[c].inversion=0;    /* not really necessary */
      if (bass_tpc-chord[c].root==4) chord[c].inversion=1;
      if (bass_tpc-chord[c].root==-3) chord[c].inversion=1;
      if (bass_tpc-chord[c].root==1) chord[c].inversion=2;
      if (bass_tpc-chord[c].root==-2) chord[c].inversion=3;
    }
    /* printf("Chord %d with root %d has inversion %d\n", c, chord[c].root, chord[c].inversion);  */
  }
}

merge_functions() {                     /* This goes through and modifies the chord info. If a chord has the same root as
					   the previous chord (and it's not a new key), and its mode, ext, or fifth are
					   unspecified, they are set to the same value as the previous chord. If it is
					   a new chord and it's mode or fifth are unspecified, these are set. */

  int b, s, first_chord=1, c, prev_root=50, prev_mode=10, prev_ext=10, prev_fifth=10;
  for (b=0; b<numbeats; b++) {    
    if (beat[b].level<beat_printout_level) continue;

    for (c=0; c<numchords; c++) {
      if (chord[c].ontime==beat[b].time) {
	if (chord[c].root==prev_root && chord[c].key==chord[c-1].key && first_chord==0) {            
	  if (chord[c].mode==0) chord[c].mode=prev_mode;        
	  if (chord[c].ext==0) chord[c].ext=prev_ext;
	  if (chord[c].fifth==0) chord[c].fifth=prev_fifth;
	}
	if (chord[c].root!=prev_root || chord[c].key!=chord[c-1].key || first_chord==1) {
	  if (chord[c].mode==0) {
	    if ((chord[c].key<28 && chord[c].function>1) || (chord[c].key>=28 && (chord[c].function>-2 && chord[c].function!=1))) chord[c].mode=2;
	    
	    /* If it's a major key and the root is more than one step above the key, or it's a minor key and the root is
	      more than -2 steps above the key (and it's not V), then the default mode for the chord should be minor */

	    else chord[c].mode=1;
	  }
	  if (chord[c].fifth==0) chord[c].fifth=1;
	}
	prev_root=chord[c].root;
	prev_mode=chord[c].mode;
	prev_ext=chord[c].ext;
	prev_fifth=chord[c].fifth;
	first_chord=0;
	/* printf("A chord with root %d has mode %d and extension %d and function %d and key %d\n", chord[c].root, chord[c].mode, chord[c].ext, chord[c].function, chord[c].key); */
      }
    }
  }
}

chords_to_romnums_kp(){
  int c;
  for(c=0; c<numchords; c++) {
    chord[c].romnum=8;
    if ((chord[c].key<28 && chord[c].function>=-1 && chord[c].function<=5) ||
	(chord[c].key>=28 && chord[c].function>=-4 && chord[c].function<=2)) {
      if (chord[c].function>=0) {
	chord[c].romnum=((4 * chord[c].function) % 7) + 1;
      }
      if (chord[c].function<0) {
	chord[c].romnum=((((4 * chord[c].function) % 7) + 7) % 7) + 1; 
      }
    }
  }
}
      

print_romnums_kp() {
  int b, c, s, k, new_key, chord_here, linelength=0, first_beat_shown=0, prev_root_shown=50, prev_mode_shown=10, 
    prev_ext_shown=10, prev_fifth_shown=10, nspaces, extnumber;
  printf("\nKostka-Payne Roman Numeral Analysis:\n\n");
  for (b=0; b<numbeats; b++) {    
    if (beat[b].level<beat_printout_level) continue;
    if (beat[b].time==final_timepoint) continue;
    new_key=0;
    nspaces=0;
    chord_here=0;
    if (beat[b].level>=segment_beat_level) {
      printf("|");
      linelength=linelength+1;
      if (linelength>110) {
	printf("\n\n");
	linelength=0;
      }
    }

    for (c=0; c<numchords; c++) {
      if (chord[c].ontime==beat[b].time) {
	chord_here=1;
	if (chord[c].key != chord[c-1].key || (s==0 && first_beat_shown==0)) {            
	  new_key=1;                                         
	  k=chord[c].key;
	  printf("%c", letter[k % 7]);
	  if (k<6 || (k>=28 && k<34)) printf("-");
	  if ((k>=6 && k<13) || (k>=34 && k<41)) printf("b");
	  if ((k>=20 && k<27) || (k>=48 && k<55)) printf("#");
	  if (k==27 || k == 55) printf("x");
	  if (k>=28) printf("m");
	  printf(": ");
	  linelength=linelength+4;
	  first_beat_shown = 1;
	}
	if (chord[c].root==prev_root_shown && chord[c].mode==prev_mode_shown && 
	    chord[c].ext==prev_ext_shown && chord[c].fifth==prev_fifth_shown && new_key==0) {
	  printf (".  ");
	  nspaces=3;
	}
	                         /* If this chord is the same as the chord at last chord shown, and it's not a new key,
				    don't print chord name. */
	else {

	  if (chord[c].ext==0) {
	    if (chord[c].inversion==0) {
	      extnumber=0;
	    }
	    if (chord[c].inversion==1) {
	      extnumber=6;
	      nspaces++;
	    }
	    if (chord[c].inversion==2) {
	      extnumber=64;
	      nspaces=nspaces+2;
	    }
	  }
	  if (chord[c].ext==1) {
	    if (chord[c].inversion==0) {
	      extnumber=7;
	      nspaces++;
	    }
	    if (chord[c].inversion==1) {
	      extnumber=65;
	      nspaces=nspaces+2;
	    }
	    if (chord[c].inversion==2) {
	      extnumber=43;
	      nspaces=nspaces+2;
	    }
	    if (chord[c].inversion==3) {
	      extnumber=42;
	      nspaces=nspaces+2;
	    }
	  }

	  if (chord[c].mode==1 && chord[c].ext==0 && chord[c].fifth==1) {            /* It's a major triad */
	    if (chord[c].romnum==1) {
	      printf("I"); 
	      nspaces=nspaces+1;
	    }
	    if (chord[c].romnum==2) {
	      printf("II"); 
	      nspaces=nspaces+2;
	    }
	    if (chord[c].romnum==3) {
	      printf("III"); 
	      nspaces=nspaces+3;
	    }
	    if (chord[c].romnum==4) {
	      printf("IV"); 
	      nspaces=nspaces+2;
	    }
	    if (chord[c].romnum==5) {
	      printf("V"); 
	      nspaces=nspaces+1;
	    }
	    if (chord[c].romnum==6) {
	      printf("VI"); 
	      nspaces=nspaces+2;
	    }
	    if (chord[c].romnum==7) {
	      printf("VII"); 
	      nspaces=nspaces+3;
	    }
	    if (extnumber!=0 && chord[c].romnum<8) printf("%d", extnumber);
	    prev_mode_shown=1;
	    prev_ext_shown=0;
	    prev_fifth_shown=1;
	  }
	  if (chord[c].mode==2 && chord[c].ext==0 && chord[c].fifth==1) {    /* It's a minor triad */
	    if (chord[c].romnum==1) {
	      printf("i"); 
	      nspaces=nspaces+1;
	    }
	    if (chord[c].romnum==2) {
	      printf("ii"); 
	      nspaces=nspaces+2;
	    }
	    if (chord[c].romnum==3) {
	      printf("iii"); 
	      nspaces=nspaces+3;
	    }
	    if (chord[c].romnum==4) {
	      printf("iv"); 
	      nspaces=nspaces+2;
	    }
	    if (chord[c].romnum==5) {
	      printf("v"); 
	      nspaces=nspaces+1;
	    }
	    if (chord[c].romnum==6) {
	      printf("vi"); 
	      nspaces=nspaces+2;
	    }
	    if (chord[c].romnum==7) {
	      printf("vii"); 
	      nspaces=nspaces+3;
	    }
	    if (extnumber!=0 && chord[c].romnum<8) printf("%d", extnumber);
	    prev_mode_shown=2;
	    prev_ext_shown=0;
	    prev_fifth_shown=1;
	  }
	  if (chord[c].mode==1 && chord[c].ext==1 && chord[c].fifth==1) {   /* It's a dominant seventh - V or secondary V */
	    if (chord[c].romnum<8) printf("V");
	    if (chord[c].romnum<8) printf("%d", extnumber);
	    if (chord[c].romnum==1) {
	      printf("/IV");
	      nspaces=nspaces+4;
	    }
	    if (chord[c].romnum==2) {
	      printf("/V");
	      nspaces=nspaces+3;
	    }
	    if (chord[c].romnum==3) {
	      printf("/vi");
	      nspaces=nspaces+4;
	    }
	    if (chord[c].romnum==4) {
	      printf("/VII");
	      nspaces=nspaces+5;
	    }
	    if (chord[c].romnum==5) {
	      nspaces++;
	    }
	    if (chord[c].romnum==6) {
	      printf("/ii");
	      nspaces=nspaces+4;
	    }
	    if (chord[c].romnum==7) {
	      printf("/iii");
	      nspaces=nspaces+5;
	    }
	    prev_mode_shown=1;
	    prev_ext_shown=1;
	    prev_fifth_shown=1;
	  }
	  if (chord[c].mode==2 && chord[c].ext==1 && chord[c].fifth==1) {     /* It's a minor seventh */

	    if (chord[c].romnum==1) {
	      printf("i");
	      nspaces=nspaces+1;
	    }
	    if (chord[c].romnum==2) {
	      printf("ii");
	      nspaces=nspaces+2;
	    }
	    if (chord[c].romnum==3) {
	      printf("iii");
	      nspaces=nspaces+3;
	    }
	    if (chord[c].romnum==4) {
	      printf("iv");
	      nspaces=nspaces+2;
	    }
	    if (chord[c].romnum==5) {
	      printf("v");
	      nspaces=nspaces+1;
	    }
	    if (chord[c].romnum==6) {
	      printf("vi");
	      nspaces=nspaces+2;
	    }
	    if (chord[c].romnum==7) {
	      printf("vii");
	      nspaces=nspaces+3;
	    }
	    if (chord[c].romnum<8) printf("%d", extnumber);
	    prev_mode_shown=2;
	    prev_ext_shown=1;
	    prev_fifth_shown=1;
	  }

	  if (chord[c].fifth==2) {                                      /* It's a diminished triad or half-dim seventh */
	    if (chord[c].romnum==2 && chord[c].ext==1) {
	      printf("iio7");
	      nspaces=nspaces+4;
	    }	    
	    if (chord[c].romnum==2 && chord[c].ext==0 && chord[c].inversion==0) {
	      printf("iio");
	      nspaces=nspaces+3;
	    }	    
	    if (chord[c].romnum==2 && chord[c].ext==0 && chord[c].inversion==1) {
	      printf("iio6");
	      nspaces=nspaces+4;
	    }	    
	    if (chord[c].romnum!=2 && chord[c].romnum!=8) {
	      printf("Dim");
	      nspaces=3;
	    }
	    prev_fifth_shown=2;
	  }

	  if (chord[c].romnum==8) {
	    printf("Chr"); 
	    nspaces=3;
	  }
	  prev_root_shown=chord[c].root;
	  if (nspaces==1) {
	    printf("  ");
	    nspaces=nspaces+2;
	  }
	  else {
	    printf(" ");
	    nspaces++;
	  }
	}
      }
    }
    if (chord_here==0) {                          /* If after cycling through the beats at printout level, it never finds a
						     a chord there... */
      printf(".  ");
      nspaces=3;
    }
    linelength=linelength+nspaces;
  }
  printf("]|\n");
}

chords_to_romnums_as(){
  int c;
  for(c=0; c<numchords; c++) {
    chord[c].romnum=8;
    if (chord[c].function>=-4 && chord[c].function<=5) {
      if (chord[c].function>=0) {
	chord[c].romnum=((4 * chord[c].function) % 7) + 1;
      }
      if (chord[c].function<0) {
	chord[c].romnum=((((4 * chord[c].function) % 7) + 7) % 7) + 1; 
      }
    }
  }
}

print_romnums_as() {
  int b, c, s, k, new_key, chord_here, linelength=0, first_beat_shown=0, prev_root_shown=50, prev_mode_shown=10, 
    prev_ext_shown=10, prev_fifth_shown=10, nspaces, extnumber;
  printf("\nAldwell-Schachter Roman Numeral Analysis:\n\n");
  for (b=0; b<numbeats; b++) {    
    if (beat[b].level<beat_printout_level) continue;
    if (beat[b].time==final_timepoint) continue;
    new_key=0;
    nspaces=0;
    chord_here=0;
    if (beat[b].level>=segment_beat_level) {
      printf("|");
      linelength=linelength+1;
      if (linelength>110) {
	printf("\n\n");
	linelength=0;
      }
    }

    for (c=0; c<numchords; c++) {
      if (chord[c].ontime==beat[b].time) {
	chord_here=1;
	if (chord[c].key != chord[c-1].key || (s==0 && first_beat_shown==0)) {            
	  new_key=1;                                         
	  k=chord[c].key;
	  printf("%c", letter[k % 7]);
	  if (k<6 || (k>=28 && k<34)) printf("-");
	  if ((k>=6 && k<13) || (k>=34 && k<41)) printf("b");
	  if ((k>=20 && k<27) || (k>=48 && k<55)) printf("#");
	  if (k==27 || k == 55) printf("x");
	  if (k>=28) printf("m");
	  printf(": ");
	  linelength=linelength+4;
	  first_beat_shown = 1;
	}
	if (chord[c].root==prev_root_shown && chord[c].mode==prev_mode_shown && 
	    chord[c].ext==prev_ext_shown && chord[c].fifth==prev_fifth_shown && new_key==0) {
	  printf (".  ");
	  nspaces=3;
	}
	                         /* If this chord is the same as the chord at last chord shown, and it's not a new key,
				    don't print chord name. */
	else {

	  if (chord[c].ext==0) {
	    if (chord[c].inversion==0) {
	      extnumber=0;
	    }
	    if (chord[c].inversion==1) {
	      extnumber=6;
	      nspaces++;
	    }
	    if (chord[c].inversion==2) {
	      extnumber=64;
	      nspaces=nspaces+2;
	    }
	  }
	  if (chord[c].ext==1) {
	    if (chord[c].inversion==0) {
	      extnumber=7;
	      nspaces++;
	    }
	    if (chord[c].inversion==1) {
	      extnumber=65;
	      nspaces=nspaces+2;
	    }
	    if (chord[c].inversion==2) {
	      extnumber=43;
	      nspaces=nspaces+2;
	    }
	    if (chord[c].inversion==3) {
	      extnumber=42;
	      nspaces=nspaces+2;
	    }
	  }
	  if (chord[c].function < -1 && chord[c].key<=27) {
	    printf("b");
	    nspaces++;
	  }
	  if (chord[c].function > 5 && chord[c].key>=28) {
	    printf("#");
	    nspaces++;
	  }
	  if ((chord[c].mode==2 || chord[c].ext==0) && chord[c].fifth!=2) {     /* It's a major triad, or minor triad or 7th*/
	    if (chord[c].romnum==1) {
	      printf("I"); 
	      nspaces=nspaces+1;
	    }
	    if (chord[c].romnum==2) {
	      printf("II"); 
	      nspaces=nspaces+2;
	    }
	    if (chord[c].romnum==3) {
	      printf("III"); 
	      nspaces=nspaces+3;
	    }
	    if (chord[c].romnum==4) {
	      printf("IV"); 
	      nspaces=nspaces+2;
	    }
	    if (chord[c].romnum==5) {
	      printf("V"); 
	      nspaces=nspaces+1;
	    }
	    if (chord[c].romnum==6) {
	      printf("VI"); 
	      nspaces=nspaces+2;
	    }
	    if (chord[c].romnum==7) {
	      printf("VII"); 
	      nspaces=nspaces+3;
	    }
	    if (extnumber!=0 && chord[c].romnum<8) printf("%d", extnumber);

	    if (((chord[c].key <= 27 && chord[c].function > 1)
	      || (chord[c].key >= 28 && (chord[c].function > -2 && chord[c].function!=1))) && chord[c].mode==1 
		&& chord[c].romnum < 8) {
	      printf("#3");
	      nspaces=nspaces+2;
	    }

	    if (((chord[c].key <= 27 && chord[c].function < 2)
	      || (chord[c].key >= 28 && (chord[c].function < -1 || chord[c].function==1))) && chord[c].mode==2 
		 && chord[c].romnum < 8) {
	      printf("b3");
	      nspaces=nspaces+2;
	    }

	    prev_mode_shown=chord[c].mode;
	    prev_ext_shown=chord[c].ext;
	    prev_fifth_shown=1;
	  }

	  if (chord[c].mode==1 && chord[c].ext==1 && chord[c].fifth==1) {   /* It's a dominant seventh - V or secondary V */
	    if (chord[c].romnum<8) printf("V");
	    if (chord[c].romnum<8) printf("%d", extnumber);
	    if (chord[c].romnum==1) {
	      printf("/IV");
	      nspaces=nspaces+4;
	    }
	    if (chord[c].romnum==2) {
	      printf("/V");
	      nspaces=nspaces+3;
	    }
	    if (chord[c].romnum==3) {
	      printf("/vi");
	      nspaces=nspaces+4;
	    }
	    if (chord[c].romnum==4) {
	      printf("/VII");
	      nspaces=nspaces+5;
	    }
	    if (chord[c].romnum==5) {
	      nspaces++;
	    }
	    if (chord[c].romnum==6) {
	      printf("/ii");
	      nspaces=nspaces+4;
	    }
	    if (chord[c].romnum==7) {
	      printf("/iii");
	      nspaces=nspaces+5;
	    }
	    prev_mode_shown=1;
	    prev_ext_shown=1;
	    prev_fifth_shown=1;
	  }


	  if (chord[c].fifth==2) {                                      /* It's a diminished triad or half-dim seventh */
	    if (chord[c].romnum==2 && chord[c].ext==1) {
	      printf("iio7");
	      nspaces=nspaces+4;
	    }	    
	    if (chord[c].romnum==2 && chord[c].ext==0 && chord[c].inversion==0) {
	      printf("iio");
	      nspaces=nspaces+3;
	    }	    
	    if (chord[c].romnum==2 && chord[c].ext==0 && chord[c].inversion==1) {
	      printf("iio6");
	      nspaces=nspaces+4;
	    }	    
	    if (chord[c].romnum!=2 && chord[c].romnum!=8) {
	      printf("Dim");
	      nspaces=3;
	    }
	    prev_fifth_shown=2;
	  }

	  if (chord[c].romnum==8) {
	    printf("Chr"); 
	    nspaces=3;
	  }
	  prev_root_shown=chord[c].root;
	  if (nspaces==1) {
	    printf("  ");
	    nspaces=nspaces+2;
	  }
	  else {
	    printf(" ");
	    nspaces++;
	  }
	}
      }
    }
    if (chord_here==0) {                          /* If after cycling through the beats at printout level, it never finds a
						     a chord there... */
      printf(".  ");
      nspaces=3;
    }
    linelength=linelength+nspaces;
  }
  printf("]|\n");
}

display_table() {
  int i, j;
  for(j=0; j<56; ++j) {
    printf("Key %d: ", j);
    for(i=0; i<56; ++i) {
      printf("%6.0f ", analysis[3][i][j]);
    }
    printf("\n");
  }
}

display_running() {                  /* generates preferred analysis at each segment */



  int key, s, j, k, n, m, i, start_segment, f, nspaces;
  double best_first_score;

  best_first_score=key_score[0][0];                 /* generate best key of first segment, given only first segment */
  for (j=0; j<56; ++j) {                          
    if (key_score[0][j] > best_first_score) {
      best_first_score = key_score[0][j];
      provisional[0][0] = j;
    }
  } 

  for (s=1; s<=segtotal; ++s) {                        /* Now cycle through the segments, creating a provisional analysis of
							piece-so-far at each one */
    for(j=0, k=0; j<56; ++j) {
      n=best[s][j];                                    /* set n to the best key for segment s-1, given key j at segment s */
      m=best[s][k];                                    /* set m to the best key for segment s-1 given key 0 at segment s */
      if (analysis[s][n][j] > analysis[s][m][k]) k=j;   /* if best analysis for [n][j] beats analysis for [m][k], let k = j */
    }

    provisional[s][s] = k;                              /* wherever k ends up, set key for s at k */
    for(i=s; i>=1; --i) {
      provisional[s][i-1] = best[i][k];
      k=provisional[s][i-1];
    }
  }

  for (s=0; s<=segtotal; ++s) {                    /* Now print out the provisionals */
    printf("Seg %3d: " , s);
    start_segment = 0;
    if (s > 39) start_segment = 35;               /* After segment 40, start a new line so as not to wrap around the screen */
    if (s > 74) start_segment = 70;
    for (i=start_segment; i<=s; ++i) {                        
      if (s == i || provisional[s][i] != provisional[s-1][i]) {               
	f = provisional[s][i];
	printf("%c", letter[f % 7]);
	nspaces=1;
	if (f<6 || (f>=28 && f<34)) {
	  printf("-");
	  nspaces=2;
	}
	if ((f>=6 && f<13) || (f>=34 && f<41)) {
	  printf("b");
	  nspaces=2;
	}
	if ((f>=20 && f<27) || (f>=48 && f<55)) {
	  printf("#");
	  nspaces=2;
	}
	if (f==27 || f == 55) {
	  printf("x");
	  nspaces=2;
	}
	if (f>=28) {
	  printf("m");
	  nspaces++;
	}
	if (nspaces==1) printf("  ");
	if (nspaces==2) printf(" "); 
      }
      else {
	printf("-  ");
      } 
    }
    printf("\n");
  }
}

