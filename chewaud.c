/*  lopwav. the idea is to lop off either the start or the end of the file. All from the command line */

#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>
#include<sys/time.h>
#include<sys/stat.h>
#include<dirent.h> 
#include<ctype.h> // required for isprint
#include<unistd.h> // required for optopt, opterr and optarg.

#define ARBSZ 128
#define GBUF 64
#define WBUF 8
#define COMMONSFRE 44100
#define COMMONNUCHA 2
typedef unsigned char boole;

typedef struct  /* optstruct_raw, a struct for the options */
{
    boole lflg, rflg, wflg; /* set to 1 for mono-ize on left channel, 2 for right channel, 3 for let program choose. wflg is for raw file */
    unsigned sflg; /* s-flag, sngle split .. which actually means two parts */
    unsigned dflg; /* d-flag, downsampling to 16 bit samples */
    char *istr; /* input filename */
    char *tstr; /* time string which will be converted to mm:ss.hh in due course */
} optstruct_raw;

typedef struct  /* opts_true: a structure for the options reflecting their true types */
{
    boole m; /* if 0 no mono-ization. 1 for use left channel, 2, for right, 3 for arbitrarily choose */
    boole s; /* for the single split */
    boole d; /* for the downsampling 32 to 16 bit samples*/
    boole w; /* if the input file is raw data */
    char *inpfn;
    int mm, ss, hh;
} opts_true;

typedef struct /* wh_t, wavheader type */
{
    char id[4]; // should always contain "RIFF"
    int glen;    // general length: total file length minus 8, could because the types so far seen (id and glen itself) are actually 8 bytes
    char fstr[8]; // format string should be "WAVEfmt ": actually two 4chartypes here.
    int fmtnum; // format number 16 for PCM format
    short pcmnum; // PCM number 1 for PCM format
    short nchans; // num channels
    int sampfq; // sampling frequency: CD quality is 44100, 48000 is also common.
    int byps; // BYtes_Per_Second (aka, BlockAlign) = numchannels* bytes per sample*samplerate. stereo shorts at 44100 . should be 176k.
    short bypc; // BYtes_Per_Capture, bipsamp/8 most probably. A capture is the same as a sample if mono, but half a sample if stereo. bypc usually 2, a short.
    short bipsamp; // bits_per_sample: CD quality is 16.
    char datastr[4]; // should always contain "data"
    int byid; // BYtes_In_Data;
} wh_t; /* wav header type */

char *mktmpd(void)
{
    struct timeval tsecs;
    gettimeofday(&tsecs, NULL);
    char *myt=calloc(14, sizeof(char));
    strncpy(myt, "tmpdir_", 7);
    sprintf(myt+7, "%lu", tsecs.tv_usec);

    DIR *d;
    while((d = opendir(myt)) != NULL) { /* see if a directory witht the same name exists ... very low likelihood though */
        gettimeofday(&tsecs, NULL);
        sprintf(myt+7, "%lu", tsecs.tv_usec);
        closedir(d);
    }
    closedir(d);
    mkdir(myt, S_IRWXU);

    return myt;
}

wh_t *hdr4chunk(int sfre, char nucha, int certainsz) /* a header for a file chunk of certain siez */
{
    wh_t *wh=malloc(sizeof(wh_t));
    strncpy(wh->id, "RIFF", 4);
    strncpy(wh->fstr, "WAVEfmt ", 8);
    strncpy(wh->datastr, "data", 4);
    wh->fmtnum=16;
    wh->pcmnum=1;
    wh->nchans=nucha; /* fed in to function */
    wh->sampfq=sfre; /* fed in to function */
    wh->glen=certainsz-8;
    wh->bipsamp=16;
    wh->bypc=wh->bipsamp/8;
    wh->byps=wh->nchans*wh->sampfq*wh->bypc;
    wh->byid=wh->glen-36;
    return wh;
}

