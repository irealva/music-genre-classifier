/* This program takes two nafiles (lists of notes plus addresses) and
compares them. (Run it like this: "compare-na [goldfile] [testfile]".
When compiling, you need to include "-lm": e.g. "cc -lm compare-na.c
-o compare-na".) It compares the note addresses digit-by-digit: for
each digit of goldfile addresses, it calculates the proportion of
corresponding events in the testfile that have the same value in that
digit. (Two events are corresponding if they have the same pitch and
the same ontime within some tolerance. If no corresponding testfile
event is found for a goldfile event, that goldfile event is counted as
unmatched at all digits.) A total score is calculated, which is simply
the average of these proportions over all levels. The output (with
verbosity=0) looks like this:

     Offset 0  1.000
     Level -1  1.000
     Level 0  1.000
     Level 1  1.000
     Level 2  1.000

The first line indicates the offset value (see below), followed by the
total score for the piece. Subsequent statements give the score for
each level.

Matching is only done up to one level lower than the
highest level present in the goldfile. If levels are present in the
goldfile that are not present in the testfile, the testfile values
at that level will just be assumed to be zero. (In other words, an
address 1000 may be read as 01000 or 001000.) 

The program also tries different "offsets" between the files in terms
of the levels, and chooses the offset that gives the best match. (For
example, offset 1 means that level 1 in the gold file is matched to
level 0 in the testfile.)  

Digits in addresses (numbered starting with 0) are assumed to
correspond to metrical levels + 1. The zeroth digit in the address
indicates whether they correspond with any beat at all (0 if they
don't, order number otherwise).

A single testfile event may serve as the corresponding event for
more than one goldfile event. This means that if there are, for example,
multiple goldfile events that all have the same onset time and pitch,
they can all be matched by the same testfile event.  */

#include <stdio.h>
#include <string.h>
#include <math.h>

FILE *first_file;
FILE *second_file;
char line[100];
char noteword[20];

int mnumnotes, nnumnotes;
int verbosity = 0;
int weight_mode = 0;
int tolerance = 70; /* two note ontimes are assumed to be simultaneous
                       if their distance from one another is <= this
                       value. */

struct {
    int ontime;
    int offtime;
    int pitch;
    int address;
} mnote[10000];

struct {
    int ontime;
    int offtime;
    int pitch;
    int address;
} nnote[10000];

struct {
    int ontime;
    int offtime;
    int pitch;
    long address;
} inote[10000];

double level_weight[6] = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
int num_nonzero[6];
double prop_nonzero[6];
int num_correct[6];
double prop_correct[6];
double level_weight_sum, level_weight_mass;
int num_pcorrect;
double total_score;
double match_score[9];
int lowest_level, highest_level, highest_testfile_level, offset;

int mnumbeats[6];
int nnumbeats[6];


double fabs(double x) {
    if(x < 0.0) return -x;
    else return x;
}

int simultaneous(int i, int j) {
    if(i + tolerance >= j && i - tolerance <= j) return 1;
    else return 0;
}

int extract_digit(int n, int v) {

    /* This takes an integer and returns the vth digit (counting from the right) of the integer */

    int i, x;
    i = pow(10, v+1);
    x = n % i;
    x = x / pow(10, v); 
    return x;
}

