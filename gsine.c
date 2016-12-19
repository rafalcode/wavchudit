/* gsine.c generate a sine wav, various options available. */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h> 
#include<ctype.h> // required for isprint
#include<unistd.h> // required for optopt, opterr and optarg.
#include<math.h>

#define SFREQ 440 // sound frequency in Hz

#define ARBSZ 128
#define GBUF 64

typedef struct  /* optstruct_raw, a struct for the options */
{
    unsigned char mflg; /* mono flag, if not set, stereo */
    char *bval, *fval; /* bit per sample, 8 16 or 32, never over 255 and then f is the ampling frequency 44100 or 48000 or whatever you like */
    char *ostr; /* output filename prefix */
    char *tstr; /* itme spec mm:ss.hh */
} optstruct_raw;

typedef struct  /* opts_true: a structure for the options reflecting their true types */
{
    short c; /* numbr of chans derived from m flag */
    short b;
    int f, mm, ss, hh;
    char *outfn; /* output filename */
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

wh_t *hdr4chunk(int sfre, short b, short nucha, int certainsz) /* a header for a file chunk of certain siez */
{
    wh_t *wh=malloc(sizeof(wh_t));
    memcpy(wh->id, "RIFF", 4*sizeof(char));
    memcpy(wh->fstr, "WAVEfmt ", 8*sizeof(char));
    memcpy(wh->datastr, "data", 4*sizeof(char));
    wh->fmtnum=16;
    wh->pcmnum=1;
    wh->nchans=nucha; /* fed in to function */
    wh->sampfq=sfre; /* fed in to function */
    wh->bipsamp=b;
    wh->bypc = wh->bipsamp/8;
    wh->byid = certainsz * wh->bypc * wh->nchans; /* this decides the true byte payload */
    wh->glen = wh->byid + 36;
    wh->byps = wh->nchans*wh->sampfq*wh->bypc;
    return wh;
}

opts_true *processopts(optstruct_raw *rawopts) /* this is where the rawopts are converted into the types the program can actually use */
{
    opts_true *trueopts=calloc(1, sizeof(opts_true));

    /* some are easy as in, they are direct tranlsations */
    trueopts->c=(rawopts->mflg)? 1 : 2;
    trueopts->b=(short)atoi(rawopts->bval);
    trueopts->f=atoi(rawopts->fval);

    if(rawopts->ostr) {
        trueopts->outfn=malloc((5+strlen(rawopts->ostr))*sizeof(char));
        strcpy(trueopts->outfn, rawopts->ostr);
        strcat(trueopts->outfn, ".wav");
        printf("The input wav filename option was defined and is set to \"%s\".\n", trueopts->outfn);
    } else {
        printf("A filename after the -i option is obligatory: it's the input wav filename option.\n");
        exit(EXIT_FAILURE);
    }

    /* Now for the time value */
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

    while ((c = getopt (oargc, oargv, "mb:f:o:t:")) != -1)
        switch (c) {
            case 'm':
                opstru->mflg = 1;
                break;
            case 'b':
                opstru->bval = optarg;
                break;
            case 'f':
                opstru->fval = optarg;
                break;
            case 'o':
                opstru->ostr = optarg;
                break;
            case 't':
                opstru->tstr = optarg;
                break;
            case '?':
                /* general error state? */
                if (optopt == 'o')
                    fprintf (stderr, "Option -%c requires an output filename prefix argument.\n", optopt);
                if (optopt == 't')
                    fprintf (stderr, "Option -%c requires an time spec (mm:ss.h).\n", optopt);
                if (optopt == 'b')
                    fprintf (stderr, "Option -%c requires a integer: 8 16 or 32.\n", optopt);
                if (optopt == 'f')
                    fprintf (stderr, "Option -%c requires a integer: most prob 44100, but can also be 48000 or even any other.\n", optopt);
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
    printf("This program generates a sine wave of 400Hz, with the following options:\n");
    printf("\t-m a flag, if specified wav will be in mono, if no, it will be stereo\n");
    printf("\t-t time spec (mm:ss.hh)\n");
    printf("\t-o <string>, this should be the prefix of the output file you want. The wav extension will be appended\n");
    printf("\t-f <integer>, this should be sampling frequency required, most probably 44100, or 48000 or whatever you want.\n");
    printf("\t-b <integer>, this should be the bit size required, most often 8, 16 or 32\n");
    printf("\teg.\n"); 
    printf("\t\t./gsine -o uit -t 0:0.50 -b 16 -f 44100\n"); 
    printf("\tor:\n"); 
    printf("\t\t./gsine -b 32 -o ui -f 44100 -t 4.0\n"); 
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

    /* create header:
     * with the time and the sampling frequency we can calculate how many actual samples we're going to need. They could be 1 2 4 byte or 1 or 2 channels though */
    int certainsz = 60*trueopts->mm*trueopts->f + trueopts->ss*trueopts->f + (int)(0.5+trueopts->hh*trueopts->f/100.);
    wh_t *whd=hdr4chunk(trueopts->f, trueopts->b, trueopts->c, certainsz);/* a header for a file chunk of certain siez */
#ifdef DBG
    printf("csz:%d byid:%d glen:%d\n", certainsz, whd->byid, whd->glen);
#endif
    FILE *outwavfp=fopen(trueopts->outfn, "wb");
    fwrite(whd, sizeof(char), 44, outwavfp);
    unsigned char *dpay=malloc(whd->byid*sizeof(unsigned char)); /* data payload */
    float cyclesrads=SFREQ*2*M_PI/whd->sampfq; /* number of cycles per second in radians */
    int i, j, k, knchans, bypcik;
    int sval /* soundvalue */, mxlev=(1<<(whd->bipsamp-1)) -1 /* this will be 0x7FFF is bipsamp is 16 */;
    float fourdpi=4/M_PI; /* Four divided by pi */
    float onedpi=1/M_PI; /* one divided by pi */

    /* Following loop looks a bit hairy, but it's fine */
    for (k = 0; k < certainsz ; k++) {
        sval = (int)(0.5+mxlev * sin (cyclesrads*k)); /* cyclesradsyou can feed large numbers into sine, no need to operate on values just in btwn 0 and 2PI */
        // sval = (int)(0.5+mxlev * fourdpi*(sin(cyclesrads*k)+(sin(3*cyclesrads*k))/3.)); /* 1st and 2nd term exp of Fourier series of square wave */
        // sval = (int)(0.5+mxlev * (0.5 + onedpi*(sin(cyclesrads*k)-(sin(2*cyclesrads*k))/2.))); /* 1st and 2nd term exp of Fourier series of sawtooth wave */
        knchans=k*whd->nchans;
        for(i=0;i<whd->nchans;++i) {
            bypcik=(knchans+i)*whd->bypc;
            for(j=0;j<whd->bypc;++j) {
#ifdef DBG2
                printf("%d ", bypcik+j);
#endif
                dpay[bypcik+j] = ((sval>>(j*8))&0xFF); /* exclusively small endian, LSB's come out first */
            }
        }
    }
#ifdef DBG2
    printf("\n");
#endif

    fwrite(dpay, sizeof(char), whd->byid, outwavfp);
    fclose(outwavfp);

    free(dpay);
    free(trueopts->outfn);
    free(trueopts);
    free(whd);
    return 0;
}
