/*  lopwav. the idea is to lop off either the start or the end of the file. All from the command line */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h> 
#include<ctype.h> // required for isprint
#include<unistd.h> // required for optopt, opterr and optarg.

#define MXINTV 0x7FFFFFFF /* max int value */

#define ARBSZ 128
#define GBUF 64

typedef struct  /* optstruct_raw, a struct for the options */
{
    unsigned char lflg, rflg; /* set to 1 for mono-ize on left channel, 2 for right channel, 3 for let program choose */
    char *istr; /* input filename */
    char *tstr; /* time string which will be converted to mm:ss.hh in due course */
} optstruct_raw;

typedef struct  /* opts_true: a structure for the options reflecting their true types */
{
    unsigned char m; /* if 0 no mono-ization. 1 for use left channel, 2, for right, 3 for arbitrarily choose */
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

long fszfind(FILE *fp)
{
    rewind(fp);
    fseek(fp, 0, SEEK_END);
    long fbytsz = ftell(fp);
    rewind(fp);
    return fbytsz;
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

int hdrchk(wh_t *inhdr)
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

    printf("There substantial evidence in the non-numerical fields of this file's header to think it is a wav file\n");

    printf("glen: %i\n", inhdr->glen);
    printf("byid: %i\n", inhdr->byid);
    printf("nchans: %d\n", inhdr->nchans);
    printf("sampfq: %i\n", inhdr->sampfq);
    printf("byps: %i\n", inhdr->byps);
    printf("bypc, bytes by capture (count of data in shorts): %d\n", inhdr->bypc);
    printf("bipsamp: %d\n", inhdr->bipsamp);

    if(inhdr->glen+8-44 == inhdr->byid)
        printf("Good, \"byid\" (%i) is 36 bytes smaller than \"glen\" (%i).\n", inhdr->byid, inhdr->glen);
    else {
        printf("WARNING: glen (%i) and byid (%i)do not show prerequisite normal relation(diff is %i).\n", inhdr->glen, inhdr->byid, inhdr->glen-inhdr->byid); 
    }
    // printf("Duration by glen is: %f\n", (float)(inhdr->glen+8-wh_tsz)/(inhdr->nchans*inhdr->sampfq*inhdr->byps));
    printf("Duration by byps is: %f\n", (float)(inhdr->glen+8-44)/inhdr->byps);

    if( (inhdr->bypc == inhdr->byps/inhdr->sampfq) && (inhdr->bypc == 2) )
        printf("bypc complies with being 2 and matching byps/sampfq. Data values can therefore be recorded as signed shorts.\n"); 

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
    else
        trueopts->m=0;

    if(rawopts->istr) {
        /* memcpy(trueopts->name, rawopts->fn, sizeof(strlen(rawopts->fn)));
         * No, don't bother with that: just copy the pointer. In any case, you'll get a segfault */
        trueopts->inpfn=rawopts->istr;
        printf("The input wav filename option was defined and it is set at \"%s\".\n", trueopts->inpfn);
    } else {
        printf("A filename after the -i option is obligatory: it's the input wav filename option.\n");
        exit(EXIT_FAILURE);
    }

    char *tmptr, *tmptr2, tspec[32]={0};

    if(rawopts->tstr) { // this is a complicated conditional. Go into it if you dare. But test first to see if it gives yuo what you want
        tmptr=strchr(rawopts->tstr, ':');
        if(tmptr) {
            sprintf(tspec, "%.*s", (int)(tmptr-rawopts->tstr), rawopts->tstr);
            trueopts->mm=atoi(tspec);
        } else // if null :, mm was already initialised as zero with a calloc, but tmptr2 can't use a null: reset it to the beginning of the string
            tmptr=rawopts->tstr;

        tmptr2=strchr(tmptr, '.');
        if(tmptr2) {
            printf("iTes .\n"); 
            trueopts->hh=atoi(tmptr2+1);
            if(tmptr!=rawopts->tstr)
                sprintf(tspec, "%.*s", (int)(tmptr2-tmptr-1), tmptr+1);
            else 
                sprintf(tspec, "%.*s", (int)(tmptr2-tmptr), tmptr);
        } else if(tmptr!=rawopts->tstr) { // here was a :, but no .
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

    while ((c = getopt (oargc, oargv, "lri:t:")) != -1)
        switch (c) {
            case 'l':
                opstru->lflg = 1;
                break;
            case 'r':
                opstru->rflg = 1;
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

    /* Before opening, let's use stat on the wav file */
    struct stat fsta;
    if(stat(trueopts->inpfn, &fsta) == -1) {
        fprintf(stderr,"Can't stat input file %s", argv[2]);
        exit(EXIT_FAILURE);
    }
    size_t statglen=fsta.st_size-8; //filesz less 8
    size_t statbyid=statglen-36; // filesz less 8+36, so that's less the wav header

    FILE *inwavfp;
    inwavfp = fopen(trueopts->inpfn, "rb");
    if ( inwavfp == NULL ) {
        fprintf(stderr,"Can't open input file %s", trueopts->inpfn);
        exit(EXIT_FAILURE);
    }

    if(statbyid%2 == 0)
        printf("statbyid is even, good.\n");  // So, what is the significance of that. Say please.
    else {
        printf("Ooops statbyid is not even.\n"); // I suppose only 8bitmono wavs can be odd
        exit(EXIT_FAILURE);
    }

    /* of course we also need the header for other bits of data */
    wh_t *inhdr=malloc(sizeof(wh_t));
    if ( fread(inhdr, sizeof(wh_t), sizeof(char), inwavfp) < 1 ) {
        printf("Can't read file header\n");
        exit(EXIT_FAILURE);
    }

    printf("stamin=%u, stasec=%u, stahuns=%u\n", trueopts->mm, trueopts->ss, trueopts->hh);
    int point=inhdr->byps*(trueopts->mm*60 + trueopts->ss) + trueopts->hh*inhdr->byps/100;
    if(point >= inhdr->byid) {
        printf("Timepoint at which to lop is over the size of the wav file. Aborting.\n");
        exit(EXIT_FAILURE);
    }
    long iwsz=fszfind(inwavfp);
    // perhaps the following guys should be called output chunks, as in ochunks, because someone might think that the chunks were input chunks. Actually there are input chunks
    int fullchunkz=(iwsz-44)/point;
    int partchunk= ((iwsz-44)%point);
    int chunkquan=(partchunk==0)? fullchunkz : fullchunkz+1;
    int bytesinchunk;

    printf("Point is %i and is %li times plus %.1f%% over.\n", point, (iwsz-44)/point, ((iwsz-44)%point)*100./point);

    char *tmpd=mktmpd();
    char *fn=calloc(GBUF, sizeof(char));
    char *bf=malloc(inhdr->byid); // fir slurping the entire file
    FILE *outwavfp;

    /* we're also going to mono ize and reduce 32 bit samples to 16, so modify the header */
    wh_t *outhdr=malloc(sizeof(wh_t));
    memcpy(outhdr, inhdr, 44*sizeof(char));
    outhdr->nchans = (trueopts->m)? 1: 2;
    outhdr->bipsamp=16;
    outhdr->bypc=outhdr->bipsamp/8;
    outhdr->byps = outhdr->nchans * outhdr->sampfq * outhdr->bypc;
    int i, j;
    int downsz = (trueopts->m)? 4 : 2;
    for(j=0;j<chunkquan;++j) {

        if( (j==chunkquan-1) && partchunk) {
            bytesinchunk = partchunk;
            outhdr->byid = partchunk/downsz;
        } else { 
            bytesinchunk = point;
            outhdr->byid = point/downsz;
        }
        outhdr->glen = outhdr->byid+36;

        sprintf(fn, "%s/%05i.wav", tmpd, j);
        outwavfp= fopen(fn,"wb");

        fwrite(outhdr, sizeof(char), 44, outwavfp);

        /* EXCUSE ME: are you seriously telling you are reading the entire input file for each chunkquan? */
        /* no just in chunk size ... fread runs its fd position down the file */
        if ( fread(bf, bytesinchunk, sizeof(char), inwavfp) < 1 ) {
            printf("Sorry, trouble putting input file into array. Overshot maybe?\n"); 
            exit(EXIT_FAILURE);
        }
        for(i=0;i<bytesinchunk;i++) 
            bf[i/2]=bf[i]>>16;
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
