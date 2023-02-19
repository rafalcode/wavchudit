/* Allows a small sample to be repteadly mixing in to a bigger sample.
 * This imix0 version will not depend on an external text file, so
 * time gaps needs to be regular. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>

#define MXINTV 0x7FFFFFFF /* max int value */

#define ARBSZ 128

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

tpt *s2tp(char *str)
{
    tpt *p=malloc(sizeof(tpt));
    int minsz, secsz, hunsz;
    char *tc, staminchar[5]={0};
    if( (tc=strchr(str, ':')) == NULL) {
        printf("There are no minutes \n"); 
        p->m=0;
        tc=str;
    } else {
        minsz = (int)(tc-str);
        printf("minsz=%i\n", minsz); 
        strncpy(staminchar, str, (int)(tc-str));
        p->m=atoi(staminchar);
        tc++;
    }

    char *ttc, stasecchar[5]={0};
    if( (ttc=strchr(tc, '.')) == NULL) {
        printf("There are no seconds\n"); 
        p->s=0;
        ttc=tc;
    } else {
        secsz = (int)(ttc-tc);
        printf("secsz=%i\n", secsz); 
        strncpy(stasecchar, tc, secsz);
        p->s=atoi(stasecchar);
        ttc++;
    }
    char stahunschar[5]={0};
    printf("hunsz=%i\n", secsz); 
    strcpy(stahunschar, ttc);
    p->h=atoi(stahunschar);
    if((strlen(stahunschar))==1)
        p->h*=10;

    return p;
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
    if(argc != 4) {
        printf("Usage: three args first wav file 2) tim ept 3) second wav file. Note output file is automatic: \"_imix\" is added.\n");
        exit(EXIT_FAILURE);
    }

    int i, j;
    struct stat fsta1;
    if(stat(argv[1], &fsta1) == -1) {
        fprintf(stderr,"Can't stat input file %s", argv[1]);
        exit(EXIT_FAILURE);
    }
    size_t statglen1=fsta1.st_size-8; //filesz less 8
    size_t statbyid1=statglen1-36; // filesz less 8+36, so that's less the wav header
                                 //
    /* parse edit file in very simple terms, that means using scanf */
    FILE *inwavfp1;
    inwavfp1 = fopen(argv[1],"rb");
    if ( inwavfp1 == NULL ) {
        fprintf(stderr,"Can't open input file %s", argv[1]);
        exit(EXIT_FAILURE);
    }

    // read header first
    wh_t *inhdr1=malloc(sizeof(wh_t));
    if ( fread(inhdr1, sizeof(wh_t), sizeof(char), inwavfp1) < 1 ) {
        printf("Can't read first wav file header\n");
        exit(EXIT_FAILURE);
    }
    // and, another check ...
    if(inhdr1->byid != statbyid1) {
        printf("header (%i) and stat (%i) conflicting on size of data payload. Pls revise.\n", inhdr1->byid, statbyid1); 
        exit(EXIT_FAILURE);
    }

    /* timepoint part */
    tpt *p=s2tp(argv[2]);
    printf("stamin=%u, stasec=%u, stahuns=%u\n", p->m, p->s, p->h);
    int point=inhdr1->byps*(p->m*60 + p->s) + p->h*inhdr1->byps/100; //byps always divisible by 100 (normally)
    printf("point-to is %i bytes inside %i bytes which is %.1f%% in.\n", point, inhdr1->byid, (point*100.)/inhdr1->byid); 
    if(point >= inhdr1->byid) {
        printf("Timepoint at which to imix in on first wav file is over its size. Aborting.\n");
        exit(EXIT_FAILURE);
    }

    // OK, now arg3! the second wav file.
    struct stat fsta2;
    if(stat(argv[3], &fsta2) == -1) {
        fprintf(stderr,"Can't stat input file %s", argv[3]);
        exit(EXIT_FAILURE);
    }
    size_t statglen2=fsta2.st_size-8; //filesz less 8
    size_t statbyid2=statglen2-36; // filesz less 8+36, so that's less the wav header

    /* parse edit file in very simple terms, that means using scanf */
    FILE *inwavfp2;
    inwavfp2 = fopen(argv[3],"rb");
    if ( inwavfp2 == NULL ) {
        fprintf(stderr,"Can't open input file %s", argv[3]);
        exit(EXIT_FAILURE);
    }

    // read header first
    wh_t *inhdr2=malloc(sizeof(wh_t));
    if ( fread(inhdr2, sizeof(wh_t), sizeof(char), inwavfp2) < 1 ) {
        printf("Can't read file header\n");
        exit(EXIT_FAILURE);
    }
    // and, another check ...
    if(inhdr2->byid != statbyid2) {
        printf("header (%i) and stat (%i) conflicting on size of data payload. Pls revise.\n", inhdr2->byid, statbyid2); 
        exit(EXIT_FAILURE);
    }

    // now check timept plus file duration is not over: rather work out how many times it would fit.
    int niops = inhdr1->byid / inhdr2->byid; // number insert ops
    int niops2 = niops/2;
    int oprem = inhdr1->byid % inhdr2->byid; // op remainder
    printf("byid1=%i, byid2=%i niops=%i, oprem=%i\n", inhdr1->byid, inhdr2->byid, niops, oprem);

    // we're going to insert the small sample, then wait one durection of its length and put it in again. etc/
    int *ints = calloc(niops2, sizeof(int)); //intervals
    ints[0]= point;
    for(i=1;i<niops2;++i) 
        ints[i]= ints[i-1] + inhdr2->byid*2;
    for(i=0;i<niops2;++i) 
        printf("%i ", ints[i]);
    printf("\n"); 

    /* now we prepare the output file */
    char outfn[ARBSZ]={0};
    strncpy(outfn, argv[1], 4);
    strcat(outfn, "_imix.wav");
    FILE *outwavfp= fopen(outfn,"wb");

    /* write we can already write out inhdr to there */
    fwrite(inhdr1, sizeof(char), 44, outwavfp);
    /* slurpy! */
    unsigned char *bf1=malloc(inhdr1->byid);
    if ( fread(bf1, inhdr1->byid, sizeof(char), inwavfp1) < 1 ) { /* Yup! we slurp in the baby! */
        printf("Sorry, trouble putting input file into array. Overshot maybe?\n"); 
        exit(EXIT_FAILURE);
    }
    fclose(inwavfp1);

    unsigned char *bf2=malloc(inhdr2->byid);
    if ( fread(bf2, inhdr2->byid, sizeof(char), inwavfp2) < 1 ) { /* Yup! we slurp in the baby! */
        printf("Sorry, trouble putting input file into array. Overshot maybe?\n"); 
        exit(EXIT_FAILURE);
    }
    fclose(inwavfp2);

    int whsz = sizeof(wh_t);

    // bigendtmp a long variable name but it' just to
    // remember that it definitely will not be littleendian which is how the numbers are stored on file in wav and in alot of places.
    unsigned short bigendtmp1, bigendtmp2, faddts;
    unsigned char ftu, ftl;
    int faddti, addt;
    float faddt;
    for(j=0;j<niops2;++j)
        for(i=0;i<inhdr2->byid;i+=2)  {
            bigendtmp1=0x00FF&bf1[i+ints[j]];
            bigendtmp1|=0xFF00&(bf1[i+ints[j]+1]<<8);
            bigendtmp2=0x00FF&bf2[i];
            bigendtmp2|=0xFF00&(bf2[i+1]<<8);

            faddts = (short)bigendtmp1 + (short)bigendtmp2;
            ftl=(unsigned char)(faddts&0x00FF);
            ftu=(unsigned char)((faddts>>8)&0x00FF);
            bf1[i+ints[j]] = ftl;
            bf1[i+ints[j]+1] = ftu;
            // printf("bf2i: %.2x, bf2i+1: %.2x, bigendtmp1: %hi, bigendtmp2: %hi, faddti %i, faddts %hi, ftu %x, ftl %x\n", bf2[i], bf2[i+1], bigendtmp1, bigendtmp2, addt, faddt, faddts, ftu, ftl);
        }

    fwrite(bf1, sizeof(char), inhdr1->byid, outwavfp);
    fclose(outwavfp);
    free(bf1);
    free(bf2);

    free(ints);
    free(p);
    free(inhdr1);
    free(inhdr2);
    return 0;
}
