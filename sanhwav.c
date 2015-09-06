/* 
 * inspiration from http://yannesposito.com/Scratch/en/blog/2010-10-14-Fun-with-wav/ 
 *
 * can be basis of a canonical wav checker */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

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

unsigned hdrchk(wh_t *hd, size_t fsz)
{
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

int main(int argc, char *argv[])
{
    if(argc == 1) {
        printf("Usage: present wavfiles as arguments and this rogram which check how canonical they are\n");
        exit(EXIT_FAILURE);
    }
    printf("Files specified on command-line= %i\n", argc-1);
    int argidx=1;
    struct stat fsta;
    FILE *wavfp;
    wh_t *inhdr=malloc(44);
    int ret;

    do {
        if (stat(argv[argidx], &fsta) == -1) /*  remember there is also lstat: it does not follow symlink */
            fprintf(stderr, "%s\n", strerror(errno));
        else {
            wavfp = fopen(argv[argidx],"r+b");
            if ( wavfp == NULL ) {
                fprintf(stderr,"Can't open input file %s", argv[argidx]);
                continue;
                exit(EXIT_FAILURE);
            }
            if ( fread(inhdr, sizeof(wh_t), sizeof(char), wavfp) < 1 ) {
                printf("Can't read file header\n");
                exit(EXIT_FAILURE);
            }
            ret=hdrchk(inhdr, fsta.st_size);
            if(ret) {
                if(ret&8)
                    corrhdda(inhdr);
                if(ret&16)
                    corrhdfmt(inhdr);
                rewind(wavfp);
                fwrite(inhdr, sizeof(char), 44, wavfp);
            }
        }
        fclose(wavfp);
    } while (argidx++ < argc-2);

    free(inhdr);
    return 0;
}