main(argc, argv)
int argc;
char *argv[];
{
    int b, n, i, c, c2, v, s, level_found, last, best_s;
    double best_score;

    first_file = fopen(argv[1], "r");
    if (fopen(argv[1], "r") == NULL) {
	printf("I can't open that file\n");
	exit(1);
    }

    b=0;
    n=0;

    if(verbosity>1) printf("First file:\n");
    while (fgets(line, sizeof(line), first_file) !=NULL) {            
	if(line[0]=='\n') continue;
	if(sscanf(line, "%s", noteword) !=1) continue;
	if(strcmp (noteword, "ANote") == 0) {
	    sscanf (line, "%s %d %d %d %d", noteword, &mnote[n].ontime, &mnote[n].offtime, &mnote[n].pitch, &mnote[n].address);
	    if(verbosity>1) printf("Note %d %d %d %d\n", mnote[n].ontime, mnote[n].offtime, mnote[n].pitch, mnote[n].address);
	    n++;
	}
    }
    mnumnotes = n;

    /* To set the highest level, look at the first note address in the 
    goldfile; the level of this note (i.e. the nearest lower power of ten) 
    indicates the highest level in the piece.  This indicates levels in
    addresses, not actual metrical levels.  (Levels in addresses are
    one higher than the actual metrical levels they represent.)  So if
    the first address is 10000, highest_level = 4, corresponding to a
    highest metrical level of 3.*/

    lowest_level = 0;
    for(v=0; v<=5; v++) {
	if(mnote[0].address >= pow(10, v)) {
	    highest_level = v;
	}
    }


    if(verbosity>0) printf("Highest level in gold file = %d (metrical level %d)\n", highest_level, highest_level-1);

    second_file = fopen(argv[2], "r");
    if (fopen(argv[2], "r") == NULL) {
	printf("I can't open that file\n");
	exit(1);
    }

    b=0;
    n=0;
    if(verbosity>1) printf("Second file:\n");    
    while (fgets(line, sizeof(line), second_file) !=NULL) {            
	if(line[0]=='\n') continue;
	if(sscanf(line, "%s", noteword) !=1) continue;
	if(strcmp (noteword, "ANote") == 0) {
	    sscanf (line, "%s %d %d %d %d", noteword, &nnote[n].ontime, &nnote[n].offtime, &nnote[n].pitch, &nnote[n].address);
	    if(verbosity>1) printf("Note %d %d %d %d\n", nnote[n].ontime, nnote[n].offtime, nnote[n].pitch, nnote[n].address);
	    n++;
	}
    }
    nnumnotes = n;
    for(v=0; v<=5; v++) {
	if(nnote[0].address >= pow(10, v)) {
	    highest_testfile_level = v;
	}
    }
    if(verbosity>0) printf("Highest level in gold file = %d (metrical level %d)\n", highest_testfile_level, highest_testfile_level-1);
    if(verbosity>0) if(highest_level > highest_testfile_level) printf("Warning: highest goldfile level %d exceeds highest testfile level %d\n", highest_level, highest_testfile_level);

    if(verbosity>0 && mnumnotes != nnumnotes) printf("Warning: number of notes in goldfile (%d) not equal to number in test file (%d)\n", mnumnotes, nnumnotes); 

    if(weight_mode==1) {
	for(v=lowest_level; v<highest_level; v++) {
	    num_nonzero[v]=0;
	    for(n=0; n<mnumnotes; n++) {
		if(extract_digit(mnote[n].address, v)!=0) num_nonzero[v]++;
	    }
	    prop_nonzero[v] = (double)num_nonzero[v] / (double)mnumnotes;
	    level_weight[v] = prop_nonzero[v];
	    if(verbosity>0) printf("Level %d, proportion nonzero = %6.3f\n", v, prop_nonzero[v]);
	}
    }

    /* Now we choose the best offset. We do this by counting the number of beats (as reflected
       in the note addresses) at each level in each file. */

    for(v=0; v<=5; v++) {
	c=0;
	last = 0;
	for(n=0; n<mnumnotes; n++) {
	    if(extract_digit(mnote[n].address, v) != last) {
		last = extract_digit(mnote[n].address, v);
		c++;
	    }
	}
	mnumbeats[v] = c;
	/* printf("Number of mbeats at level %d = %d\n", v, c); */
    }

    for(v=0; v<=5; v++) {
	c2=0;
	last = 0;
	for(n=0; n<nnumnotes; n++) {
	    if(extract_digit(nnote[n].address, v) != last) {
		last = extract_digit(nnote[n].address, v);
		c2++;
	    }
	}
	nnumbeats[v] = c2;
	/* printf("Number of nbeats at level %d = %d\n", v, c2); */
    }

    /* Now we look for the offset that best matches the two address
     lists together in terms of the number of beats at each level.
     Try different values of s, 1 to 7. For each value, add s-4 to the
     levels of the testfile before comparing them to the goldfile
     levels. So if s-4==1, we're comparing goldfile level 1 with
     testfile level 2. (Notice that offset values, as ouputted, correspond to
     4-s. So, comparing goldfile level 1 with testfile level 2 would be
     a stated offset value of -1.) */

    best_score = 1000.0;
    for(s=1; s<8; s++) {
	match_score[s] = 0.0;
	for(v=0; v<=5; v++) {
	    c = mnumbeats[v]+1;
	    if((s-4) + v < 0 || (s-4) + v > 5) c2 = 1;
	    else c2 = nnumbeats[(s-4)+v] + 1;
	    match_score[s] += (double)fabs((double)log( (double)c2 / (double)c));  
	}
	if(verbosity>0) printf("match_score for offset %d = %6.3f\n", 4-s, match_score[s]);
	if(match_score[s] < best_score) {
	    best_score = match_score[s];
	    best_s = s;
	}

	/* if(nnumbeats[(s-4) + 3] < (1.5 * (double)c) && nnumbeats[(s-4) + 3] > (.66 * (double)c)) break; */
    }
    /* printf("best s = %d\n", best_s-4); */

    /* best_s is a number from 1 to 7. We subtract it from 4 to get the actual offset value. */
    offset = 4-best_s;
    compare_addresses(offset); 
}


