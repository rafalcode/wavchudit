/*  lopwav. the idea is to lop off either the start or the end of the file. All from the command line */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>

#define ONSZ 32 // otuput filename size.

typedef struct /* wavheader type: wh_t */
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
    int byid; // BYtes_In_Data (ection), sorry to those who interpret "by ID" no sense in this context */
} wh_t; /* wav header type */

char *mkon(char *inpfn)  /* make output filename ... some convenient */
{
    char *on=calloc(ONSZ, sizeof(char));

    struct timeval tsecs;
    gettimeofday(&tsecs, NULL);
    char lsns[7]={0}; // micseconds
    sprintf(lsns, "%lu", tsecs.tv_usec);
    sprintf(on, "t%.*s", 3, lsns); 

    char *per=strrchr(inpfn, '.');
    sprintf(on+4, "_%.*s.wav", 3, per-3); // 5 chars

    /* let's avoid overwriting same named file */
    struct stat fsta;
    while(stat(on, &fsta) != -1) {
        sprintf(lsns, "%lu", tsecs.tv_usec+1UL);
        sprintf(on+1, "%.*s", 3, lsns); 
        sprintf(on+4, "_%.*s.wav", 3, per-3); // 5 chars
    }

    return on;
}

int main(int argc, char *argv[])
{
    if(argc == 1) {
        printf("Usage: this program takes a wav file and renders it into raw.\n");
        exit(EXIT_FAILURE);
    }

    /* The first check is to see if both these files are compatible. I.e. must have same sample frequencies and the like.
     * I was goign to check to see which was the smallest, but actually right now, we're going to pull them both into memory */
    int i;
    FILE *inrawfp;

    /* first wav file dealt with separately */
    /* Before opening, let's use stat on the wav file */
    struct stat fsta;
    if(stat(argv[1], &fsta) == -1) {
        fprintf(stderr,"Can't stat input file %s", argv[1]);
        exit(EXIT_FAILURE);
    }

    inrawfp = fopen(argv[1],"rb");
    if ( inrawfp == NULL ) {
        fprintf(stderr,"Can't open input file %s", argv[1]);
        exit(EXIT_FAILURE);
    }
    if ( fread(inhdr1, sizeof(wh_t), sizeof(char), inrawfp) < 1 ) {
        printf("Can't read file header\n");
        exit(EXIT_FAILURE);
    }
    if(inhdr1->byid != statbyid) {
        printf("header and stat conflicting on size of data payload. Pls revise.\n"); 
        exit(EXIT_FAILURE);
    }

    /* decide an output file name */
    char *outfn=mkon(argv[1]);
    FILE *outwavfp= fopen(outfn,"wb");
    char *bf=malloc(inhdr1->byid);
    if ( fread(bf, inhdr1->byid, sizeof(char), inrawfp) < 1 ) { /* Yup! we slurp in the baby! */
        printf("Sorry, trouble putting input file into array\n"); 
        exit(EXIT_FAILURE);
    }
    /* close input file, it's in our buffer now */
    fclose(inrawfp);
    /* but we write out to outfile almost immediately */
    fwrite(bf, sizeof(char), inhdr1->byid, outwavfp);
    fclose(outwavfp);
    /* and now we don;t need the buffer */
    wh_t *inhdr1=malloc(sizeof(wh_t));
    size_t statglen=fsta.st_size-8; //filesz less 8
    size_t statbyid=statglen-36; // filesz less 8+36, so that's less the wav header
    free(bf);

    free(inhdr1);
    free(outfn);

    return 0;
}
