/*  lopwav. the idea is to lop off either the start or the end of the file. All from the command line */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h> 

#define GBUF 64
#define ARBSZ 256

typedef struct /* time point, tpt */
{
    int m, s, c;
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

tpt *s2tp(char *str) /* maps mins:seconds.centiseconds string into tpt struct: NOTE: a mapping not a conversion */
{
    tpt *p=malloc(sizeof(tpt));
    char *tc, staminchar[5]/*STArt MINutes CHARacter string*/={0};
    if( (tc=strchr(str, ':')) == NULL) { /* case of zero minutes */
#ifdef DBG
        printf("There are no minutes \n"); 
#endif
        p->m=0;
        tc=str; /* set tc to beginning of string */
    } else { /* case when there ARE minutes */
        strncpy(staminchar, str, (size_t)(tc-str));
        p->m=atoi(staminchar);
        tc++; /* move tc to next character after ":" */
    }

    char *ttc, stasecchar[5]/* STArt SEConds CHARacter string*/={0};
    if( (ttc=strchr(tc, '.')) == NULL) {
        printf("There are no seconds\n"); 
        p->s=0;
        ttc=tc;
    } else {
        strncpy(stasecchar, tc, (size_t)(ttc-tc));
        p->s=atoi(stasecchar);
        ttc++;
    }
    char stacentischar[5]/* STArt CENTIseconds CHAR*/={0};
    strcpy(stacentischar, ttc);
    p->c=atoi(stacentischar); /* see next line for preventing say, ".4" coming out as 4 centiseconds instead of 40 */
    if((strlen(stacentischar))==1)
        p->c*=10;

    return p;
}

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

    if( (inhdr->byps != inhdr->sampfq*inhdr->nchans*(inhdr->bsamp/8))) {
        printf("Faulty header: byps does not match sampfq*nchans/bisamp/8 (typically 176k for most-common setup).\n"); 
        printf("Aborting because this program uses byps, sorry.\n");
        exit(EXIT_FAILURE);
    }
    return 0;
}

int main(int argc, char *argv[])
{
    if(argc != 4) {
        printf("Usage: divides wav file into equal parts and (if nec) a single unequal part (remainder)\n");
        printf("Args: 1) Name of first (earlier) wavfile 2) Name of first (later) wavfile 3) mm:ss.hh string.\n");
        exit(EXIT_FAILURE);
    }
    /* parse edit file in very simple terms, that means using scanf */
    FILE *inwavfp;
    inwavfp = fopen(argv[2],"rb");
    if ( inwavfp == NULL ) {
        fprintf(stderr,"Can't open input file %s", argv[1]);
        exit(EXIT_FAILURE);
    }

    /* let's use stat.h to work out size of WAV instead of its header info .. it's more reliable */
    struct stat fsta;
    if(stat(argv[1], &fsta) == -1) {
        fprintf(stderr,"Can't open input file %s", argv[2]);
        exit(EXIT_FAILURE);
    }
    size_t statglen=fsta.st_size-8;
    size_t tstatbyid=statglen-36;

    /* of course we also need the header for other bits of data */
    wh_t *inhdr=malloc(sizeof(wh_t));
    if ( fread(inhdr, sizeof(wh_t), sizeof(unsigned char), inwavfp) < 1 ) {
        printf("Can't read file header\n");
        exit(EXIT_FAILURE);
    }

    tpt *p=s2tp(argv[1]);
#ifdef DBG
    printf("Specified timept: mins=%d, secs=%d, centisecs=%d\n", p->m, p->s, p->c);
#endif
    int calcbyps= inhdr->sampfq*inhdr->nchans*(inhdr->bsamp/8);
    int point=calcbyps*(p->m*60 + p->s) + p->c*calcbyps/100;
    if(point >= tstatbyid) {
        printf("Error. Specified timept comes after length of first (earlier) wav. Aborting.\n");
        exit(EXIT_FAILURE);
    }
    unsigned char *bf=malloc(tstatbyid);
    if ( fread(bf, tstatbyid, sizeof(unsigned char), inwavfp) < 1 ) {
        printf("Sorry, trouble putting input file into array. Aborting\n"); 
        exit(EXIT_FAILURE);
    }
    fclose(inwavfp);
    /* now we'll write a new, truncated, "earlier" wav */
    inhdr->glen -= (inhdr->byid - point);
    inhdr->byid = point;

    /* prepare truncated first(earlier) wavfilename */
    char outfn[ARBSZ]={0};
    char *tmpd=mktmpd();
    strcpy(outfn, tmpd);
    strcat(outfn, "/");
    char *perpt=strrchr(argv[1], '.');
    strncpy(outfn, argv[1], (size_t)(perpt-argv[1]));
    strcat(outfn, "_trunc.wav");
    FILE *outwavfp= fopen(outfn,"wb");

    /* prepare new header and write out buffer up to point */
    inhdr->byid = point;
    inhdr->glen=inhdr->byid+36;
    fwrite(inhdr, sizeof(unsigned char), 44, outwavfp);
    fwrite(bf, sizeof(unsigned char), point, outwavfp);
    fclose(outwavfp);

    /* memory efficiency: we undergo a memcpy here purely to be able to release bf.
     * We do this because it could be quite big and we're about to read in a second file */
    size_t endslicesz=tstatbyid-point;
    unsigned char *bf2=malloc(endslicesz);
    memcpy(bf2, bf+point, endslicesz); 
    free(bf);

    /* so now we take care of the second wav file, which we're going to prefix */
    char *fn=calloc(GBUF, sizeof(char));
    char *bf=malloc(inhdr->byid);
    FILE *outwavfp;

    int j;
    for(j=0;j<chunkquan;++j) {

        if( (j==chunkquan-1) && partchunk)
            inhdr->byid = partchunk;
        else 
            inhdr->byid = point;
        inhdr->glen = inhdr->byid+36;

        sprintf(fn, "%s/%05i.wav", tmpd, j);
        outwavfp= fopen(fn,"wb");

        fwrite(inhdr, sizeof(char), 44, outwavfp);

        if ( fread(bf, inhdr->byid, sizeof(char), inwavfp) < 1 ) {
            printf("Sorry, trouble putting input file into array. Overshot maybe?\n"); 
            exit(EXIT_FAILURE);
        }
        fwrite(bf, sizeof(char), inhdr->byid, outwavfp);
        fclose(outwavfp);
    }
    fclose(inwavfp);
    free(tmpd);
    free(bf);
    free(fn);
    free(p);
    free(inhdr);
    return 0;
}
