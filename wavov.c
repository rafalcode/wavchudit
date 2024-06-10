/* 
 * wavov - how can we maniuplate a wav file by changing stuff in its header
 */

#include <stdio.h>
#include<unistd.h> // required for optopt, opterr and optarg.
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>

typedef struct  /* optstruct_raw, a struct for the options */
{
    char *bval, *fval; /* sval, sound freq i.e. 440; bit per sample, 8 16 or 32, never over 255 and then f is the ampling frequency 44100 or 48000 or whatever you like */
    char *istr; /* output filename prefix */
} optstruct_raw;

typedef struct  /* opts_true: a structure for the options reflecting their true types */
{
    short b;
    int f;
    char *infn; /* output filename */
} opts_true;

opts_true *processopts(optstruct_raw *rawopts) /* this is where the rawopts are converted into the types the program can actually use */
{
    opts_true *trueopts=calloc(1, sizeof(opts_true));

    /* some are easy as in, they are direct tranlsations */
    trueopts->b=(short)atoi(rawopts->bval);
    trueopts->f=atoi(rawopts->fval);
    trueopts->infn=rawopts->istr;

    return trueopts;
}

int catchopts(optstruct_raw *opstru, int oargc, char **oargv)
{
    int c;
    opterr = 0;

    while ((c = getopt (oargc, oargv, ":i:f:b:")) != -1)
        switch (c) {
            case 'i':
                opstru->istr = optarg;
                break;
            case 'b':
                opstru->bval = optarg;
                break;
            case 'f':
                opstru->fval = optarg;
                break;
            case '?':
                /* general error state? */
                if (optopt == 'i')
                    fprintf (stderr, "Option -%c requires an input filename argument.\n", optopt);
                if (optopt == 's')
                    fprintf (stderr, "Option -%c requires a sound frequency, i.e. 440 for A_4.\n", optopt);
                if (optopt == 'b')
                    fprintf (stderr, "Option -%c requires a integer: 8 16 or 32.\n", optopt);
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

typedef struct /* wh_t, the wav file header, always 44 bytes in length. */
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
    int byid; // BYtes_In_Data, should be glen-36;
} wh_t; /* wav header type */

void corrhdda(wh_t *hd) /* forces "data" field in header */
{
    hd->datastr[0] = 'd';
    hd->datastr[1] = 'a';
    hd->datastr[2] = 't';
    hd->datastr[3] = 'a';

    return;
}

void corrhdfmt(wh_t *hd) /* forces "data" field in header */
{
    hd->fmtnum = 16;
    return;
}

unsigned hdrchk(wh_t *hd, size_t fsz)
{
    printf("\n"); 
    unsigned ret=0;
    /* OK .. we test what sort of header we have */
    if ( hd->id[0] != 'R' || hd->id[1] != 'I' || hd->id[2] != 'F' || hd->id[3] != 'F' ) { 
#ifdef DBG
        printf("ERROR: RIFF string problem\n"); 
#endif
        ret |=0x01;
    }

    if ( hd->fstr[0] != 'W' || hd->fstr[1] != 'A' || hd->fstr[2] != 'V' || hd->fstr[3] != 'E' || hd->fstr[4] != 'f' || hd->fstr[5] != 'm' || hd->fstr[6] != 't' || hd->fstr[7] != ' ' ) { 
#ifdef DBG
        printf("ERROR: WAVEfmt string problem\n"); 
#endif
        ret |= 0x02;
    }

    if ( hd->datastr[0] != 'd' || hd->datastr[1] != 'a' || hd->datastr[2] != 't' || hd->datastr[3] != 'a' ) { 
#ifdef DBG
        printf("WARNING: header \"data\" string does not come up\n"); 
#endif
        ret |= 0x04;
    }
    if ( hd->fmtnum != 16 ) { /* a common other value is 18 */
#ifdef DBG
        printf("WARNING: fmtnum is %i, not 16, this may be a compressed file, despite being wav\n", hd->fmtnum); 
#endif
        ret |=0x08;
    }
    if ( hd->pcmnum != 1 ) {
        printf("WARNING: pcmnum is %i, while it's better off being %i\n", hd->pcmnum, 1); 
        ret |=16;
    }

    printf("Header first tests fine: id, fstr, datastr, fmtnum and pcmnum all fine\n");
    printf("Further header fields: "); 
    printf("glen: %i /", hd->glen);
    printf("byid: %i /", hd->byid);
    printf("nchans: %d /", hd->nchans);
    printf("sampfq: %i /", hd->sampfq);
    printf("byps: %i /", hd->byps);
    printf("bypc: %d /", hd->bypc);
    printf("bipsamp: %d\n", hd->bipsamp);

    if(hd->glen+8-44 == hd->byid)
        printf("Checked: \"byid\" (%i) is 36 bytes smaller than \"glen\" (%i).\n", hd->byid, hd->glen);
    else {
        printf("WARNING: glen (%i) and byid (%i)do not show prerequisite normal relation(diff is %i).\n", hd->glen, hd->byid, hd->glen-hd->byid); 
    }
    // printf("Duration by glen is: %f\n", (float)(hd->glen+8-wh_tsz)/(hd->nchans*hd->sampfq*hd->byps));
    printf("Duration by byps is: %f\n", (float)(hd->glen+8-44)/hd->byps);

    if( (hd->bypc == hd->byps/hd->sampfq) && (hd->bypc == 2) )
        printf("bypc complies with being 2 and matching byps/sampfq. Data values can therefore be recorded as signed shorts.\n"); 

    if(hd->byid+44 == fsz)
        printf("Checked: header's byid is 44 less than stat.h's reading of file size (%zu).\n", fsz);
    else {
        printf("WARNING: byid and file's size do not differ by 44 but by is %li.\n", (long)(fsz - (size_t)hd->byid));
    }

    printf("All good. Playing time in secs %f\n", (float)hd->byid/hd->byps);

    return 0;
}

int main(int argc, char *argv[])
{
    if(argc == 1) {
        printf("Usage: \"./wavov file.wav -f 48000 -b 8 -o <outname>\", omit the -f (aka. force) is you do not want the byid field to be overwritten by size info)\n");
        exit(EXIT_FAILURE);
    }
    optstruct_raw rawopts={0};
    catchopts(&rawopts, argc, argv);
    opts_true *trueopts=processopts(&rawopts);

    struct stat fsta;
    FILE *wavfp;
    unsigned whsz=sizeof(wh_t); // of course will always be 44
    wh_t *inhdr=malloc(whsz);
    int ret;

    if (stat(trueopts->infn, &fsta) == -1) { /*  remember there is also lstat: it does not follow symlink */
        printf("Could not stat given file.\n"); 
        fprintf(stderr, "%s\n", strerror(errno));
    } else {
        wavfp = fopen(trueopts->infn,"r+b");
        if ( wavfp == NULL ) {
            fprintf(stderr,"Can't open input file %s", argv[1]);
            exit(EXIT_FAILURE);
        }
        if ( fread(inhdr, sizeof(wh_t), sizeof(char), wavfp) < 1 ) {
            printf("Can't read wav header\n");
            exit(EXIT_FAILURE);
        }
        ret=hdrchk(inhdr, fsta.st_size);
        if(ret) {
            if(ret&8)
                corrhdda(inhdr);
            if(ret&16)
                corrhdfmt(inhdr);
        }
        printf("f=%i\n", trueopts->f);
        if( (inhdr->sampfq != trueopts->f) && trueopts->f) {
        // if(inhdr->sampfq != trueopts->f) {
            inhdr->sampfq = trueopts->f;
            printf("sampfq changed\n"); 
        }
        if((inhdr->bipsamp != trueopts->b) && trueopts->b) // b must not be zero
            inhdr->bipsamp = trueopts->b;
        rewind(wavfp);
        fwrite(inhdr, sizeof(char), whsz, wavfp);
    }
    fclose(wavfp);

    free(inhdr);
    return 0;
}
