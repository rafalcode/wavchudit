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

int hdrchk(wh_t *inhdr, size_t fsz)
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
        printf("WARNING: fmtnum is %i, not 16, this may be a compressed file, despite being wav\n", inhdr->fmtnum); 
        return 1;
    }
    if ( inhdr->pcmnum != 1 ) {
        printf("WARNING: pcmnum is %i, while it's better off being %i\n", inhdr->pcmnum, 1); 
        return 1;
    }

    printf("Header first tests fine: id, fstr, datastr, fmtnum and pcmnum all fine\n");
    printf("Further header field: "); 
    printf("glen: %i ", inhdr->glen);
    printf("byid: %i ", inhdr->byid);
    printf("nchans: %d ", inhdr->nchans);
    printf("sampfq: %i ", inhdr->sampfq);
    printf("byps: %i ", inhdr->byps);
    printf("bypc: %d ", inhdr->bypc);
    printf("bipsamp: %d\n", inhdr->bipsamp);

    if(inhdr->glen+8-44 == inhdr->byid)
        printf("Checked:, \"byid\" (%i) is 36 bytes smaller than \"glen\" (%i).\n", inhdr->byid, inhdr->glen);
    else {
        printf("WARNING: glen (%i) and byid (%i)do not show prerequisite normal relation(diff is %i).\n", inhdr->glen, inhdr->byid, inhdr->glen-inhdr->byid); 
    }
    // printf("Duration by glen is: %f\n", (float)(inhdr->glen+8-wh_tsz)/(inhdr->nchans*inhdr->sampfq*inhdr->byps));
    printf("Duration by byps is: %f\n", (float)(inhdr->glen+8-44)/inhdr->byps);

    if( (inhdr->bypc == inhdr->byps/inhdr->sampfq) && (inhdr->bypc == 2) )
        printf("bypc complies with being 2 and matching byps/sampfq. Data values can therefore be recorded as signed shorts.\n"); 

    if(inhdr->byid+44 == fsz)
        printf("Checked: header's byid is 44 less than stat.h's reading of file size (%zu).\n", fsz);
    else {
        printf("WARNING: byid and file's size do not differ by 44 but by is %li.\n", (long)(fsz - (size_t)inhdr->byid));
    }

    printf("All good. Playing time in secs %f\n", (float)inhdr->byid/inhdr->byps);

    return 0;
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

    do {
        if (stat(argv[argidx], &fsta) == -1) /*  remember there is also lstat: it does not follow symlink */
            fprintf(stderr, "%s\n", strerror(errno));
        else {
            wavfp = fopen(argv[argidx],"rb");
            if ( wavfp == NULL ) {
                fprintf(stderr,"Can't open input file %s", argv[argidx]);
                continue;
                exit(EXIT_FAILURE);
            }
            if ( fread(inhdr, sizeof(wh_t), sizeof(char), wavfp) < 1 ) {
                printf("Can't read file header\n");
                exit(EXIT_FAILURE);
            }
            hdrchk(inhdr, fsta.st_size);
        }
        fclose(wavfp);
    } while (argidx++ < argc-2);

    free(inhdr);
    return 0;
}
