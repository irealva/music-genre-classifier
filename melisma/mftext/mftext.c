/*
 * mftext
 * 
 * Convert a MIDI file to verbose text.
 */

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include "midifile.h"

void prtime();
void initfuncs();

static FILE *F;
int SECONDS;      /* global that tells whether to display seconds or ticks */
int division;        /* from the file header */

int verbose = 0;  /* when 1 it prints the messages in the original mftext
		     program, when 0 it does not print them */

long tempo = 500000; /* the default tempo is 120 beats/minute */

int filegetc()
{
    int x;
    x = getc(F);
	return(x);
}

int main(argc,argv)
char **argv;
{
	FILE *efopen();
	char ch;
	int i, lowest, argcount;

	n=0;
	SECONDS = 0;
	argcount = 1;

	if ((argc > argcount) && strcmp(argv[argcount], "-s")==0) {
	  SECONDS = 1;
	  argcount++;
	}

	if (argc > argcount) {
	  F = efopen(argv[argcount],"r");
	} else {
	  F = stdin;
	}

	initfuncs();

	for(i=0; i<100; i++) davypitch[i]=-1;
	for(i=0; i<100000; i++) davynote[i].done=0;

	Mf_getc = filegetc;

	/* This next function does all the work - Davy */
	midifile();
	fclose(F);

	while(1) {
	  lowest=1000000;
	  for(i=0; i<n; i++) {
	    if(davynote[i].done==1) continue;
	    if(davynote[i].ontime<lowest) lowest=davynote[i].ontime;
	  }
	  for(i=0; i<n; i++) {
	    if(davynote[i].ontime==lowest && davynote[i].done==0) {
	      printf("Note %6d %6d %2d\n", davynote[i].ontime, davynote[i].offtime, davynote[i].pitch);
	      davynote[i].done=1;
	    }
	  }
	  if(lowest==1000000) break;
	}

	exit(0);
}

FILE *
efopen(name,mode)
char *name;
char *mode;
{
	FILE *f;
	const char *errmess;

	if ( (f=fopen(name,mode)) == NULL ) {
 		fprintf(stderr,"*** ERROR in mftext *** Cannot open '%s'!\n",name);
 		perror("Reason");
		exit(1);
	}
	return(f);
}

void error(s)
char *s;
{
	fprintf(stderr,"Error: %s\n",s);
}

void txt_header(format,ntrks,ldivision)
{
        division = ldivision;
	if (verbose) {
	    printf("Header format=%d ntrks=%d division=%d\n",format,ntrks,division);
	}
}

void txt_trackstart()
{
	if (verbose) printf("Track start\n");
}

void txt_trackend()
{
	if (verbose) printf("Track end\n");
}

/*
txt_noteon(chan,pitch,vol)
{
	prtime();
	printf("Note on, chan=%d pitch=%d vol=%d\n",chan+1,pitch,vol);
}

txt_noteoff(chan,pitch,vol)
{
	prtime();
	printf("Note off, chan=%d pitch=%d vol=%d\n",chan+1,pitch,vol);
}
*/

void txt_noteon(chan,pitch,vol)
{
        int newtime = (int) (1000 * mf_ticks2sec(Mf_currtime,division,tempo));
        /* print time in milliseconds */

        if(davypitch[pitch]!=-1) {
	  davynote[n].ontime=davypitch[pitch];
	  davynote[n].offtime=newtime;
	  davynote[n].pitch=pitch;
	  n++;
	  /*printf("Note %6d %6d %2d\n", davypitch[pitch], (int) (1000 * mf_ticks2sec(Mf_currtime,division,tempo)), pitch);  */
	}
	davypitch[pitch]=newtime;
	/*
        printf("Note-on ");
	printf("%d ",(int) (1000 * mf_ticks2sec(Mf_currtime,division,tempo)));  
	printf("%d\n",pitch); */

}

void txt_noteoff(chan,pitch,vol)
{
        int newtime = (int) (1000 * mf_ticks2sec(Mf_currtime, division,tempo));
        /*if(davypitch[pitch]==-1) printf("Error: Pitch %d is being turned off when it's not on\n", pitch);*/

        if(davypitch[pitch]!=-1 && davypitch[pitch]!=newtime) {
	  davynote[n].ontime=davypitch[pitch];
	  davynote[n].offtime=newtime;
	  davynote[n].pitch=pitch;
	  n++;
	  /*	  printf("Note %6d %6d %2d\n", davypitch[pitch], (int) (1000 * mf_ticks2sec(Mf_currtime,division,tempo)), pitch);  */
	  davypitch[pitch]=-1;
	}

  /*        printf("Note-off ");
	printf("%d ", (int) (1000 * mf_ticks2sec(Mf_currtime,division,tempo)));  
	printf("%d\n",pitch); */

}

