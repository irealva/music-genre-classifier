
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

bad_param(char * line) {
  char * x;
  x = strchr(line, '\n');
  if (x != NULL) *x = '\0';
  printf("Warning: cannot interpret \"%s\" in parameters file -- skipping it.\n", line);
}

read_parameter_file(char *filename, int file_specified) {
  char line[100];
  char part[14][100];
  int i, p;
  float value;
  FILE *param_stream;

  if(file_specified) {
      param_stream = fopen(filename, "r");
      if (param_stream == NULL) {
	  printf("Warning: cannot open \"%s\".  Using defaults.\n", filename);
	  return;
      }
  }
  else {
      filename = "parameters";
      param_stream = fopen(filename, "r");
      if(param_stream == NULL) filename = "key/parameters";
      param_stream = fopen(filename, "r");
      if(param_stream == NULL) return;
  }


  while(fgets(line, sizeof(line), param_stream) !=NULL) {
    for (i=0; isspace(line[i]); i++);
    if (line[i] == '%' || line[i] == '\0') continue;  /* ignore comment and blank lines */
    for (i=0; line[i] != '\0'; i++) if (line[i] == '=') line[i] = ' '; /* kill all '=' signs */

    if (sscanf(line, "%99s %99s %99s", part[0], part[1], part[2]) != 2) {
      /* The line has more than two parts; it either has to be a profile statement or its bad */
      if(strcmp(part[0], "major_profile") == 0) {
	if (sscanf(line, "%99s %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %99s ", part[0], 
		   &major_profile[0], &major_profile[1], &major_profile[2], &major_profile[3], &major_profile[4], &major_profile[5], &major_profile[6], &major_profile[7], &major_profile[8], &major_profile[9], &major_profile[10], &major_profile[11], part[13]) != 13) {
	  bad_param(line);
	}
      }
      else if(strcmp(part[0], "minor_profile") == 0) {
	if (sscanf(line, "%99s %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %99s", part[0], 
		   &minor_profile[0], &minor_profile[1], &minor_profile[2], &minor_profile[3], &minor_profile[4], &minor_profile[5], &minor_profile[6], &minor_profile[7], &minor_profile[8], &minor_profile[9], &minor_profile[10], &minor_profile[11], part[13]) != 13) {
	  bad_param(line);
	}
      }

      else {
	bad_param(line);
      }
      continue;
    }

    /* If we get this far, it's an ordinary parameter with one value */
    if (sscanf(part[1], "%f", &value) != 1) {
      bad_param(line);
      continue;
    }
    if (strcmp(part[0], "change_penalty") == 0) {
      change_penalty = value;
    } else if (strcmp(part[0], "scoring_mode") == 0) {
      scoring_mode = value;
    } else if (strcmp(part[0], "npc_or_tpc_profile") == 0) {
      npc_or_tpc_profile = value;
    } else if (strcmp(part[0], "segment_beat_level") == 0) {
      segment_beat_level = value;
    } else if (strcmp(part[0], "beat_printout_level") == 0) {
      beat_printout_level = value;
    } else if (strcmp(part[0], "verbosity") == 0) {
      verbosity = value;
    } else if (strcmp(part[0], "romnums") == 0) {
      romnums = value;
    } else if (strcmp(part[0], "romnum_type") == 0) {
      romnum_type = value;
    } else if (strcmp(part[0], "running") == 0) {
      running = value;
    } else if (strcmp(part[0], "default_profile_value") == 0) {
      default_profile_value = value;
    } else {
      bad_param(line);
    }
  }      
  fclose(param_stream);
  if(verbosity>=2) {
    printf("verbosity = %d\n", verbosity);
    printf("major_profile: ");
    for(p=0; p<12; p++) printf("%6.3f ", major_profile[p]);
    printf("\n");
    printf("minor_profile: ");
    for(p=0; p<12; p++) printf("%6.3f ", minor_profile[p]);
    printf("\n");
    printf("default_profile_value = %6.3f\n", default_profile_value);    
    printf("npc_or_tpc_profile = %d\n", npc_or_tpc_profile);
    printf("scoring_mode = %d\n", scoring_mode);
    printf("change penalty = %6.2f\n", change_penalty);
    printf("segment beat level = %d\n", segment_beat_level);
    printf("beat printout level = %d\n", beat_printout_level);
    printf("romnums = %d\n", romnums);
    printf("romnum_type = %d\n", romnum_type);
    printf("running = %d\n", running);
  }
}

