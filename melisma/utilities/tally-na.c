/* This program takes in a series of statement-groups as outputted by
the program compare-na, like this:

     Offset 0  1.000
     Level -1  1.000
     Level 0  1.000
     Level 1  1.000
     Level 2  1.000

The first line indicates the offset value (the best alignment between
testfile and goldfile addresses), followed by the overall score for
that piece. Subsequent statements give the score for each level.

For each level, the program averages the scores for that level across all the
statement-groups. It outputs the average for each level. It also averages
all these level scores across levels and produces this "overall average",
and outputs the number of files with offset=0.

level -1: average proportion correct =  0.998
level 0: average proportion correct =  0.966
level 1: average proportion correct =  0.928
level 2: average proportion correct =  0.899
level 3: average proportion correct =  0.814
Overall average =  0.931; number with zero offset = 36 out of 46

*/

#include <stdio.h>
#include <string.h>
#include <math.h>


FILE *in_file;
char line[200];
char word[20];
char junk[20];
int in_int;
float in_num[3];
int level_tally[6];
int total_tally;
float level_total[6];

main(argc, argv)
int argc;
char *argv[];
{
    int valid_events_found = 0;
    int i=0, j=0, v=0, N_parts = 0;
    float total_score = 0;
    int correct_offset = 0;

    total_tally = 0;
    for(v=0; v<6; v++) {
	level_total[v] = 0.0;
	level_tally[v] = 0;
    }

    in_file = fopen(argv[1], "r");
    while (fgets(line, sizeof(line), in_file) !=NULL) {
      for (j=0; isspace(line[j]); j++);
      if(line[j] == '\0') continue;
      
      N_parts = sscanf (line, "%s %d %f %f", word, &in_int, &in_num[1], &in_num[2]);

      if(strcmp(word, "Offset")==0) {
	  if(N_parts !=3) {
	      printf("Bad input\n");
	      exit(1);
	  }
	  total_score += in_num[1];
	  total_tally++;
	  /* printf("total_score = %6.3f\n", total_score); */
	  valid_events_found = 1;
	  if(in_int == 0) correct_offset++;
      }

      if(strcmp(word, "Level")==0) {
	  if(N_parts !=3) {
	      printf("Bad input\n");
	      exit(1);
	  }

	  /* We add one to the level number in the input file before reading it in */
	  v=in_int+1;
	  if(v>5 || v<0) {
	      printf("Error: level number out of range\n");
	      exit(1);
	  }
	  level_tally[v]++;
	  level_total[v] += in_num[1];
	  valid_events_found = 1;
      }
    }      

    if(valid_events_found==0) {
	printf("No valid input found\n");
	exit(1);
    }

    for(v=0; v<6; v++) {
	/* printf("level_tally[v] = %d\n", level_tally[v]); */
	if(level_tally[v] == 0) continue;
	printf("level %d: average proportion correct = %6.3f (%d)\n", v-1, level_total[v]/level_tally[v], level_tally[v]);
    }
    printf("Overall corpus score = %6.3f; number with zero offset = %d out of %d\n", total_score/(double)total_tally, correct_offset, total_tally);

}


