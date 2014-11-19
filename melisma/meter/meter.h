
 /***************************************************************************/
 /*                                                                         */
 /*       Copyright (C) 2000 Daniel Sleator and David Temperley             */
 /*           See http://www.link.cs.cmu.edu/music-analysis                 */
 /*        for information about commercial use of this system              */
 /*                                                                         */
 /***************************************************************************/

#define TRUE 1
#define FALSE 0

#define MAX(a,b) ((a)>(b))?(a):(b);
#define MIN(a,b) ((a)<(b))?(a):(b);

#define MAX_PITCH 200 /* the maximum value of a pitch is MAX_PITCH-1. */
                      /* pitches are also assumed to be non-negative */

/* These should be in the params file */
/* #define MIN_PIP 30 */  /* The minimum number of pips that can be in a beat period */
/* #define MAX_PIP 50 */  /* The maximum.... */

/* #define P_SIZE  (MAX_PIP - MIN_PIP + 1) */

/* Lowest level is 0.  */
#define TACTUS_LEVEL 2
#define HIGHEST_LEVEL 4

#define LOW_LEVELS TACTUS_LEVEL   /* Levels start at 0... */
#define HIGH_LEVELS (HIGHEST_LEVEL-TACTUS_LEVEL)
#define N_LEVELS (HIGHEST_LEVEL+1)

#define MUST_USE_PENALTY 10000.0
#define NOT_FIRST_BEAT_PENALTY 0.5

#define ms_to_sec(x)  (((double)(x))/1000.0)

typedef int Pitch;

typedef struct note_struct {
    int start;    /* starting time in milliseconds */
    int duration; /* also in milliseconds */
    int ioi;      /* For monophonic pieces, this is the 
		     inter-onset-interval, which is the amount
		     of time until the next note after this
		     one starts */
    int rioi;     /* register ioi...that is
		     the amount of time until the next
		     note that is within +-7 of the current note */
    int effective_length;   /* This is maximum of the duration and the rioi,
			       but does not exceed the max_effective_length parameter */
    Pitch pitch;  /* the pitch of this note */
    struct note_struct * next;   /* sometimes we want a list of notes */
} Note;

/* The lists of notes corresponding to each pip are stored in the
   following fashion */
typedef struct pip_note_list_struct {
    Note * note;
    char mark;      /* a mark bit used by adjust_notes() */
    double weight;  /* in case we use my weighted method to do quantization */
    struct pip_note_list_struct * next;
} Pip_note_list;

typedef struct pip_struct {
    Pip_note_list * pnl;
    double * score; /* must allocate these arrays when you allocate this element */
    double base;    /* the base score of this pip due to the notes at this time
		       this is not really needed...could use base_score()
		       to compute it as needed */
    double higher_base;
    int best_j;     /* which choice in the score array is the one used here */
    char is_beat[N_LEVELS];  /* Is this pip a beat at this level? */
    char is_first_beat[N_LEVELS];  /* Is this pip the first beat at this level? */
    int chord;      /* added by Davy for chord -> meter thing */
    int nnotes;     /* Davy added -- number of notes beginning on that pip */
    int group_boundary;     /* Davy added -- indicates whether pip is near group boundary */
} Pip;

/* read-input.c */
Note * build_note_list_from_input(void);
void normalize_notes(Note *nl);

/* meter.c */
void build_pip_array(Note *nl);
void compute_tactus_level(void);
void compute_lower_level(int base_level);
void compute_higher_level(int base_level);
void print_levels(void);
double deviation_penalty(int x, int y);
int quantize(int t);

/* misc.c */
void * xalloc(int size);
void xfree(void * p);
void safe_strcpy(char *u, char * v, int usize);

/* adjust_notes.c */
void move_notes(int p1, int p2);
void adjust_notes();
void print_metronome();
void print_standard_note_list(void);

/* global variables */
extern FILE * instream;
extern int N_notes;
extern Note ** note_array;
extern Pip * pip_array;
extern int N_pips;
char this_program[20];

/* Davy added: */
struct {
  int time;
}
prechord[1000];
int N_chords;
void add_chords();
void find_gaps();

/* settable parameters of the algorithm */
extern int verbosity;
extern int terse;                    /* Davy added */
extern int pip_time;
extern double beat_interval_factor;
extern double note_factor;
extern double tactus_min;
extern double tactus_width;
extern double tactus_step;
extern double tactus_max;
extern int    beat_slop;
extern double meter_change_penalty;
extern double raising_change_penalty;
extern double duple_bonus;
extern double max_effective_length;
extern int graphic;
extern int duration_augmentation;
extern double triple_bonus;
extern double note_bonus;
extern int highest_level_to_compute;
extern int lowest_level_to_compute;