bad_input(int ln)
{
  if(verbosity>1) printf("Bad input on line %d\n", ln);
}

main(argc, argv)
int argc;
char *argv[];
{
    char line[100];
    char noteword[10];
    char junk[10];
    int z=0, b=0, c=0, s=0, j, line_no=0, i, pitch, tpc_found, npc_found;
    
    char *parameter_file = NULL, *input_file = NULL;
    int param_file_specified = 0;
    
    for (j=1; j<argc; j++) {
	if (strcmp(argv[j], "-p") == 0) {
	    parameter_file = argv[j+1];
	    param_file_specified = 1;
	    j++;
	} else if (input_file == NULL) {
	    /* assume it's a file */
	    input_file = argv[j];
	}
    }
    
    read_parameter_file (parameter_file, param_file_specified);

    if(scoring_mode == 0 && npc_or_tpc_profile == 1) {
	printf("Error: scoring mode 0 requires an npc profile\n");
	exit(1);
    }
    if(scoring_mode == 3 && npc_or_tpc_profile == 1) {
	printf("Error: scoring mode 3 requires an npc profile\n");
	exit(1);
    }

    final_timepoint=0;
    if (input_file != NULL) {
	in_file = fopen(input_file, "r");
	if (in_file == NULL) {
	    printf("I can't open that file\n");
	    exit(1);
	}
    } else {
	in_file = stdin;
    }
    
    tpc_found = 0;
    npc_found = 0;

    while (fgets(line, sizeof(line), in_file) !=NULL) {            /* read in TPC_Notes, Chords, and Beats */
	line_no++;
	for (i=0; isspace(line[i]); i++);
	if (line[i]=='%') continue;                                  /* Ignore comments and blank lines */
	if (sscanf (line, "%s", noteword) !=1) continue;             
	
	if (strcmp (noteword, "TPCNote") == 0) { 
	    
	    if(npc_found == 1) {
		printf("Error: Can't combine Notes and TPCNotes in a single input file\n");
		exit(1);
	    }
	    if (sscanf (line, "%s %d %d %d %d %10s", noteword, &note[z].ontime, &note[z].offtime, &note[z].pitch,
			&note[z].tpc, junk) !=5) bad_input(line_no);
	    note[z].duration = note[z].offtime - note[z].ontime;
	    
	    if(npc_or_tpc_profile==1) note[z].tpc=note[z].tpc+12;             /* C = 14 for present purposes */
	    else note[z].tpc = ((note[z].tpc+3+120) % 12) + 9; /* Use this line instead to shift all TPC's into 
								  one cycle of the LOF: 9 to 20 inclusive */
	    
	    /*printf("%d %d %d %d\n", note[z].ontime, note[z].offtime, note[z].pitch, note[z].tpc);  */
	    total_duration = total_duration + note[z].duration;
      tpc_found = 1;
      ++z;
      
	}
	
	else if (strcmp (noteword, "Note") == 0) {  
	    
	    if (npc_or_tpc_profile == 1) {
		printf("Error: TPCNotes needed as input for TPC profile mode\n");
		exit(1);
	    }
	    if(tpc_found == 1) {
		printf("Error: Can't combine Notes and TPCNotes in a single input file\n");
		exit(1);
	    }
	    
	    if (sscanf (line, "%s %d %d %d %10s", noteword, &note[z].ontime, &note[z].offtime, &pitch, junk)!=4)
		bad_input(line_no);
	    
	    note[z].tpc = ((((((pitch%12) * 7) % 12) + 5) % 12) + 9);    /* For note input, generate TPC labels
									    within the 9-to-20 range */
	    note[z].duration = note[z].offtime - note[z].ontime;
	    /* printf("%d %d %d %d\n", note[z].ontime, note[z].offtime, pitch, note[z].tpc);   */
	    total_duration = total_duration + note[z].duration;
	    npc_found = 1;
	    ++z;
	}
	
	else if (strcmp (noteword, "Chord") == 0) {  
	    if (sscanf (line, "%s %d %d %d %10s", noteword, &ichord[c].ontime, &ichord[c].offtime, &ichord[c].root, junk)!=4)
		bad_input(line_no);
	    if((c>0 && ichord[c].ontime!=ichord[c-1].offtime) || ichord[c].ontime>=ichord[c].offtime) {
		printf("Line %d: Error: Chords must be chronological, each one starting where the previous one ended.\n", line_no);
		exit(1);
	    }
	    ichord[c].duration = ichord[c].offtime - ichord[c].ontime;
	    ichord[c].root=ichord[c].root+12;                     /* C = 14 */
	    c++;
	}
	
	else if (strcmp (noteword, "Beat") == 0) {   
	    if(sscanf (line, "%s %d %d %10s", noteword, &beat[b].time, &beat[b].level, junk) != 3) bad_input(line_no);
	    if(b>0 && beat[b].time<=beat[b-1].time) {
		printf("Line %d: Error: Beats must be inputted in chronological order.\n", line_no);
		exit(1);
	    }
	    b++;
	}
	else bad_input(line_no);
    }
    /*    printf("the number of events is %d\n", z);           */
    /*	  printf("total duration is %d\n", total_duration);    */
    
    firstbeat = beat[0].time;
    final_timepoint=beat[b-1].time;
    numnotes = z;
    numchords = c;
    numbeats = b;
    if(z==0) {
	printf("Error: No notes in input.\n");
	exit(1);
    }
    if(c>0)harmonic_input=1;
    if(harmonic_input==1) {
	if(ichord[0].ontime!=beat[0].time) {
	    /* printf("Error: First chord ontime must coincide with first beat.\n");
	       exit(1); */
	    printf("Warning: First chord ontime doesn't coincide with first beat--adjusting it.\n");
	    ichord[0].ontime = beat[0].time;
	}
	if(ichord[c-1].offtime!=final_timepoint) {
	    /* printf("Error: Last chord offtime must coincide with last beat.\n");
	       exit(1); */
	    printf("Warning: Last chord offtime doesn't coincide with last beat--adjusting it.\n");
	    ichord[c-1].offtime = final_timepoint;
	}
    }
    
    for (b=0; b<numbeats; b++) {                          /* create sbeats - beats of segment level or higher */
	if(beat[b].level>=segment_beat_level) {                   
	    sbeat[s].time=beat[b].time;
	    s++;
	}
    }	
    num_sbeats = s;
    seglength = (sbeat[1].time-sbeat[0].time)/1000.0; /* define segment length as the length of the first segment (in secs) */
    if(verbosity>1) printf("seglength = %3.3f\n", seglength); 
    for (c=0; c<numchords; c++) {                         /* set beat_level of chords */
	for (b=0; b<numbeats; b++) {
	    if (ichord[c].ontime==beat[b].time) {
		ichord[c].beatlevel=beat[b].level;
	    }
	}
    }
    
    create_chords();
    create_segments(); 
    fill_segments();
    count_segment_notes();
    if(scoring_mode==0) prepare_profiles(); 
    if(npc_or_tpc_profile==1) generate_tpc_profiles();
    else generate_npc_profiles();
    match_profiles(); 
    if(segtotal>0) {
	make_first_table();
	make_tables();
	best_key_analysis();
    }
    if(romnums==1 && harmonic_input==1){
	generate_chord_info();
	merge_functions();
	if(romnum_type==0) {
	    chords_to_romnums_kp();
	    print_romnums_kp();
	}
	if(romnum_type==1) {
	    chords_to_romnums_as(); 
	    print_romnums_as(); 
	}
    }
#if 0
    display_table();                           /* Displays the search table for one segment (specified there) */
#endif
    if (running==1) display_running(); 
    
}
