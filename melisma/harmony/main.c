
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

#include "harmony.h"

void bad_param(char * line) {
    char * x;
    x = strchr(line, '\n');
    if (x != NULL) *x = '\0';
    printf("Warning: cannot interpret \"%s\" in parameters file -- skipping it.\n", line);
}

void read_parameter_file(char * filename, int file_specified) {
    FILE * param_stream;
    char line[100];
    char part[14][100];
    int i, c;
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
    /* printf("%%Reading parameters from file \"%s\".\n", filename); */
    while(fgets(line, sizeof(line), param_stream) != NULL) {
	for (i=0; isspace(line[i]); i++);
	if (line[i] == '%' || line[i] == '\0') continue;  /* ignore comment and blank lines */
	for (i=0; line[i] != '\0'; i++) if (line[i] == '=') line[i] = ' '; /* kill all '=' signs */
	if (sscanf(line, "%99s %99s %99s", part[0], part[1], part[2]) != 2) {
	  /* The line has more than two parts; it either has to be the compat parameter or its bad */
	  if(strcmp(part[0], "compat_values") == 0) {
	    if (sscanf(line, "%99s %99f %99f %99f %99f %99f %99f %99f %99f %99f %99f %99f %99f %99s ", part[0], 
		   &compat_value[0], &compat_value[1], &compat_value[2], &compat_value[3], &compat_value[4], &compat_value[5], &compat_value[6], &compat_value[7], &compat_value[8], &compat_value[9], &compat_value[10], &compat_value[11], part[13]) != 13) {
	      bad_param(line);
	      continue;
	    }
	    if(verbosity>0) {
	      printf("%compatibility values: ");
	      for(c=0; c<12; c++) printf("%6.3f ", compat_value[c]);
	      printf("\n");
	    }
	  }
	  else {
	    bad_param(line);
	  }
	  continue;
	}

	/* If we get this far, we know it's an ordinary parameter with one value */	  
	if (sscanf(part[1], "%f", &value) != 1) {
	    bad_param(line);
	    continue;
	} 
	if (strcmp(part[0], "verbosity") == 0) {
	    verbosity = value;
	    value = verbosity;  /* show the truncated value */
	} else if (strcmp(part[0], "tpc_var_factor") == 0) {
	    tpc_var_factor = value;
	} else if (strcmp(part[0], "buckets_per_unit_of_cog") == 0) {
	    buckets_per_unit_of_cog = value;
	} else if (strcmp(part[0], "half_life") == 0) {
	    half_life = value;
	} else if (strcmp(part[0], "pruning_cutoff") == 0) {
	    pruning_cutoff = value;
	} else if (strcmp(part[0], "print_tpc_notes") == 0) {
	    print_tpc_notes = value;
	} else if (strcmp(part[0], "print_chords") == 0) {
	    print_chords = value;
	} else if (strcmp(part[0], "print_beats") == 0) {
	    print_beats = value;
	} else if (strcmp(part[0], "round_to_beat") == 0) {
	    round_to_beat = value;
	} else if (strcmp(part[0], "har_var_factor") == 0) {
	    har_var_factor = value;
	} else if (strcmp(part[0], "odp_linear_factor") == 0) {
	    odp_linear_factor = value;
	} else if (strcmp(part[0], "odp_quadratic_factor") == 0) {
	    odp_quadratic_factor = value;
	} else if (strcmp(part[0], "odp_constant") == 0) {
	    odp_constant = value;
	} else if (strcmp(part[0], "sbp_weight") == 0) {
	    sbp_weight = value;
	} else if (strcmp(part[0], "sbp_constant") == 0) {
	    sbp_constant = value;
	} else if (strcmp(part[0], "compat_factor") == 0) {
	    compat_factor = value;
	} else if (strcmp(part[0], "voice_leading_time") == 0) {
	    voice_leading_time = value;
	} else if (strcmp(part[0], "voice_leading_penalty") == 0) {
	    voice_leading_penalty = value;
	} else if (strcmp(part[0], "prechord_mode") == 0) {
	    prechord_mode = value;
	} else {
	    bad_param(line);
	    continue;
	}
	if (verbosity > 0) {
	    printf("%%%s = %3.2f\n", part[0], value);
	}
    }
    fclose(param_stream);
}

void usage(void) {
    fprintf(stderr, "usage: %s [-p parameter-file] [notes-file]\n", this_program);
    my_exit(1);
}

void main (int argc, char * argv[]) {
    Note * nl;
    int i;
    char *x;
    Chord * clist, * m_clist, * cm_clist;
    char * parameter_file;

    x = strrchr(argv[0], '/');
    if (x != NULL) x++; else x = argv[0];
    safe_strcpy(this_program, x, sizeof(this_program));
    
    if (argc >= 2 && strcmp(argv[1], "-p") == 0) {
	if (argc < 3) usage();
	parameter_file = argv[2];
	i = 3;
    } else {
	parameter_file = "parameters";
	i = 1;
    }
    
    if (argc == i+1) {
	/* open the specified notes file */
	instream = fopen(argv[i], "r");
	if (instream == NULL) {
	    fprintf(stderr, "%s: Cannot open %s\n", this_program, argv[i]);
	    my_exit(1);
	}
    } else if (argc == i) {
	instream = stdin;
    } else {
	usage();
    }
    
    read_parameter_file(parameter_file, parameter_file == argv[2]);
    
    alpha = (log(2.0)/(half_life * 1000));
    initialize_hashing();
    
    nl = build_note_list_from_input();
    label_notes_with_ornamental_dissonance_penalties(nl);

    label_notes_with_voice_leading_neighbor(nl);    
    
    clist = build_chord_representation(nl);
    if (verbosity >= 4) print_chord_list(clist);
    m_clist = build_metered_chord_representation(clist);
    
    modify_ornamental_dissonance_penalties(m_clist);
    
    if (verbosity >= 4) print_chord_list(m_clist);
    cm_clist = compact_metered_chord_representation(m_clist);  
    if (verbosity >= 3) print_chord_list(cm_clist);
    
    initialize_harmonic(cm_clist);
    compute_harmonic_table();   
    
    compute_direct_notes_TPCs(print_chords);
    
    if (verbosity >= 2) print_harmonic();
    if (prechord_mode==1) {
      print_prechords();
    }
    if (print_tpc_notes) print_direct_notes(); 
    if (verbosity >= 1) ASCII_display(m_clist);
    cleanup_harmonic();
    free_chords(clist);
    free_chords(m_clist);
    free_chords(cm_clist);
    
    /* Did I forget to free the note list and note array? */
}
