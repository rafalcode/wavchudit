/* Takes a wav file and an mplayer-generated edl file and extracts the chunks */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h> 

#define MXINTV 0x7FFFFFFF /* max int value */

#define ARBSZ 128
#define GBUF 64
#define WBUF 8

typedef unsigned char boole;

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

typedef struct /* wseq_t */
{
    size_t *wln;
    size_t wsbuf;
    size_t quan;
    size_t lbuf;
    size_t numl;
    size_t *wpla; /* words per line array number of words per line */
} wseq_t;

wseq_t *create_wseq_t(size_t initsz)
{
    wseq_t *words=malloc(sizeof(wseq_t));
    words->wsbuf = initsz;
    words->quan = initsz;
    words->wln=calloc(words->wsbuf, sizeof(size_t));
    words->lbuf=WBUF;
    words->numl=0;
    words->wpla=calloc(words->lbuf, sizeof(size_t));
    return words;
}

void free_wseq(wseq_t *wa)
{
    free(wa->wln);
    free(wa->wpla);
    free(wa);
}

double *processinpf(char *fname, int *m, int *n)
{
    /* In order to make no assumptions, the file is treated as lines containing the same amount of words each,
     * except for lines starting with #, which are ignored (i.e. comments). These words are checked to make sure they contain only floating number-type
     * characters [0123456789+-.] only, one string variable is icontinually written over and copied into a growing floating point array each time */

    /* declarations */
    FILE *fp=fopen(fname,"r");
    int i;
    size_t couc /*count chars */, couw=0 /* count words */, oldcouw = 0;
    char c;
    boole inword=0;
    wseq_t *wa=create_wseq_t(GBUF);
    size_t bwbuf=WBUF;
    char *bufword=calloc(bwbuf, sizeof(char)); /* this is the string we'll keep overwriting. */

    double *mat=malloc(GBUF*sizeof(double));

    while( (c=fgetc(fp)) != EOF) {
        /*  take care of  */
        if( (c== '\n') | (c == ' ') | (c == '\t') | (c=='#')) {
            if( inword==1) { /* we end a word */
                wa->wln[couw]=couc;
                bufword[couc++]='\0';
                bufword = realloc(bufword, couc*sizeof(char)); /* normalize */
                mat[couw]=atof(bufword);
                couc=0;
                couw++;
            }
            if(c=='#') {
                while( (c=fgetc(fp)) != '\n') ;
                continue;
            } else if(c=='\n') {
                if(wa->numl == wa->lbuf-1) {
                    wa->lbuf += WBUF;
                    wa->wpla=realloc(wa->wpla, wa->lbuf*sizeof(size_t));
                    memset(wa->wpla+(wa->lbuf-WBUF), 0, WBUF*sizeof(size_t));
                }
                wa->wpla[wa->numl] = couw-oldcouw;
                oldcouw=couw;
                wa->numl++;
            }
            inword=0;
        } else if( (inword==0) && ((c == 0x2B) | (c == 0x2D) | (c == 0x2E) | ((c >= 0x30) && (c <= 0x39))) ) { /* deal with first character of new word, + and - also allowed */
            if(couw == wa->wsbuf-1) {
                wa->wsbuf += GBUF;
                wa->wln=realloc(wa->wln, wa->wsbuf*sizeof(size_t));
                mat=realloc(mat, wa->wsbuf*sizeof(double));
                for(i=wa->wsbuf-GBUF;i<wa->wsbuf;++i)
                    wa->wln[i]=0;
            }
            couc=0;
            bwbuf=WBUF;
            bufword=realloc(bufword, bwbuf*sizeof(char)); /* don't bother with memset, it's not necessary */
            bufword[couc++]=c; /* no need to check here, it's the first character */
            inword=1;
        } else if( (c == 0x2E) | ((c >= 0x30) && (c <= 0x39)) ) {
            if(couc == bwbuf-1) { /* the -1 so that we can always add and extra (say 0) when we want */
                bwbuf += WBUF;
                bufword = realloc(bufword, bwbuf*sizeof(char));
            }
            bufword[couc++]=c;
        } else {
            printf("Error. Non-float character detected. This program is only for reading floats\n"); 
            free_wseq(wa);
            exit(EXIT_FAILURE);
        }

    } /* end of big for statement */
    fclose(fp);
    free(bufword);

    /* normalization stage */
    wa->quan=couw;
    wa->wln = realloc(wa->wln, wa->quan*sizeof(size_t)); /* normalize */
    mat = realloc(mat, wa->quan*sizeof(double)); /* normalize */
    wa->wpla= realloc(wa->wpla, wa->numl*sizeof(size_t));

    *m= wa->numl;
    int k=wa->wpla[0];
    for(i=1;i<wa->numl;++i)
        if(k != wa->wpla[i])
            printf("Warning: Numcols is not uniform at %i words per line on all lines. This file has one with %zu.\n", k, wa->wpla[i]); 
    *n= k; 
    free_wseq(wa);

    return mat;
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

int hdrchkbasic(wh_t *inhdr)
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

    return 0;
}