int hdrchk(wh_t *inhdr, size_t statbyid, boole *usestatbyid)
{
    /* OK .. we test what sort of header we have */
    if ( inhdr->id[0] != 'R'
            || inhdr->id[1] != 'I' 
            || inhdr->id[2] != 'F' 
            || inhdr->id[3] != 'F' ) { 
        printf("ERROR: RIFF string problem\n"); 
        return 1;
    }

    if ( inhdr->fstr[0] != 'W'
            || inhdr->fstr[1] != 'A' 
            || inhdr->fstr[2] != 'V' 
            || inhdr->fstr[3] != 'E' 
            || inhdr->fstr[4] != 'f'
            || inhdr->fstr[5] != 'm' 
            || inhdr->fstr[6] != 't'
            || inhdr->fstr[7] != ' ' ) { 
        printf("ERROR: WAVEfmt string problem\n"); 
        return 1;
    }

    if ( inhdr->datastr[0] != 'd'
            || inhdr->datastr[1] != 'a' 
            || inhdr->datastr[2] != 't' 
            || inhdr->datastr[3] != 'a' ) { 
        printf("WARNING: header \"data\" string does not come up\n"); 
        return 1;
    }
    if ( inhdr->fmtnum != 16 ) {
        printf("WARNING: fmtnum is %i, while it's better off being %i\n", inhdr->fmtnum, 16); 
        return 1;
    }
    if ( inhdr->pcmnum != 1 ) {
        printf("WARNING: pcmnum is %i, while it's better off being %i\n", inhdr->pcmnum, 1); 
        return 1;
    }

    printf("There substantial evidence in the non-numerical fields of this file's header to think it is a wav file\nValues held in wavheader:");

	printf("glen: %i\n", inhdr->glen);
	if((size_t)inhdr->byid == statbyid) {
		printf("byid: %zu, which is the same as the stat value of %zu\n", (size_t)inhdr->byid, statbyid);
		*usestatbyid=0;
	} else {
		printf("NOTE: wavheader's byid: %i, does not match stat value of %zu.\n", inhdr->byid, statbyid);
		printf("wavheader's glen will also be wrong of course. Will use fstat output for input file then,\n"); 
		*usestatbyid=1;
	}

	printf("nchans: %d\n", inhdr->nchans);
	printf("sampfq: %i\n", inhdr->sampfq);
	printf("byps: %i\n", inhdr->byps);
	printf("bypc, bytes by capture (count of data in shorts): %d\n", inhdr->bypc);
	printf("bipsamp: %d\n", inhdr->bipsamp);

	if(!usestatbyid & (inhdr->glen+8-44 == inhdr->byid))
		printf("Good, \"byid\" (%i) is 36 bytes smaller than \"glen\" (%i).\n", inhdr->byid, inhdr->glen);
	if(!usestatbyid)
		printf("Duration by byid and byps is: %f\n", (float)(inhdr->byid)/inhdr->byps);
	else
		printf("Duration by fstat and byps is: %f\n", (float)(statbyid)/inhdr->byps);

	if( (inhdr->bypc == inhdr->byps/inhdr->sampfq) && (inhdr->bypc == 2) )
		printf("bypc complies with being 2 and matching byps/sampfq. Data values will be the usual signed shorts.\n"); 

	return 0;
}

opts_true *processopts(optstruct_raw *rawopts)
{
	opts_true *trueopts=calloc(1, sizeof(opts_true));

	/* take care of flags first */
	if( (rawopts->lflg) & (rawopts->rflg) )
		trueopts->m=3;
	else if (rawopts->rflg)
		trueopts->m=2;
	else if (rawopts->lflg)
		trueopts->m=1;
	else if (rawopts->wflg)
		trueopts->w=1;
	else
		trueopts->m=0;

	/* some are easy as in, they are direct tranlsations */
	trueopts->d=rawopts->dflg;
	trueopts->s=rawopts->sflg;

	if(rawopts->istr) {
		/* memcpy(trueopts->name, rawopts->fn, sizeof(strlen(rawopts->fn)));
		 * No, don't bother with that: just copy the pointer. In any case, you'll get a segfault */
		trueopts->inpfn=rawopts->istr;
		printf("The input filename option was defined and is \"%s\".\n", trueopts->inpfn);
	} else {
		printf("A filename after the -i option is obligatory: it's the input wav filename option.\n");
		exit(EXIT_FAILURE);
	}
	if(trueopts->w)
		return trueopts;

	char *tmptr, *tmptr2, tspec[32]={0};

	if(rawopts->tstr) { // this is a complicated conditional. Go into it if you dare. But test first to see if it gives yuo what you want
		tmptr=strchr(rawopts->tstr, ':');
		if(tmptr) {
			sprintf(tspec, "%.*s", (int)(tmptr-rawopts->tstr), rawopts->tstr);
			trueopts->mm=atoi(tspec);
		} else // if null :, mm was already initialised as zero with a calloc, but tmptr2 can't use a null: reset it to the beginning of the string
			tmptr=rawopts->tstr;

		tmptr2=strchr(tmptr, '.'); // hundreths separator
		if(tmptr2) {
			trueopts->hh=atoi(tmptr2+1);
			if(tmptr!=rawopts->tstr)
				sprintf(tspec, "%.*s", (int)(tmptr2-tmptr-1), tmptr+1);
			else 
				sprintf(tspec, "%.*s", (int)(tmptr2-tmptr), tmptr);
		} else if(tmptr!=rawopts->tstr) { // there was a :, but no .
			sprintf(tspec, "%.*s", (int)(tmptr2-tmptr-1), tmptr+1);
		} else if(tmptr==rawopts->tstr) { // here was a :, but no .
			sprintf(tspec, "%.*s", (int)(tmptr2-tmptr), tmptr);
		}
		trueopts->ss=atoi(tspec);
#ifdef DBG
		printf("Mins: %d, secs = %d, hundreths=%d\n", trueopts->mm, trueopts->ss, trueopts->hh);
#endif

	} else {
		printf("Error: time option obligatory, in a later version, this would imply that the entire file should be done.\n");
		exit(EXIT_FAILURE);
	}
	return trueopts;
}

