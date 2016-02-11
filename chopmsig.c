/*  chopmsig, mne: "chop most significant": will chop the most significant bits of each sample and render into a 16 bit wav file */
/* used wtxslice as template */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h> 

typedef struct /* time point, tpt */
{
    int m, s, c;
    unsigned char ref2beg; /* will be positive if timept is referenced to beginning or byte zero of source file */
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

void usage(void)
{
        printf("Usage: Takes a wav file and chops the most significant bits down to the specified integer.\n");
        printf("2 obligatory args:\n");
        printf("\t1) Name of first wavfile 2) integer the number of least significant bits required to be preserved.\n");
}

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

int dhdrchk(wh_t *hdr1, wh_t *hdr2) /* double header check, as well as sanity, they must match nchans, sampfq and bipsamp */
{
    /* OK .. we test what sort of header we have */
    if ( hdr1->id[0] != 'R' || hdr1->id[1] != 'I' || hdr1->id[2] != 'F' || hdr1->id[3] != 'F' ) { 
        printf("ERROR: RIFF string problem, first wav. Bailing out.\n"); 
        exit(EXIT_FAILURE);
    }
    if ( hdr2->id[0] != 'R' || hdr2->id[1] != 'I' || hdr2->id[2] != 'F' || hdr2->id[3] != 'F' ) { 
        printf("ERROR: RIFF string problem, second wav. Bailing out.\n"); 
        exit(EXIT_FAILURE);
    }

    if ( hdr1->fstr[0] != 'W' || hdr1->fstr[1] != 'A' || hdr1->fstr[2] != 'V' || hdr1->fstr[3] != 'E' || hdr1->fstr[4] != 'f' || hdr1->fstr[5] != 'm' || hdr1->fstr[6] != 't' || hdr1->fstr[7] != ' ' ) { 
        printf("ERROR: WAVEfmt string problem, first wav. Bailing out.\n"); 
        exit(EXIT_FAILURE);
    }
    if ( hdr2->fstr[0] != 'W' || hdr2->fstr[1] != 'A' || hdr2->fstr[2] != 'V' || hdr2->fstr[3] != 'E' || hdr2->fstr[4] != 'f' || hdr2->fstr[5] != 'm' || hdr2->fstr[6] != 't' || hdr2->fstr[7] != ' ' ) { 
        printf("ERROR: WAVEfmt string problem, second wav. Bailing out.\n"); 
        exit(EXIT_FAILURE);
    }

    if ( hdr1->datastr[0] != 'd' || hdr1->datastr[1] != 'a' || hdr1->datastr[2] != 't' || hdr1->datastr[3] != 'a' ) { 
        printf("WARNING: header \"data\" string does not come up in first wav. Bailing out.\n"); 
        exit(EXIT_FAILURE);
    }
    if ( hdr2->datastr[0] != 'd' || hdr2->datastr[1] != 'a' || hdr2->datastr[2] != 't' || hdr2->datastr[3] != 'a' ) { 
        printf("WARNING: header \"data\" string does not come up in second wav. Bailing out.\n"); 
        exit(EXIT_FAILURE);
    }

    if ( hdr1->fmtnum != 16 ) {
        printf("WARNING: fmtnum in first wav is %i, it should be %i. Bailing out.\n", hdr1->fmtnum, 16); 
        exit(EXIT_FAILURE);
    }
    if ( hdr2->fmtnum != 16 ) {
        printf("WARNING: fmtnum in second wav is %i, it should be %i. Bailing out.\n", hdr2->fmtnum, 16); 
        exit(EXIT_FAILURE);
    }
    if ( hdr1->pcmnum != 1 ) {
        printf("WARNING: pcmnum in first wav is %i, it should be %i. Bailing out.\n", hdr1->pcmnum, 1); 
        exit(EXIT_FAILURE);
    }
    if ( hdr2->pcmnum != 1 ) {
        printf("WARNING: pcmnum in second wav is %i, it should be %i. Bailing out.\n", hdr2->pcmnum, 1); 
        exit(EXIT_FAILURE);
    }

    /* OK, all that meant that they're pretty much wav files, now to check if the correspond. The three most important
     * parameters are nchans, sampfq and bipsamp */

    if( (hdr1->nchans != hdr2->nchans) | (hdr1->sampfq != hdr2->sampfq) | (hdr1->bipsamp != hdr2->bipsamp) ) {
       printf("One of either nchans, sampfq or bipsamp did not match up between the two wavs.\n"); 
       return 1;
    }
    return 0;
}

void tx(char *wf1, char *wf2, char *timestr, unsigned char backw)
{
    /* OK, we have two wav files, it's important that they have the same sampling and such parameters.
     * It's going to be a rare case if they don't, but , BUT ... good practice declares that we should.
     * This means we'll open both at same time. First however, we're goig to stat them using stat.h */
    struct stat fsta;
    size_t tstatbyid[2]={0};
    if(stat(wf1, &fsta) == -1) {
        fprintf(stderr,"Can't open input file %s", wf1);
        exit(EXIT_FAILURE);
    }
    tstatbyid[0]=fsta.st_size-44;
    if(stat(wf2, &fsta) == -1) {
        fprintf(stderr,"Can't open input file %s", wf2);
        exit(EXIT_FAILURE);
    }
    tstatbyid[1]=fsta.st_size-44;

    /* OK. as yet we don't know if they are decent WAV files, and wther they have the same parameters
       Note that we open them as a matter of course, because the stat command has already checked for basic file trouble. */
    FILE *wav1fp = fopen(wf1,"rb"), *wav2fp = fopen(wf2,"rb");

    wh_t *hdr1=malloc(sizeof(wh_t)), *hdr2=malloc(sizeof(wh_t));
    if ( fread(hdr1, sizeof(wh_t), sizeof(unsigned char), wav1fp) < 1 ) {
        printf("Can't read %s's file header. Bailing out.\n", wf1);
        exit(EXIT_FAILURE);
    }
    if ( fread(hdr2, sizeof(wh_t), sizeof(unsigned char), wav2fp) < 1 ) {
        printf("Can't read %s's file header. Bailing out.\n", wf2);
        exit(EXIT_FAILURE);
    }
    if( dhdrchk(hdr1, hdr2)) { /* check both wavheaders, they need to match */
        printf("Error. Wav header parameter mismatch. Aborting.\n"); 
        exit(EXIT_FAILURE);
    }
    /* with that check done we can just choose any of the two to calculate a robust byps */
    int calcbyps= hdr1->sampfq*hdr1->nchans*(hdr1->bipsamp/8);

    tpt *p=s2tp(timestr);
#ifdef DBG
    printf("Specified timept: mins=%d, secs=%d, centisecs=%d\n", p->m, p->s, p->c);
#endif
    int point= calcbyps*(p->m*60 + p->s) + p->c*calcbyps/100;

    if(point >= tstatbyid[0]) {
        printf("Error. Specified timept comes after length of first (earlier) wav. Aborting.\n");
        exit(EXIT_FAILURE);
    }
    size_t slicesz= point; /* for clarity */
    unsigned char *bf=malloc(tstatbyid[0]);
    if ( fread(bf, tstatbyid[0], sizeof(unsigned char), wav1fp) < 1 ) {
        printf("Sorry, trouble putting first input file into array. Aborting\n"); 
        exit(EXIT_FAILURE);
    }

    /* instead of closing the file, we'll rewind and write over, wiht truncated data. Erase that. We'll close and reopen */
    fclose(wav1fp);
    wav1fp = fopen(wf1,"wb");
    hdr1->byid = tstatbyid[0]-point;
    hdr1->glen=hdr1->byid+36;
    fwrite(hdr1, sizeof(unsigned char), 44, wav1fp);
    if(backw)
    fwrite(bf+point, sizeof(unsigned char), hdr1->byid, wav1fp);
    else 
    fwrite(bf, sizeof(unsigned char), hdr1->byid, wav1fp);
    fclose(wav1fp);
    free(hdr1);

    /* another buffer, but let's avoid two big ones open */
    size_t newwavsz= tstatbyid[1]+slicesz;
    unsigned char *bf2;
    if(backw) {
        bf2=malloc(newwavsz);
        if ( fread(bf2, tstatbyid[1], sizeof(unsigned char), wav2fp) < 1 ) {
            printf("Sorry, trouble putting second wav file into array. Aborting\n"); 
            exit(EXIT_FAILURE);
        }
        memcpy(bf2+tstatbyid[1], bf, slicesz); 
        free(bf);
    } else {
        bf2=malloc(slicesz);
        memcpy(bf2, bf+(tstatbyid[0]-point), slicesz); 
        free(bf);
        bf2=realloc(bf2, newwavsz);
        if ( fread(bf2+slicesz, tstatbyid[1], sizeof(unsigned char), wav2fp) < 1 ) {
            printf("Sorry, trouble putting second wav file into array. Aborting\n"); 
            exit(EXIT_FAILURE);
        }
    }

    hdr2->byid += slicesz;
    hdr2->glen=hdr2->byid+36;
    fclose(wav2fp); /* open an re-open in order to overwrite */
    wav2fp = fopen(wf2,"wb");
    fwrite(hdr2, sizeof(unsigned char), 44, wav2fp);
    fwrite(bf2, sizeof(unsigned char), newwavsz, wav2fp);
    fclose(wav2fp);
    free(bf2);
    free(hdr2);
    free(p);

    return;
}

int main(int argc, char *argv[])
{
    if(argc != 3) {
        usage();
        exit(EXIT_FAILURE);
    }

    /* here is how we show the difference in transfer directions, the first wav will always be the shorter one, the donor, so to speak
     * while the second is the recipient, borrowing medical terminology */
    if(backw)
        tx(argv[2], argv[1], argv[3], backw);
    else
        tx(argv[1], argv[2], argv[3], backw);

    return 0;
}