void txt_pressure(chan,pitch,press)
{
	prtime();
	printf("Pressure, chan=%d pitch=%d press=%d\n",chan+1,pitch,press);
}

void txt_parameter(chan,control,value)
{
	prtime();
	printf("Parameter, chan=%d c1=%d c2=%d\n",chan+1,control,value);
}

void txt_pitchbend(chan,msb,lsb)
{
	prtime();
	printf("Pitchbend, chan=%d msb=%d lsb=%d\n",chan+1,msb,lsb);
}

void txt_program(chan,program)
{
	prtime();
	printf("Program, chan=%d program=%d\n",chan+1,program);
}

void txt_chanpressure(chan,press)
{
	prtime();
	printf("Channel pressure, chan=%d pressure=%d\n",chan+1,press);
}

void txt_sysex(leng,mess)
char *mess;
{
	prtime();
	printf("Sysex, leng=%d\n",leng);
}

void txt_metamisc(type,leng,mess)
char *mess;
{
	prtime();
	printf("Meta event, unrecognized, type=0x%02x leng=%d\n",type,leng);
}

void txt_metaspecial(type,leng,mess)
char *mess;
{
	prtime();
	printf("Meta event, sequencer-specific, type=0x%02x leng=%d\n",type,leng);
}

void txt_metatext(type,leng,mess)
char *mess;
{
	static char *ttype[] = {
		NULL,
		"Text Event",		/* type=0x01 */
		"Copyright Notice",	/* type=0x02 */
		"Sequence/Track Name",
		"Instrument Name",	/* ...       */
		"Lyric",
		"Marker",
		"Cue Point",		/* type=0x07 */
		"Unrecognized"
	};
	int unrecognized = (sizeof(ttype)/sizeof(char *)) - 1;
	register int n, c;
	register char *p = mess;

	if ( type < 1 || type > unrecognized )
		type = unrecognized;
	prtime();
	printf("Meta Text, type=0x%02x (%s)  leng=%d\n",type,ttype[type],leng);
	printf("     Text = <");
	for ( n=0; n<leng; n++ ) {
		c = *p++;
		printf( (isprint(c)||isspace(c)) ? "%c" : "\\0x%02x" , c);
	}
	printf(">\n");
}

void txt_metaseq(num)
{
	prtime();
	printf("Meta event, sequence number = %d\n",num);
}

void txt_metaeot()
{
	prtime();
	if (verbose) printf("Meta event, end of track\n");
}

void txt_keysig(sf,mi)
{
	prtime();
	if (verbose) printf("Key signature, sharp/flats=%d  minor=%d\n",sf,mi);
}

void txt_tempo(ltempo)
long ltempo;
{
	tempo = ltempo;
	prtime();
	if (verbose) printf("Tempo, microseconds-per-MIDI-quarter-note=%ld\n",tempo);
}

void txt_timesig(nn,dd,cc,bb)
{
	int denom = 1;
	while ( dd-- > 0 )
		denom *= 2;
	prtime();
	if (verbose) printf("Time signature=%d/%d  MIDI-clocks/click=%d  32nd-notes/24-MIDI-clocks=%d\n",
		nn,denom,cc,bb);
}

void txt_smpte(hr,mn,se,fr,ff)
{
	prtime();
	printf("SMPTE, hour=%d minute=%d second=%d frame=%d fract-frame=%d\n",
		hr,mn,se,fr,ff);
}

void txt_arbitrary(leng,mess)
char *mess;
{
	prtime();
	printf("Arbitrary bytes, leng=%d\n",leng);
}

void prtime()
{
    if (verbose) {
	if(SECONDS) {
	    printf("Time=%f   ",mf_ticks2sec(Mf_currtime,division,tempo));
	} else {
	    printf("Time=%ld  ",Mf_currtime);
	}
    }
}

void initfuncs()
{
	Mf_error = error;
	Mf_header =  txt_header;
	Mf_trackstart =  txt_trackstart;
	Mf_trackend =  txt_trackend;
	Mf_noteon =  txt_noteon;
	Mf_noteoff =  txt_noteoff;
	Mf_pressure =  txt_pressure;
	Mf_parameter =  txt_parameter;
	Mf_pitchbend =  txt_pitchbend;
	Mf_program =  txt_program;
	Mf_chanpressure =  txt_chanpressure;
	Mf_sysex =  txt_sysex;
	Mf_metamisc =  txt_metamisc;
	Mf_seqnum =  txt_metaseq;
	Mf_eot =  txt_metaeot;
	Mf_timesig =  txt_timesig;
	Mf_smpte =  txt_smpte;
	Mf_tempo =  txt_tempo;
	Mf_keysig =  txt_keysig;
	Mf_seqspecific =  txt_metaspecial;
	Mf_text =  txt_metatext;
	Mf_arbitrary =  txt_arbitrary;
}