int catchopts(optstruct_raw *opstru, int oargc, char **oargv)
{
	int c;
	opterr = 0;

	while ((c = getopt (oargc, oargv, "lrdswi:t:")) != -1)
		switch (c) {
			case 'l':
				opstru->lflg = 1;
				break;
			case 'r':
				opstru->rflg = 1;
				break;
			case 'd':
				opstru->dflg = 1;
				break;
			case 's':
				opstru->sflg = 1;
				break;
			case 'w':
				opstru->wflg = 1;
				break;
			case 'i':
				opstru->istr = optarg;
				break;
			case 't':
				opstru->tstr = optarg;
				break;
			case '?':
				/* general error state? */
				if (optopt == 'i')
					fprintf (stderr, "Option -%c requires an filename argument.\n", optopt);
				if (optopt == 't')
					fprintf (stderr, "Option -%c requires a mm:ss.hh or simple number (interpreted as seconds).\n", optopt);
				else if (isprint (optopt))
					fprintf (stderr, "`-%c' is not a valid option for this program.\n", optopt);
				else
					fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
				return 1;
			default:
				abort();
		}
	return 0;
}

void prtusage(void)
{
	printf("This program cuts wav (specified by -i, into chunks according to time (mm:ss.hh) interval specified by -t.\n");
	printf("It can also onvert a raw audio file into a wav (specifed by -w option).\n");
	printf("This program converts 32bit sampled to 16 bit by simple shifting.\n"); 
	printf("The input wav is indicated by the -i option. The -m option is a flag for also mono-izing the file.\n");
	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
	if(argc == 1) {
		prtusage();
		exit(EXIT_FAILURE);
	}
	optstruct_raw rawopts={0};
	catchopts(&rawopts, argc, argv);
	opts_true *trueopts=processopts(&rawopts);

	/* Before opening, let's use stat on the wav file. This is vital as wav headers limited by ints while their data isn't */
	struct stat ifsta;
	if(stat(trueopts->inpfn, &ifsta) == -1) {
		fprintf(stderr,"Can't stat input file %s", trueopts->inpfn);
		exit(EXIT_FAILURE);
	}
	size_t statglen, statbyid;
	char *per;
	char *fn=calloc(GBUF, sizeof(char));
	statglen=ifsta.st_size-(trueopts->w)?0:8;
	statbyid=statglen-(trueopts->w)?0:36; // filesz less 8+36, so that's less the wav header
	if(statbyid%2 == 0)
		printf("statbyid is even, good.\n");  // So, what is the significance of that. Say please.
	else {
		printf("Ooops statbyid is not even.\n"); // I suppose only 8bitmono wavs can be odd
		exit(EXIT_FAILURE);
	}

	/* OK, now for reading in the WAV file */
	FILE *inwavfp, *outwavfp;
	inwavfp = fopen(trueopts->inpfn, "rb");
	if ( inwavfp == NULL ) {
		fprintf(stderr,"Can't open input file %s", trueopts->inpfn);
		exit(EXIT_FAILURE);
	}
	/* good, so the file was successfully read .. now its wav-header */
	wh_t *inhdr;
	boole usestatbyid; /* a marker to say whether we should use the stat value of byid instead of the byid in the wavheader */
	if(trueopts->w) {
		inhdr=hdr4chunk(COMMONSFRE, COMMONNUCHA, ifsta.st_size);
		goto raw;
	} else {
		inhdr=malloc(sizeof(wh_t));
		if ( fread(inhdr, sizeof(wh_t), sizeof(char), inwavfp) < 1 ) {
			printf("Can't read file header\n");
			exit(EXIT_FAILURE);
		}
	}
	/*let's test the header */
	if (hdrchk(inhdr, statbyid, &usestatbyid)) {
		printf("Header failed some basic WAV/RIFF file checks.\n");
		exit(EXIT_FAILURE);
	}

	int point;
	printf("stamin=%u, stasec=%u, stahuns=%u\n", trueopts->mm, trueopts->ss, trueopts->hh);
	point=inhdr->byps*(trueopts->mm*60 + trueopts->ss) + trueopts->hh*inhdr->byps/100;
	if(!usestatbyid) {
		if(point >= inhdr->byid) {
			printf("Timepoint at which to loop is over the size of the wav file. Aborting.\n");
			exit(EXIT_FAILURE);
		}
	} else {
		if((size_t)point >= statbyid) {
			printf("Timepoint at which to lop is over the size of the wav file. Aborting.\n");
			exit(EXIT_FAILURE);
		}
	}
	// perhaps the following guys should be called output chunks, as in ochunks, because someone might think that the chunks were input chunks. Actually there are input chunks
	/* note in the following, is ->s is set there will be one split only, (although thsi is inevitable if the poijnt is over half the size of the file */
	int fullchunkz, partchunk;
	if(usestatbyid) {
		fullchunkz= (trueopts->s)? 1 : (int)((statbyid)/point);
		partchunk= (trueopts->s)? point*((int)(statbyid/point) -1) + ((int)(statbyid%point)) : ((int)(statbyid%point));
	} else {
		fullchunkz= (trueopts->s)? 1 : inhdr->byid/point;
		partchunk= (trueopts->s)? point*(inhdr->byid/point -1) + inhdr->byid%point : inhdr->byid%point;
	}
#ifdef DBG
	printf("fullchkz: %d, partchk: %d\n", fullchunkz, partchunk);
#endif
	int chunkquan=(partchunk==0)? fullchunkz : fullchunkz+1;
	int bytesinchunk;

	if(trueopts->s)
		printf("One splitpoint from file of size %d: first of size %d, second of size %d (these 2 added: %d)\n", inhdr->byid, point, partchunk, point + partchunk); 
	else
		printf("Point is %i and goes into the total data payload is %li times plus %.1f%% left over.\n", point, (ifsta.st_size-44)/point, ((ifsta.st_size-44)%point)*100./point);

	char *tmpd=mktmpd();
	printf("Your split chunks will go into directory \"%s\"\n", tmpd);
	// char *bf=malloc(inhdr->byid); // fir slurping the entire file
	char *bf=NULL;
	if(trueopts->w) {
raw:		per=strchr(trueopts->inpfn, '.');
		// 	printf("%s to %s\n", trueopts->inpfn, per);
		sprintf(fn, "%.*s_2.wav", (int)(per-trueopts->inpfn), trueopts->inpfn);
		printf("%s\n", fn);
		outwavfp= fopen(fn,"wb");
		fwrite(inhdr, sizeof(char), 44, outwavfp);
		bf=malloc(ifsta.st_size*sizeof(char));
		fread(bf, ifsta.st_size, sizeof(char), inwavfp);
		fclose(inwavfp);
		fwrite(bf, sizeof(char),  ifsta.st_size, outwavfp);
		fclose(outwavfp);
		free(bf);
		free(inhdr);
		free(fn);
		free(trueopts);
		exit(EXIT_SUCCESS);
	} else if(partchunk>point)
		bf=malloc(partchunk*sizeof(char)); // maximum amoutn we'll be reading from the file
	else
		bf=malloc(point*sizeof(char)); // ref. above: too generous ... in fact it need only be this big

	/* we're also going to mono ize and reduce 32 bit samples to 16, so modify the header */
	wh_t *outhdr=malloc(sizeof(wh_t));
	memcpy(outhdr, inhdr, 44*sizeof(char));
	int i, j;
	int downsz;
	if(trueopts->m) {
		if(trueopts->d)
			downsz=4;
		else
			downsz=2;
	} else {
		if(trueopts->d)
			downsz=2;
		else
			downsz=1;
	}

	if(trueopts->m)
		outhdr->nchans = 1; // if not, keep to original (assured bia memcpy above)
	outhdr->bipsamp=16; //force 16 bit sampling.
	outhdr->bypc=outhdr->bipsamp/8;
	outhdr->byps = outhdr->nchans * outhdr->sampfq * outhdr->bypc;
	for(j=0;j<chunkquan;++j) {

		if( (j==chunkquan-1) && partchunk) {
			bytesinchunk = partchunk;
			outhdr->byid = partchunk/downsz;
		} else { // j==chunkquan-1 with partchunk zero will also get here.
			bytesinchunk = point;
			outhdr->byid = point/downsz;
		}
		outhdr->glen = outhdr->byid+36;

		sprintf(fn, "%s/%03i.wav", tmpd, j);
		outwavfp= fopen(fn,"wb");

		fwrite(outhdr, sizeof(char), 44, outwavfp);

		/* OK read in one chunk of size bytesinchunk. fread runs its fd position down the file */
#ifdef DBG
		printf("byid for this fread = %d, bytesinchunk=%d\n",outhdr->byid, bytesinchunk); 
#endif
		if ( fread(bf, bytesinchunk, sizeof(char), inwavfp) < 1 ) {
			printf("Sorry, trouble putting input file into array. Overshot maybe?\n"); 
			exit(EXIT_FAILURE);
		}

		/* for the mon mix ... it will be lossy of course */
		short combine, combine2;
		int outcome;
		float intermediate;

		if(trueopts->d) { // if we are downsizing from 32 to 16.
			for(i=0;i<outhdr->byid;i+=2) { // this is entirely for small-endian. big endian haughtily ignored. Sorry!
				if(trueopts->m==1) {
					bf[i]=bf[4*i+4];
					bf[i+1]=bf[4*i+5];
				} else if(trueopts->m==2) {
					bf[i]=bf[4*i+6];
					bf[i+1]=bf[4*i+7];
				} else if(trueopts->m==3) {
					combine=0; // assure ourselves we have a clean int.
					combine = (short)bf[4*i+7]; // casually slot into first byte;
					combine <<= 8; // move first byte into second
					combine |= (short)bf[4*i+6]; //merge with least signficant
					combine2=0; // assure ourselves we have a clean int.
					combine2 = (short)bf[4*i+5]; // casually slot into first byte;
					combine2 <<= 8; // move first byte into second
					combine2 |= (short)bf[4*i+4]; //merge with least signficant
					intermediate=(float)(combine + combine2)*.5;
					outcome=(int)(intermediate+.5);
					bf[i]=(char)(outcome&0xff);
					bf[i+1]=(char)((outcome&0xff00)>>8);
				} else if(!trueopts->m) {
					bf[i]=bf[2*i+2];
					bf[i+1]=bf[2*i+3];
				}
			}
		} else if(trueopts->m) { // i.e no downsizing, but yes mono-izing.
			for(i=0;i<outhdr->byid;i+=2) { // this is entirely for small-endian. big endian haughtily ignored. Sorry!
				if(trueopts->m==1) { // want the first channel
					bf[i]=bf[2*i];
					bf[i+1]=bf[2*i+1];
				} else if(trueopts->m==2) {
					bf[i]=bf[2*i+2];
					bf[i+1]=bf[2*i+3];
				} else if(trueopts->m==3) {
					combine=0; // assure ourselves we have a clean int.
					combine = (short)bf[2*i+3]; // casually slot into first byte;
					combine <<= 8; // move first byte into second
					combine |= (short)bf[2*i+2]; //merge with least signficant
					combine2=0; // assure ourselves we have a clean int.
					combine2 = (short)bf[2*i+1]; // casually slot into first byte;
					combine2 <<= 8; // move first byte into second
					combine2 |= (short)bf[2*i]; //merge with least signficant
					intermediate=(float)(combine + combine2)*.5;
					outcome=(int)(intermediate+.5);
					bf[i]=(char)(outcome&0xff);
					bf[i+1]=(char)((outcome&0xff00)>>8);
				}
			}
		}

		fwrite(bf, sizeof(char), outhdr->byid, outwavfp);
		fclose(outwavfp);
	}
	fclose(inwavfp);
	free(tmpd);
	free(bf);
	free(fn);
	free(trueopts);
	free(inhdr);
	free(outhdr);
	return 0;
}
