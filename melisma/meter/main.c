
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
#include <ctype.h>

#include "meter.h"

/* 
   First version of the meter finding algorithm. 

   Time is divided into discrete units called pips.

   (1) Read in the note list.

   (2) Convert the note list to a sequence of onset events,
       each of which is weighted by the duration (or keystroke velocity
       if we have that).

   (3) Now we allocate the onset events to the pip sequence.
       This can be done either by rounding the time of the event to an
       integral pip time, or by smearing the event over a couple of
       nearby pips.  (I like the latter solution).

   (4) Now we compute, for each pip in the sequence, an array of scores.
       The score in each element represents the quality of the solution
       involing putting a beat at this point, and putting a beat at a
       previous point in the past (by the given period).  

   (5) By examining the score arrays for the pips near the end of the
       sequence, we should be able to construct a good single beat
       sequence.  This does not give us a full meter structure.  We'll
       do that later on.
 */

void bad_param(char * line) {
    char * x;
    x = strchr(line, '\n');
    if (x != NULL) *x = '\0';
    printf("Warning: cannot interpret \"%s\" in parameters file -- skipping it.\n", line);
 }

void read_parameter_file(char * filename, int file_specified) {
    FILE * param_stream;
    char line[100];
    char part[3][100];
    int i;
    float value;
    
    param_stream = fopen(filename, "r");
    if (param_stream == NULL) {
	if (file_specified) {
	    printf("Warning: cannot open \"%s\".  Using defaults.\n", filename);
	} else {
	    /*      printf("Cannot open \"%s\".  Using defaults.\n", filename);       */
	}
	return;
    }
    if (verbosity > 0) {
    printf("%% Reading parameters from file \"%s\"\n", filename);
    }
    while(fgets(line, sizeof(line), param_stream) != NULL) {
	for (i=0; isspace(line[i]); i++);
	if (line[i] == '%' || line[i] == '\0') continue;  /* ignore comment and blank lines */
	for (i=0; line[i] != '\0'; i++) if (line[i] == '=') line[i] = ' '; /* kill all '=' signs */
	if (sscanf(line, "%99s %99s %99s", part[0], part[1], part[2]) != 2) {
	    bad_param(line);
	    continue;
	}
	if (sscanf(part[1], "%f", &value) != 1) {
	    bad_param(line);
	    continue;
	}

	if (strcmp(part[0], "verbosity") == 0) {
	    if (terse == 0) {  
	    verbosity = value;
	    }
	} else if (strcmp(part[0], "pip_time") == 0) {
	    pip_time = value;
	} else if (strcmp(part[0], "beat_interval_factor") == 0) {
	    beat_interval_factor = value;
	} else if (strcmp(part[0], "note_factor") == 0) {
	    note_factor = value;
	} else if (strcmp(part[0], "tactus_min") == 0) {
	    tactus_min = value;
	} else if (strcmp(part[0], "tactus_max") == 0) {
	    tactus_max = value;
	} else if (strcmp(part[0], "tactus_width") == 0) {
	    tactus_width = value;
	} else if (strcmp(part[0], "tactus_step") == 0) {
	    tactus_step = value;
	} else if (strcmp(part[0], "beat_slop") == 0) {
	    beat_slop = value;
	} else if (strcmp(part[0], "meter_change_penalty") == 0) {
	    meter_change_penalty = value;
	} else if (strcmp(part[0], "raising_change_penalty") == 0) {
	    raising_change_penalty = value;
	} else if (strcmp(part[0], "duple_bonus") == 0) {
	    duple_bonus = value;
	} else if (strcmp(part[0], "triple_bonus") == 0) {
	    triple_bonus = value;
	} else if (strcmp(part[0], "note_bonus") == 0) {
	    note_bonus = value;
	} else if (strcmp(part[0], "max_effective_length") == 0) {
	    max_effective_length = value;
	} else if (strcmp(part[0], "highest_level") == 0) {
	    highest_level_to_compute = value;
	} else if (strcmp(part[0], "lowest_level") == 0) {
	    lowest_level_to_compute = value;
	} else if (strcmp(part[0], "duration_augmentation") == 0) {
	    duration_augmentation = value;
	} else {
	    bad_param(line);
	    continue;
	}
        if (verbosity > 0) {
	  printf("%%    %-24s = %10.4f\n", part[0], value);
	}
    }
    fclose(param_stream);
}

void usage(void) {
    fprintf(stderr, "usage: %s [-p parameter-file] [-graphic] [-verbose] [-terse] [notes-file]\n", this_program);
    exit(1);
}

void main (int argc, char * argv[]) {
    Note * nl;
    char * x;
    int level, j;
    char * parameter_file, * input_file = NULL;
    int param_file_specified = FALSE;

    x = strrchr(argv[0], '/');
    if (x != NULL) x++; else x = argv[0];
    safe_strcpy(this_program, x, sizeof(this_program));

    parameter_file = "parameters";
    
    for (j=1; j<argc; j++) {
	if (strcmp(argv[j], "-p") == 0) {
	    parameter_file = argv[j+1];
	    param_file_specified = TRUE;
	    j++;
	} else if (strcmp(argv[j], "-graphic") == 0) {
	    graphic = 1;
      	} else if (strcmp(argv[j], "-terse") == 0) {
	    terse = 1;
            verbosity = 0; 
	} else if (strcmp(argv[j], "-verbose") == 0) {
	    verbosity = 1;
	} else if (argv[j][0] == '-') {
	    usage();
	} else if (input_file == NULL) {
	    /* assume it's a file */
	    input_file = argv[j];
	} else {
	    usage();
	}
    }
    
    if (input_file != NULL) {
	/* open the specified notes file */
	instream = fopen(input_file, "r");
	if (instream == NULL) {
	    fprintf(stderr, "%s: Cannot open %s\n", this_program, input_file);
	    exit(1);
	}
    } else {
	instream = stdin;
    }
    if (verbosity > 0) {
      printf("%% Note list generated by %s", this_program);
      for (j=1; j<argc;j++) printf(" %s", argv[j]);
      printf("\n%%\n");
    }
    
    read_parameter_file(parameter_file, param_file_specified);
    
    nl = build_note_list_from_input();
    normalize_notes(nl);
    build_pip_array(nl);
    compute_tactus_level();

    for (level = TACTUS_LEVEL; level > lowest_level_to_compute; level--) {
	compute_lower_level(level);
    }
    for (level = TACTUS_LEVEL; level < highest_level_to_compute; level++) {
	compute_higher_level(level);
    }

    if (graphic || verbosity > 0) print_levels();

    /* now move the notes around in the pip array
       so that every note is in a beat pip. */

    adjust_notes(); 

    if (verbosity != 1) {
      print_metronome();
      print_standard_note_list();
    }
}
