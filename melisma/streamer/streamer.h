
 /***************************************************************************/
 /*                                                                         */
 /*       Copyright (C) 2000 Daniel Sleator and David Temperley             */
 /*           See http://www.link.cs.cmu.edu/music-analysis                 */
 /*        for information about commercial use of this system              */
 /*                                                                         */
 /***************************************************************************/ 

#define SEGMENT_BEAT_LEVEL 1
#define MAXV 10                   /* The maximum number of squares in an analysis - EVER. This is used to 
define the arrays. */

#define MV 4                      /* The maximum number of black squares in a segment; also the maximum number of squares
				   in an analysis. This can be set in a parameter file.  */
#define MG 6                      /* The maximum number of grey squares in a segment */
#define MA 1000                   /* The maximum number of analyses for a segment */
#define MT 256                    /* The maximum number of transitions for an analysis */
#define MC 4                      /* The maximum number of voices in an analysis in which collisions are allowed 
				   (if the analysis has MC or fewer voices, allow collisions) */
#define BSP 20
#define CP 20
#define NVP 20
#define TNVP 20
#define TBSP 20
#define PPP 1

FILE *in_file;
FILE *out_file;
char line[50];

int verbosity = 1;
int max_voices = MV;
int max_grey = MG;
int max_collisions = MC;
int pitch_proximity_penalty = PPP;
int blank_square_penalty = BSP;
int new_voice_penalty = NVP;
int collision_penalty = CP;
int top_new_voice_penalty = TNVP;
int top_blank_square_penalty = TBSP;

char noteword[10];
int display_command;
int numnotes, numbeats;
int total_duration;
int final_timepoint;
int segment_beat_level = SEGMENT_BEAT_LEVEL;
double seglength;

typedef struct note_struct {
  int ontime;
  int offtime;
  int duration;
  int pitch;
  int segment;
  int voice_number;
} blah;

struct note_struct note[10000];

struct {
  int time;
  int level;
} beat[5000];

struct {
  int start;
  int end;
  int duration;
  int bsp;             /* blank square penalty */
  int tbsp;            /* extra penalty for blank square in top voice */
  int cp;              /* collision penalty */
  int beatlevel;
  int inote[20];
  int snote[20];
  int numnotes;        /* number of black or grey squares in the segment */
  int numblack; 
  int column[100];
  int numanal;
  int analysis[MA][MAXV];
  int analtop[MA];
  int analcard[MA];
  int analblack[MA];
  int analcont[MA];
  int analscore[MA];
  int voice_number[MAXV];
} segment[1000];        /* An array storing the notes in each segment. */
int segtotal;              /* total number of segments - 1 */

int canceled[100];    /* This value is 1 for a pitch if another pitch a half-step or whole-step away has occurred more
			   recently than the pitch itself */

int prov_analysis[MAXV+1];

/* The arrays below are only kept for a given pair of segments. Then the data is just overwritten for the next pair of
   segments. */

int prov_trans[MAXV+1];

int ltransition[MA][MT][MAXV];            /* All the transitions for the left side of an analysis pair. 
					      [analysis number][transition number][pitches of transition] */

int ltranscard[MA][MT];                 /* The cardinality of each transition */

int rtransition[MA][MT][MAXV];
int rtranscard[MA][MT];

/* The arrays below keep data for all segments (the entire piece) */

int best_transition_score;
int best_ltransition[MAXV];
int best_rtransition[MAXV];
int best_transcard;

int seg, a, t;
int ltnum[MA], rtnum[MA];                      /* The number of transitions for each analysis */

int best[1000][MA];                              /* The best prior analysis for each analysis */
int global_analysis[1000][MA];
int final[1000];
int final_ltransition[1000][MAXV];
int final_rtransition[1000][MAXV];
int final_transcard[1000];

