/*  lopwav. the idea is to lop off either the start or the end of the file. All from the command line */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>

#define MXINTV 0x7FFFFFFF /* max int value */

#define ARBSZ 128
#define ONSZ 16

typedef struct /* time point, tpt */
{
    int m, s, h;
} tpt;

typedef struct
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

char *mkon(char *inpfn)  /* make output filename ... some convenient */
{
    char *on=calloc(ONSZ, sizeof(char));

    struct timeval tsecs;
    gettimeofday(&tsecs, NULL);
    char lsns[7]={0}; // micseconds
    sprintf(lsns, "%lu", tsecs.tv_usec);
    sprintf(on, "t%.*s", 3, lsns); 

    char *per=strrchr(inpfn, '.');
    sprintf(on+4, "_%.*s.wav", 3, per-3); // 5 chars

    /* let's avoid overwriting same named file */
    struct stat fsta;
    while(stat(on, &fsta) != -1) {
        sprintf(lsns, "%lu", tsecs.tv_usec+1UL);
        sprintf(on+1, "%.*s", 3, lsns); 
        sprintf(on+4, "_%.*s.wav", 3, per-3); // 5 chars
    }

    return on;
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

int main(int argc, char *argv[])
{
    if(argc == 1) {
        printf("Usage: this program takes a wav file and renders it into raw\n");
        exit(EXIT_FAILURE);
    }
    /* after overwriting errors settle the output file first, this is where all the other files are cat'ed */
    char *outfn=mkon(argv[1]);

    /* The first check is to see if both these files are compatible. I.e. must have same sample frequencies and the like.
     * I was goign to check to see which was the smallest, but actually right now, we're going to pull them both into memory */
    int i;
    FILE *inwavfp;
    wh_t *inhdr1=malloc(sizeof(wh_t));

    /* first wav file dealt with separately */
    /* Before opening, let's use stat on the wav file */
    struct stat fsta;
    if(stat(argv[1], &fsta) == -1) {
        fprintf(stderr,"Can't stat input file %s", argv[1]);
        exit(EXIT_FAILURE);
    }
    size_t statglen=fsta.st_size-8; //filesz less 8
    size_t statbyid=statglen-36; // filesz less 8+36, so that's less the wav header

    inwavfp = fopen(argv[1],"rb");
    if ( inwavfp == NULL ) {
        fprintf(stderr,"Can't open input file %s", argv[1]);
        exit(EXIT_FAILURE);
    }
    if ( fread(inhdr1, sizeof(wh_t), sizeof(char), inwavfp) < 1 ) {
        printf("Can't read file header\n");
        exit(EXIT_FAILURE);
    }
    if(inhdr1->byid != statbyid) {
        printf("header and stat conflicting on size of data payload. Pls revise.\n"); 
        exit(EXIT_FAILURE);
    }

    FILE *outwavfp= fopen(outfn,"wb");
    fwrite(inhdr1, sizeof(char), 44, outwavfp); /* actually its len and byid are incorrect but we'll rewrite afterwards care of this afterwards */
    char *bf=malloc(inhdr1->byid);
    if ( fread(bf, inhdr1->byid, sizeof(char), inwavfp) < 1 ) { /* Yup! we slurp in the baby! */
        printf("Sorry, trouble putting input file into array\n"); 
        exit(EXIT_FAILURE);
    }
    fwrite(bf, sizeof(char), inhdr1->byid, outwavfp);
    fclose(inwavfp);

    free(bf);
    inhdr1->byid = runningbyid;
    inhdr1->glen = 36+inhdr1->byid;
    rewind(outwavfp);
    fwrite(inhdr1, sizeof(char), 44, outwavfp);
    fclose(outwavfp);

    free(inhdr1);
    free(outfn);

    return 0;
}
