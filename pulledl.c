#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include<unistd.h>
#include<sys/time.h>
#include<sys/stat.h>
#include<dirent.h> 
#include "genread.h"

#define GBUF 64
#define WBUF 8

typedef struct  /* optstruct, a struct for the options */
{
    int aflg, bflg;
    char *edlfn;
} optstruct;

typedef struct /* wh_t: WAV header type */
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

void prtusage(void)
{
        printf("Usage: divides wav file according to an mplayer-generated EDL file.\n");
        printf("\tCan run with only one argument: Name of wavfile. The edl file will be assumed to have\n");
        printf("\tto have the same basename, but with the \"edl\" extension.\n");
        printf("\tDefault usage: segments before first time point and after last timepoint: omitted. Interstitial\n");
        printf("\tsegments output as files. If \"-a\" switch follows wav filename, segment before first timepoint\n");
        printf("\twill be output. If \"-b\" is the switch, segment after last time point is also output. These two\n");
        printf("\tcan be used together\n"); 
        printf("\tFinally, the \"-e\" switch accepts the name of an edl file\n"); 
        return;
}

int catchopts(optstruct *opstru, int oargc, char **oargv)
{
    int c;
    opterr = 0;

    while ((c = getopt (oargc, oargv, "abe:")) != -1)
        switch (c) {
            case 'a':
                opstru->aflg = 1;
                break;
            case 'b':
                opstru->bflg = 1;
                break;
            case 'e':
                opstru->edlfn = optarg;
                break;
            case '?':
                if (optopt == 'e')
                    fprintf (stderr, "Option -%c requires an argument (name of edl file).\n", optopt);
                else if (isprint (optopt))
                    fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
                return 1;
            default:
                abort();
        }

    return 0;
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

w_c *crea_wc(unsigned initsz)
{
    w_c *wc=malloc(sizeof(w_c));
    wc->lp1=initsz;
    wc->t=STR;
    wc->w=malloc(wc->lp1*sizeof(char));
    return wc;
}

void reall_wc(w_c **wc, unsigned *cbuf)
{
    w_c *twc=*wc;
    unsigned tcbuf=*cbuf;
    tcbuf += CBUF;
    twc->lp1=tcbuf;
    twc->w=realloc(twc->w, tcbuf*sizeof(char));
    *wc=twc; /* realloc can often change the ptr */
    *cbuf=tcbuf;
    return;
}

void norm_wc(w_c **wc)
{
    w_c *twc=*wc;
    twc->w=realloc(twc->w, twc->lp1*sizeof(char));
    *wc=twc; /* realloc can often change the ptr */
    return;
}

void free_wc(w_c **wc)
{
    w_c *twc=*wc;
    free(twc->w);
    free(twc);
    return;
}

aw_c *crea_awc(unsigned initsz)
{
    int i;
    aw_c *awc=malloc(sizeof(aw_c));
    awc->ab=initsz;
    awc->al=awc->ab;
    awc->aw=malloc(awc->ab*sizeof(w_c*));
    for(i=0;i<awc->ab;++i) 
        awc->aw[i]=crea_wc(CBUF);
    return awc;
}

void reall_awc(aw_c **awc, unsigned buf)
{
    int i;
    aw_c *tawc=*awc;
    tawc->ab += buf;
    tawc->al=tawc->ab;
    tawc->aw=realloc(tawc->aw, tawc->ab*sizeof(aw_c*));
    for(i=tawc->ab-buf;i<tawc->ab;++i)
        tawc->aw[i]=crea_wc(CBUF);
    *awc=tawc;
    return;
}

void norm_awc(aw_c **awc)
{
    int i;
    aw_c *tawc=*awc;
    /* free the individual w_c's */
    for(i=tawc->al;i<tawc->ab;++i) 
        free_wc(tawc->aw+i);
    /* now release the pointers to those freed w_c's */
    tawc->aw=realloc(tawc->aw, tawc->al*sizeof(aw_c*));
    *awc=tawc;
    return;
}

void free_awc(aw_c **awc)
{
    int i;
    aw_c *tawc=*awc;
    for(i=0;i<tawc->al;++i) 
        free_wc(tawc->aw+i);
    free(tawc->aw); /* unbelieveable: I left this out, couldn't find where I leaking the memory! */
    free(tawc);
    return;
}

aaw_c *crea_aawc(unsigned initsz)
{
    int i;
    unsigned lbuf=initsz;
    aaw_c *aawc=malloc(sizeof(aaw_c));
    aawc->numl=0;
    aawc->aaw=malloc(lbuf*sizeof(aw_c*));
    for(i=0;i<initsz;++i) 
        aawc->aaw[i]=crea_awc(WABUF);
    return aawc;
}

void free_aawc(aaw_c **aw)
{
    int i;
    aaw_c *taw=*aw;
    for(i=0;i<taw->numl;++i) /* tried to release 1 more, no go */
        free_awc(taw->aaw+i);
    free(taw->aaw);
    free(taw);
}

float *edlf2arr(aaw_c *aawc) /* convert the floats in an edl to an array */
{
    int i, j, k;
    float *a=malloc(aawc->numl*2*sizeof(float));
    for(i=0;i<aawc->numl;++i)
        if(aawc->aaw[i]->al%3) {
            printf("Error: not a familiar EDL file as its lines anot a multiple of 3. Bailing out.\n");
            exit(EXIT_FAILURE);
        }
    k=0;
    for(i=0;i<aawc->numl;++i) {
        for(j=0;j<aawc->aaw[i]->al-1;++j) {
            if(!((3*i+j+1)%3)) /* no remainder after div by 3? we don't want it */
                continue;
            a[k++]= atof(aawc->aaw[i]->aw[j]->w);
        }
    }
    return a;
}

void prtaawcdbg(aaw_c *aawc)
{
    int i, j, k;
    for(i=0;i<aawc->numl;++i) {
        printf("l.%u(%u): ", i, aawc->aaw[i]->al); 
        for(j=0;j<aawc->aaw[i]->al;++j) {
            printf("w_%u: ", j); 
            if(aawc->aaw[i]->aw[j]->t == NUM) {
                printf("NUM! "); 
                continue;
            } else if(aawc->aaw[i]->aw[j]->t == PNI) {
                printf("PNI! "); 
                continue;
            }
            for(k=0;k<aawc->aaw[i]->aw[j]->lp1-1; k++)
                putchar(aawc->aaw[i]->aw[j]->w[k]);
            printf("/%u ", aawc->aaw[i]->aw[j]->lp1-1); 
        }
        printf("\n"); 
    }
}

void prtaawcdata(aaw_c *aawc) /* print line and word details, but not the words themselves */
{
    int i, j;
    for(i=0;i<aawc->numl;++i) {
        printf("L%u(%uw):", i, aawc->aaw[i]->al); 
        for(j=0;j<aawc->aaw[i]->al;++j) {
            printf("l%ut", aawc->aaw[i]->aw[j]->lp1-1);
            switch(aawc->aaw[i]->aw[j]->t) {
                case NUM: printf("N "); break;
                case PNI: printf("I "); break;
                case STR: printf("S "); break;
            }
        }
    }
    printf("\n"); 
}

aaw_c *processinpf(char *fname)
{
    /* declarations */
    FILE *fp=fopen(fname,"r");
    int i;
    size_t couc /*count chars per line */, couw=0 /* count words */;
    int c;
    boole inword=0;
    unsigned lbuf=LBUF /* buffer for number of lines */, cbuf=CBUF /* char buffer for size of w_c's: reused for every word */;
    aaw_c *aawc=crea_aawc(lbuf); /* array of words per line */

    while( (c=fgetc(fp)) != EOF) {
        if( (c== '\n') | (c == ' ') | (c == '\t') ) {
            if( inword==1) { /* cue word -edning procedure */
                aawc->aaw[aawc->numl]->aw[couw]->w[couc++]='\0';
                aawc->aaw[aawc->numl]->aw[couw]->lp1=couc;
                norm_wc(aawc->aaw[aawc->numl]->aw+couw);
                couw++; /* verified: this has to be here */
            }
            if(c=='\n') { /* cue line-ending procedure */
                if(aawc->numl ==lbuf-1) {
                    lbuf += LBUF;
                    aawc->aaw=realloc(aawc->aaw, lbuf*sizeof(aw_c*));
                    for(i=lbuf-LBUF; i<lbuf; ++i)
                        aawc->aaw[i]=crea_awc(WABUF);
                }
                aawc->aaw[aawc->numl]->al=couw;
                norm_awc(aawc->aaw+aawc->numl);
                aawc->numl++;
                couw=0;
            }
            inword=0;
        } else if(inword==0) { /* a normal character opens word */
            if(couw ==aawc->aaw[aawc->numl]->ab-1) /* new word opening */
                reall_awc(aawc->aaw+aawc->numl, WABUF);
            couc=0;
            cbuf=CBUF;
            aawc->aaw[aawc->numl]->aw[couw]->w[couc++]=c;
            GETTYPE(c, aawc->aaw[aawc->numl]->aw[couw]->t); /* MACRO: the firt character gives a clue */
            inword=1;
        } else if(inword) { /* simply store */
            if(couc == cbuf-1)
                reall_wc(aawc->aaw[aawc->numl]->aw+couw, &cbuf);
            aawc->aaw[aawc->numl]->aw[couw]->w[couc++]=c;
            /* if word is a candidate for a NUM or PNI (i.e. via its first character), make sure it continues to obey rules: a MACRO */
            MODTYPEIF(c, aawc->aaw[aawc->numl]->aw[couw]->t);
        }
    } /* end of big for statement */
    fclose(fp);

    /* normalization stage */
    for(i=aawc->numl; i<lbuf; ++i) {
        free_awc(aawc->aaw+i);
    }
    aawc->aaw=realloc(aawc->aaw, aawc->numl*sizeof(aw_c*));

    return aawc;
}

int main(int argc, char *argv[])
{
    /* argument accounting */
    if(argc == 1) {
        prtusage();
        exit(EXIT_FAILURE);
    }

    /* options and flags */
    optstruct opstru={0};
    int argignore=1;
    int oargc=argc-argignore;
    char **oargv=argv+argignore;
    catchopts(&opstru, oargc, oargv);

    /* let's use stat.h to work out size of WAV instead of its header info .. it's more reliable */
    struct stat fsta;
    if(stat(argv[1], &fsta) == -1) {
        fprintf(stderr,"Can't open input file %s", argv[2]);
        exit(EXIT_FAILURE);
    }
    size_t statglen=fsta.st_size-8;
    size_t tstatbyid=statglen-36; /* total stabyid, because we'll have a "current" stabyid for the chunks */
    int i, j;

    /* open our wav file: we get the header in early that way */
    FILE *inwavfp;
    inwavfp = fopen(argv[1],"rb");
    /* of course we also need the header for other bits of data */
    wh_t *inhdr=malloc(sizeof(wh_t));
    if ( fread(inhdr, sizeof(wh_t), sizeof(unsigned char), inwavfp) < 1 ) {
        printf("Can't read file header\n");
        exit(EXIT_FAILURE);
    }
    if (hdrchkbasic(inhdr)) {
        printf("Header failed some basic WAV/RIFF file checks.\n");
        exit(EXIT_FAILURE);
    }

    size_t totsamps=0;
    if(opstru.bflg)
        totsamps=(tstatbyid/inhdr->nchans)/(inhdr->bipsamp/8);

    aaw_c *aawc;
    if(opstru.edlfn)
        aawc=processinpf(opstru.edlfn);
    else {
        char edlfn[128]={0};
        char *dptr=strrchr(argv[1], '.');
        strncpy(edlfn, argv[1], dptr-argv[1]);
        strcat(edlfn, ".edl");
        aawc=processinpf(edlfn);
    }

    float *a=edlf2arr(aawc);

    /* convert seconds to sample timepts */
    size_t *sampa=malloc(aawc->numl*2*sizeof(size_t));
    for(i=0;i<aawc->numl*2;++i)
        sampa[i]=(size_t)(.5 + ((float)inhdr->sampfq)*a[i]);
    free(a);

    /* this version: ignore leading and ending chunk */
    int chunkquan;
    if(opstru.aflg && opstru.bflg)
        chunkquan=2*aawc->numl+1;
    else if((opstru.aflg) || (opstru.bflg))
        chunkquan=2*aawc->numl;
    else
        chunkquan=2*aawc->numl-1;

#ifdef DBG
    printf("chunkquan is %d\n", chunkquan); 
#endif
    free_aawc(&aawc);

    char *tmpd=mktmpd();
    char *fn=calloc(GBUF, sizeof(char));
    unsigned char *bf=NULL;
    FILE *outwavfp;
    size_t frompt, topt, staby, cstatbyid /* current statbyid */;
    int byidmultiplier=inhdr->nchans*inhdr->bipsamp/8;

    for(j=0;j<chunkquan;++j) {

        if(opstru.aflg && opstru.bflg)  {
           frompt = (j==0)? 0: sampa[j-1];
           topt = (j==chunkquan-1)? totsamps: sampa[j];
        } else if(opstru.aflg) {
           frompt = (j==0)? 0: sampa[j-1];
           topt = sampa[j];
        } else if(opstru.bflg) {
           frompt = sampa[j];
           topt = (j==chunkquan-1)? totsamps: sampa[j+1];
        } else {
           frompt = sampa[j];
           topt = sampa[j+1];
        }

        cstatbyid = (topt - frompt)*byidmultiplier;
        bf=realloc(bf, cstatbyid*sizeof(unsigned char));
        staby=frompt*byidmultiplier; /* starting byte */

        /* We reuse the hdr for this particular chunk */
        inhdr->byid=(int)cstatbyid;
        inhdr->glen = inhdr->byid+36;

        sprintf(fn, "%s/%03i.wav", tmpd, j); /* no fancy name */
        outwavfp= fopen(fn,"wb");

        fwrite(inhdr, sizeof(unsigned char), 44, outwavfp);

        fseek(inwavfp, 44+staby, SEEK_SET); /* originally forgot to skip the 44 bytes of the header! */

        if ( fread(bf, cstatbyid, sizeof(unsigned char), inwavfp) < 1 ) {
            printf("Sorry, trouble putting input file into array. Overshot maybe?\n"); 
            exit(EXIT_FAILURE);
        }
        fwrite(bf, sizeof(unsigned char), cstatbyid, outwavfp);
        fclose(outwavfp);
    }
    printf("Directory \"%s\" holds the %d extracted files.\n", tmpd, chunkquan); 
    fclose(inwavfp);
    free(tmpd);
    free(bf);
    free(sampa);
    free(fn);
    free(inhdr);

    return 0;
}
