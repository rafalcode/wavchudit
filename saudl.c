/* Takes a wav file and an mplayer-generated edl file and extracts the chunks */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h> 
#include <libmp3splt/mp3splt.h>

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

static void print_confirmation_and_exit_if_error(splt_state *state, splt_code error) //Callback function that handles error code from libmp3splt.
{
  char *message = mp3splt_get_strerror(state, error);
  if (!message)
    return;

  if (error < 0) {
    fprintf(stderr, "%s\n", message);
    fflush(stderr);
    mp3splt_free_state(state);
    exit(1);
  } else {
    fprintf(stdout, "%s\n", message);
    fflush(stdout);
  }
  free(message);
}

static void print_message_from_library(const char *message, splt_message_type type, void *data) //Callback function printing any messages from libmp3splt.
{
  if (type == SPLT_MESSAGE_INFO) {
    fprintf(stdout, message);
    fflush(stdout);
    return;
  }
  fprintf(stderr, message);
  fflush(stderr);
}

static void print_split_filename(const char *filename, void *data) //Callback function printing the created filenames.
{
  fprintf(stdout, "   %s created.\n", filename);
  fflush(stdout);
}

long *processinpf(char *fname, int *m, int *n)
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

    long *mat=malloc(GBUF*sizeof(long));

    while( (c=fgetc(fp)) != EOF) {
        /*  take care of  */
        if( (c== '\n') | (c == ' ') | (c == '\t') | (c=='#')) {
            if( inword==1) { /* we end a word */
                wa->wln[couw]=couc;
                bufword[couc++]='\0';
                bufword = realloc(bufword, couc*sizeof(char)); /* normalize */
                mat[couw]=atol(bufword);
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
                mat=realloc(mat, wa->wsbuf*sizeof(long));
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
    mat = realloc(mat, wa->quan*sizeof(long)); /* normalize */
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
        printf("Usage: reads an edl file and outputs in several formats.\n");
        printf("1st argument: {mp3,ogg,flac} file 2) name of edl-file.\n");
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
#ifdef DBG
    printf("tot filesize=%zu\n", fsta.st_size); 
#endif

    size_t totsamps=(tstatbyid/inhdr->nchans)/(inhdr->bipsamp/8);

    /* Read in the edl file: note that the third value is irrelevant for us,
     * it's some sort of marker mplayer puts in */
    long *mat=processinpf(argv[1], &nr, &nc);
    int divby3=(nr*nc)%3;
    if(divby3) {
        printf("Error: the EDL file is not a multiple of 3. Bailing out.\n");
        exit(EXIT_FAILURE);
    }
#ifdef DBG2
    printf("nr: %d nc: %d\n", nr, nc); 
#endif
    long *sampa=malloc(nr*(nc-1), sizeof(long));
    k=0;
    for(i=0;i<nr;++i) 
        for(j=0;j<nc;++j) {
            if(!((nc*i+j+1)%3)) /* no remainder after div by 3? we don't want it */
                continue;
#ifdef DBG2
          printf("%d ", (nc*i+j+1)%3); 
#endif
            sampa[k++]=mat[nc*i+j];
        }
    free(mat); /* we've rendered the edl matrix into a integers now, as in sampa */
#ifdef DBG
    printf("\n"); 
#endif

    int chunkquan=nr*(nc-1)+1; /* include pre-first edlstartpt, post-last edlendpt, and edlstart and edlend interstitials. */

    char *tmpd=mktmpd();
    char *fn=calloc(GBUF, sizeof(char));
    unsigned char *bf=NULL;
    FILE *outwavfp;
    size_t frompt, topt, staby, cstatbyid /* current statbyid */;

    for(j=0;j<chunkquan;++j) { /* note we will reuse the inhdr, only changing byid and glen */

        frompt = (j==0)? 0: sampa[j-1];
        // totsampes need to get the length
        topt = (j==chunkquan-1)? totsamps: sampa[j]; /* initially had cstatbyid instead of totsamps ... kept obvershooting file */
        cstatbyid = (topt - frompt)*byidmultiplier;
        bf=realloc(bf, cstatbyid*sizeof(unsigned char));
        staby=frompt*byidmultiplier; /* starting byte */
#ifdef DBG
        printf("j: %d frompt: %zu, topt: %zu, cstatbyid: %zu, staby: %zu ", j, frompt, topt, cstatbyid, staby);
#endif

        /* here we revert to wav's limitations on max file size */
        inhdr->byid=(int)cstatbyid; /* overflow a distinct possibility */
        inhdr->glen = inhdr->byid+36;

        sprintf(fn, "%s/%03i.wav", tmpd, j); /* purposely only allow FOR 1000 edits */
        outwavfp= fopen(fn,"wb");

        fwrite(inhdr, sizeof(unsigned char), 44, outwavfp);

        fseek(inwavfp, 44+staby, SEEK_SET); /* originally forgot to skip the 44 bytes of the header! */
#ifdef DBG
        printf("ftell: %li ftell+cstatbyid: %li\n", ftell(inwavfp), cstatbyid+ftell(inwavfp)); 

#endif
        if ( fread(bf, cstatbyid, sizeof(unsigned char), inwavfp) < 1 ) {
            printf("Sorry, trouble putting input file into array. Overshot maybe?\n"); 
            exit(EXIT_FAILURE);
        }
        fwrite(bf, sizeof(unsigned char), cstatbyid, outwavfp);
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
