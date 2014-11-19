
 /***************************************************************************/
 /*                                                                         */
 /*       Copyright (C) 2000 Daniel Sleator and David Temperley             */
 /*           See http://www.link.cs.cmu.edu/music-analysis                 */
 /*        for information about commercial use of this system              */
 /*                                                                         */
 /***************************************************************************/


#define TRUE 1
#define FALSE 0

#define MAX_PITCH 200 /* the maximum value of a pitch is MAX_PITCH-1. */
                      /* pitches are also assumed to be non-negative */

#define MAX_BEAT_LEVEL 5

/* The following definitions are used both in tpc.c and in harmonic.c */
#if 0
OBS #define OCT_ABOVE ( 2)  /* consider this many octaves above the base_TPC for a note */
OBS #define OCT_BELOW (-2)  /* number to consider below the base_TPC for a note */
#endif

#define LOWEST_TPC (-24)   /* The lowest TPC that the program will generate for
			      the spelling of a note */
#define HIGHEST_TPC (34)   /* The highest */

#if 0
OBS #define N_OCTS (OCT_ABOVE - OCT_BELOW + 1)
OBS 
OBS #define MAX_TPC (11 + 12*OCT_ABOVE)  /* the highest TPC that can occur */
OBS #define MIN_TPC ( 0 + 12*OCT_BELOW)  /* the lowest TPC that can occur */
OBS 
OBS #define N_TPC (MAX_TPC - MIN_TPC + 1)
#endif

/* The average TPC of what we've seen so far is a real number, but will
   be put into discrete buckets.  To this end, we need to define the buckets */

#if 0
OBS #define BUCKETS_PER_UNIT  5
OBS 
OBS /* obsolete? */
OBS #define BUCKET_SIZE (1/BUCKETS_PER_UNIT)   /* the size of the buckets for stuffing info about TPC */
OBS #define N_BUCKETS  ((MAX_TPC - MIN_TPC) * BUCKETS_PER_UNIT)  /* total number of buckets */
#endif

typedef int Pitch;
typedef int NPC;
typedef int TPC;

struct side_effect_struct {
  TPC tpc_choice[100];     /* the choices made for the tpc.  assumes at most 100 notes in a chord */
  double compatibility;    /* the compatibility score achieved in this situation */
  double orn_diss_penalty;
  double strong_beat_penalty;
  double tpc_variance;
  double tpc_cog;
};

#define UNINITIALIZED_TPC -10000

typedef struct direct_note_struct {
    /* This is the list of notes directly read in from the input.
       See comment right before build_note_list_from_input in
       read-input.c for an explanation of this. */
    int start;
    int duration;
    int adjusted_start_time, adjusted_end_time;  /* just used at the beginning to do adjustment */
    int pitch;
    TPC tpc;
    struct direct_note_struct * next;
} DirectNote;

typedef struct note_struct {
    int start;    /* starting time in milliseconds */
    int duration; /* also in milliseconds */
    Pitch pitch;  /* the pitch of this note */
    NPC npc;      /* this and the following are just functions of the pitch...could be removed */
    TPC base_tpc;
    
    TPC tpc;      /* the result of the tpc computation */
    char is_first_note;          /* am I the first note originating from a given input note?
				    This is for the ornamental dissonance rule */
    signed char voice_leading_neighbor;
                                 /* -1 if I am a lower neighbor, 1 if I am an upper neighbor
				    0 otherwise.  See comment at the start of harmony.c about
				    the voice leading rule. */
    
    double orn_dis_penalty;      /* the ornamental dissonance panalty that could hypothetically
				    be applied cause of this note.  computed by 
				    label_notes_with_ornamental_dissonance_penalties() */

    struct note_struct * next;   /* sometimes we want a list of notes */
    DirectNote *directnote;
    
} Note;

/* A chord is just a collection of notes.  Each chord has a beat level
   time.  A start time and a duration.  All the notes of a chord have
   the same start and duration as each of the notes in the cord.  A
   chord can contain no notes.  In the chord representation of a piece,
   the entire piece is filled with chords (some of them represent rests) */

typedef struct chord_struct {
  int start;
  int duration;
  int level;
  int level_time;    /* this is just a shortcut to save writing beatlevel[level].units * baseunit */
  Note * note;       /* the linked list of notes in this chord */
  char is_first_chord;         /* am I the first chord originating from a given chord computed by
				  build_metered_chord_representation()?  Needed for ornamental
				  dissonance computation */
  struct chord_struct * next;
} Chord;


typedef struct beatlevel_struct {
  int count;    /* the number of beats at one lower level that make one beat of this level */
  int start;    /* how many beats to skip of one lower before the first beat of this level */
  int units;    /* the number of base units that make up this beatlevel */
} Beatlevel;

