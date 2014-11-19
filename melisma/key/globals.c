
 /***************************************************************************/
 /*                                                                         */
 /*       Copyright (C) 2000 Daniel Sleator and David Temperley             */
 /*           See http://www.link.cs.cmu.edu/music-analysis                 */
 /*        for information about commercial use of this system              */
 /*                                                                         */
 /***************************************************************************/ 


#define CHANGE_PENALTY 12
#define SEGMENT_BEAT_LEVEL 3
#define BEAT_PRINTOUT_LEVEL 2


int segment_beat_level = SEGMENT_BEAT_LEVEL;
int beat_printout_level = BEAT_PRINTOUT_LEVEL;
float change_penalty = CHANGE_PENALTY;
int npc_or_tpc_profile = 1;
int romnums = 0;
int romnum_type = 0;
int scoring_mode = 1;
int verbosity = 1;
int running = 0;

int harmonic_input=0;
int seg = 1;

char letter[7] = {'C', 'G', 'D', 'A', 'E', 'B', 'F'};
double major_profile[12] = {5.0, 2.0, 3.5, 2.0, 4.5, 4.0, 2.0, 4.5, 2.0, 3.5, 1.5, 4.0};
double minor_profile[12] = {5.0, 2.0, 3.5, 4.5, 2.0, 4.0, 2.0, 4.5, 3.5, 2.0, 1.5, 4.0};
double default_profile_value=1.5;

/*

CBMS profiles:

major_profile = 5.0 2.0 3.5 2.0 4.5 4.0 2.0 4.5 2.0 3.5 1.5 4.0
minor_profile = 5.0 2.0 3.5 4.5 2.0 4.0 2.0 4.5 3.5 2.0 1.5 4.0

Bayesian profiles (based on frequencies in Kostka-Payne corpus):

major_profile =  0.748  0.060  0.488  0.082  0.670  0.460  0.096  0.715  0.104  0.366  0.057  0.400
minor_profile =  0.712  0.084  0.474  0.618  0.049  0.460  0.105  0.747  0.404  0.067  0.133  0.330

Krumhansl's profiles:

major_profile = 6.35 2.23 3.48 2.33 4.38 4.09 2.52 5.19 2.39 3.66 2.29 2.88
minor_profile = 6.33 2.68 3.52 5.38 2.60 3.53 2.54 4.75 3.98 2.69 3.34 3.17

Krumhansl's minor, normalized: 5.94 2.51 3.30 5.05 2.44 3.31 2.38 4.46 3.73 2.52 3.13 2.97

*/