int main(int argc, char *argv[])
{
    if(argc != 3) {
        printf("Usage: divides wav file according to an mplayer- generated EDL file.\n");
        printf("2 arguments: 1) Name of wavfile. 2) name of edl-file.\n");
        exit(EXIT_FAILURE);
    }
    int i, j, k, nr, nc;
    /* we expect som pretty big wavs so we can't rely on byid and gleni:
     * let's use stat.h to work them out instead */
    struct stat fsta;
    if(stat(argv[1], &fsta) == -1) {
        fprintf(stderr,"Can't open input file %s", argv[2]);
        exit(EXIT_FAILURE);
    }
    size_t statglen=fsta.st_size-8;
    size_t statbyid=statglen-36;
    /* open our wav file: we get the header in early that way */
    FILE *inwavfp;
    inwavfp = fopen(argv[1],"rb");
    /* of course we also need the header for other bits of data */
    wh_t *inhdr=malloc(sizeof(wh_t));
    if ( fread(inhdr, sizeof(wh_t), sizeof(char), inwavfp) < 1 ) {
        printf("Can't read file header\n");
        exit(EXIT_FAILURE);
    }
    if (hdrchkbasic(inhdr)) {
        printf("Header failed some basic WAV/RIFF file checks.\n");
        exit(EXIT_FAILURE);
    }

    /* Read in the edl file: note that the third value is irrelevant for us,
     * it's some sort of marker mplayer puts in */
    double *mat=processinpf(argv[2], &nr, &nc);
    int divby3=(nr*nc)%3;
    if(divby3) {
        printf("Error: the EDL file is not a multiple of 3. Bailing out.\n");
        exit(EXIT_FAILURE);
    }
#ifdef DBG
    printf("nr: %d nc: %d\n", nr, nc); 
#endif
    int sampasz=nr*(nc-1); /* we get rid of one colum: mne: samp-pt array size */
    size_t *sampa=malloc(sampasz*sizeof(size_t));
    k=0;
    for(i=0;i<nr;++i) 
        for(j=0;j<nc;++j) {
            if(!((nc*i+j+1)%3)) /* no remainder after div by 3? we don't want it */
                continue;
#ifdef DBG
          printf("%d ", (nc*i+j+1)%3); 
#endif
            sampa[k++]=(size_t)(.5 + ((double)inhdr->sampfq)*mat[nc*i+j]);
        }
    free(mat); /* we've rendered the edl matrix into a integers now, as in sampa */
#ifdef DBG
    printf("\n"); 
#endif

    int chunkquan=nr*(nc-1)/2;

#ifdef DBG
    printf("Chunkquan: %d\n", chunkquan); 
#endif
    char *tmpd=mktmpd();
    char *fn=calloc(GBUF, sizeof(char));
    unsigned char *bf=NULL;
    FILE *outwavfp;
    int stai, endi;
    size_t staby;
    int byidmultiplier=inhdr->nchans*inhdr->bipsamp/8;

    for(j=0;j<chunkquan;++j) { /* note we will reuse the inhdr, only changing byid and glen */

        endi=2*j+1; /* end index */
        stai=2*j; /* start index */
        statbyid = (sampa[endi] - sampa[stai])*byidmultiplier;
        bf=realloc(bf, statbyid*sizeof(unsigned char));
        staby=sampa[stai]*byidmultiplier;

        /* here we revert to wav's limitations on max file size */
        inhdr->byid=(int)statbyid; /* overflow a distinct possibility */
        inhdr->glen = inhdr->byid+36;

        sprintf(fn, "%s/%03i.wav", tmpd, j); /* purposely only allow FOR 1000 edits */
        outwavfp= fopen(fn,"wb");

        fwrite(inhdr, sizeof(unsigned char), 44, outwavfp);

        fseek(inwavfp, 44+staby, SEEK_SET); /* originally forgot to skip the 44 bytes of the header! */
        if ( fread(bf, statbyid, sizeof(unsigned char), inwavfp) < 1 ) {
            printf("Sorry, trouble putting input file into array. Overshot maybe?\n"); 
            exit(EXIT_FAILURE);
        }
        fwrite(bf, sizeof(unsigned char), statbyid, outwavfp);
        fclose(outwavfp);
    }
    fclose(inwavfp);
    free(tmpd);
    free(bf);
    free(sampa);
    free(fn);
    free(inhdr);
    return 0;
}