typedef struct beat_struct {
    struct beat_struct * next;
    int level;     /* the level of this beat */
    int start;     /* the time of this beat */
} Beat;

typedef struct bucket_struct {
  float score;           /* the following keep the choices that were made to achieve the above result */

  /*  double compatibility, strong_beat_penalty, orn_diss_penalty; */
                          /* these are here for debugging purposes only.
                             not needed for the algorithm */

  int tpc_prime;          /* this is used only by buckets in the 1st
			     column.  It's used to prime the
			     tpc_choice_score function with a variety of
			     different options in the 1st column */

  float tpc_variance, har_variance;
                          /* these two are not actually used for except
                             printing out the solution for debugging
                             purposes */

  float tpc_cog, har_cog;      /* the actual center of gravity achieved with this solution */

  /* the following 4 things are what is used to lookup this buck in the hash table */
  int int_tpc_cog, int_har_cog;  /* integerized versions of the above
				    actually, it's not really needed to store these and the float versions */
  TPC root, window;

  struct bucket_struct * next;  /* pointer to the next bucket in this hash bucket */
  struct bucket_struct * prev_bucket; /* the bucket containing the previous weighted average TPC */

} Bucket;


typedef struct {
  Chord * chord;
  Bucket ** table;
  double my_mass;     /* mass of this note only */
                      /* the mass of a note is simply its duration in seconds */
  double chord_mass;  /* This is the total scaled mass of this chord and all prior ones */
                      /* at the moment this note begins.  These masses decay with time. */
  double note_mass;   /* this is the decaying note mass, counting this note and all prior ones */

  double decayed_prior_note_mass; /* the mass of all the notes prior to this one, decayed according to the
				     length of the last note */

  double decayed_prior_chord_mass; /* ditto */

} Column;


/* global variables */

extern char this_program[20];
extern Column * column_table;
extern int table_size;
extern FILE * instream;
extern int N_notes;
extern Note ** note_array;
extern int N_chords;
extern int baseunit;
extern Beatlevel beatlevel[];
extern int N_beatlevel;
extern double alpha;
extern struct side_effect_struct side_effect;
extern double buckets_per_unit_of_cog;
extern double pruning_cutoff;
extern int verbosity;
extern int print_tpc_notes;
extern int print_chords;
extern int print_beats;
extern int round_to_beat;
extern double har_var_factor;
extern double half_life;
extern double odp_constant;
extern double odp_linear_factor;
extern double odp_quadratic_factor;
extern double sbp_weight;
extern double sbp_constant;
extern int prechord_mode;
extern double compat_factor;
extern double voice_leading_time;
extern double voice_leading_penalty;
extern double tpc_var_factor;
extern Beat * global_beat_list;
extern Beat ** beat_array;
extern int N_beats;
extern DirectNote * global_DN_list;
extern float compat_value[12];

/* read-input.c */
Note * build_note_list_from_input(void);
NPC Pitch_to_NPC(Pitch p);
TPC base_TPC(NPC n);

/* misc.c */
void my_exit(int code);
void safe_strcpy(char *u, char * v, int usize);
void * xalloc(int size);
void xfree(void * p);

/* chords.c */
void print_chord_list(Chord *c_list);
Chord * build_chord_representation (Note * nl);
Chord * build_metered_chord_representation(Chord *chord);
Chord * compact_metered_chord_representation(Chord *chord);
void free_chords(Chord * ch);
int lookup_beat(int t);
int rlookup_beat(int t, int l, int r);

/* harmonic.c */
void tpc_choice_score(TPC root, TPC window, int same_roots, Chord *ch, double my_mass, double decayed_prior_note_mass, double tpc_cog);
void initialize_hashing(void);
void initialize_octave_table(void);
void initialize_harmonic(Chord *nl);
void compute_harmonic_table(void);
void initialize_tpc_order_table(void);
void print_terse_harmonic(void);
void print_harmonic(void);
void print_prechords(void);
void print_notes(Note * nl);
void label_notes_with_ornamental_dissonance_penalties(Note * nl);
void label_notes_with_voice_leading_neighbor(Note *nl);
void modify_ornamental_dissonance_penalties(Chord *m_clist);
int discrete_cog(double cog);
void cleanup_harmonic (void);
void compute_direct_notes_TPCs(int should_print_chords);

/* display.c */
char * tpc_string(TPC t);
char * limited_tpc_string(TPC t, int * sharps);
void ASCII_display(Chord *long_ch);
int used_on_prev_line[];
void print_direct_notes(void);