compare_addresses(int offset) {
    
    int n, m, match_found, v, ma, na;
    double i;
    double accuracy, c;

    i = pow(10, offset);
    if(verbosity>0) printf("Multiplying addresses by %6.3f\n", i);
    for(n=0; n<nnumnotes; n++) {
	inote[n].address = nnote[n].address * i;
	/* printf("Inote %d \n", inote[n].address); */
    }


    for(v=lowest_level; v<highest_level; v++) num_correct[v] = 0;
    num_pcorrect = 0;
    total_score = 0.0;

    for(m=0; m<mnumnotes; m++) {
	match_found = 0;
	for(n=0; n<nnumnotes; n++) {
	    if(mnote[m].pitch == nnote[n].pitch && simultaneous(mnote[m].ontime, nnote[n].ontime)==1) {
		match_found = 1;
		break;
	    }
	}
	if(match_found == 0) {
	    if(verbosity>0) printf("Warning: no corresponding note found for goldfile Note %d %d %d\n", mnote[m].ontime, mnote[m].offtime, mnote[m].pitch);
	    continue;
	}

    /* In counting the matches, we go up to one less than the highest level in the gold file */

	for(v=lowest_level; v<highest_level; v++) {
	    if (extract_digit(mnote[m].address, v) == extract_digit(inote[n].address, v)) {
		num_correct[v]++;
	    }
	}
	/* printf("Match: %d %d\n", mnote[m].address, inote[n].address); */

	if(mnote[m].address == inote[n].address) num_pcorrect++;
	else if(verbosity>1) printf("Non-matching addresses for Note %d at %d: %d, %d\n", mnote[m].pitch, mnote[m].ontime, mnote[m].address, inote[n].address);

    }		

    for(v=lowest_level; v<highest_level; v++) {
	prop_correct[v] = (double)num_correct[v] / (double)mnumnotes;
	total_score += prop_correct[v] * level_weight[v];
	level_weight_mass += level_weight[v];
    }
    total_score = total_score / level_weight_mass;

    if(verbosity>0) printf("Offset %d: Total score = %6.3f\n", offset, total_score);
    if(verbosity==0) printf("Offset %d %6.3f\n", offset, total_score);
    for(v=lowest_level; v<highest_level; v++) {
	if(verbosity==0) printf("Level %d %6.3f\n", v-1, prop_correct[v]);
	if(verbosity>0) printf("Level %d(%d), proportion correct = %6.3f\n", v-1, (v-1)-offset, prop_correct[v]);
    }

}



    
