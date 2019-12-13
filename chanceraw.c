/*  lopwav. the idea is to lop off either the start or the end of the file. All from the command line */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h> 

#define ONSZ 32 // output filename size.
#define GBUF 128
#define nr 3
#define nc 2
#define nb 3

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

char *mktmpd(void)
{
    struct timeval tsecs;
    gettimeofday(&tsecs, NULL);
    char *myt=calloc(14, sizeof(char));
    strncpy(myt, "tmpdir_", 7);
    sprintf(myt+7, "%lu", tsecs.tv_usec);

    DIR *d;
    while((d = opendir(myt)) != NULL) { /* NULL is returned if a directory with this name does not exist. That is waht we want, we do not want to clobber any existing directory */
        gettimeofday(&tsecs, NULL);
        sprintf(myt+7, "%lu", tsecs.tv_usec);
        closedir(d);
    }
    // closedir(d); // this causing errors, because a returned NULL is a failure ... no directory will have been opened at this point, as it needs to have existed.
    mkdir(myt, S_IRWXU);

    return myt;
}

int main(int argc, char *argv[])
{
    if(argc == 1) {
        printf("Usage: this program takes a wav file and renders it into raw.\n");
        exit(EXIT_FAILURE);
    }

    /* The first check is to see if both these files are compatible. I.e. must have same sample frequencies and the like.
     * I was goign to check to see which was the smallest, but actually right now, we're going to pull them both into memory */
    int i, j, k;
    FILE *inrawfp;

    /* Before opening, let's use stat on the wav file */
    struct stat fsta;
    if(stat(argv[1], &fsta) == -1) {
        fprintf(stderr,"Can't stat input file %s", argv[1]);
        exit(EXIT_FAILURE);
    }
    size_t whsz=sizeof(wh_t);
    size_t trsz=sizeof(char); // transfer size
    printf("size of raw file is: %zu\n", fsta.st_size);
    char *buf=malloc(fsta.st_size);
    inrawfp = fopen(argv[1], "rb");
    if ( inrawfp == NULL ) {
        fprintf(stderr,"Can't open input file %s", argv[1]);
        exit(EXIT_FAILURE);
    }
    if(fread(buf, fsta.st_size, trsz, inrawfp) < 1) {
        printf("Sorry, trouble putting input file into buffer.\n"); 
        exit(EXIT_FAILURE);
    }
    fclose(inrawfp);

    wh_t *hd=calloc(1, whsz);
    memcpy(hd->fstr, "WAVEfmt ", 8*sizeof(char));
    memcpy(hd->id, "RIFF", 4*sizeof(char));
    memcpy(hd->datastr, "data", 4*sizeof(char));
    hd->fmtnum = 16;
    hd->pcmnum = 1;
    hd->bipsamp = 16;
    hd->bypc = hd->bipsamp/8;
    hd->byid=fsta.st_size;
    hd->glen = hd->byid+36;

    int r[nr]={44100, 48000, 22050};
    int c[nc]={1, 2};
    short b[nb]={16, 24, 32};

    char *tmpd=mktmpd();
    char *fn=calloc(GBUF, sizeof(char));
    FILE *outwavfp;

    for(i=0;i<nr;++i) {
        for(j=0;j<nc;++j) {
            for(k=0;k<nb;++k) {
                hd->sampfq = r[i];
                hd->nchans = c[j];
                hd->bipsamp = b[k];
                sprintf(fn, "%s/r%i_c%i_b%i.wav", tmpd, r[i], c[j], b[k]);
                outwavfp = fopen(fn,"wb");
                fwrite(hd, trsz, whsz, outwavfp);
                fwrite(buf, trsz, fsta.st_size, outwavfp);
                fclose(outwavfp);
                printf("\"%s\" %d %d %d\n", hd->datastr, hd->nchans, hd->sampfq, hd->bipsamp);
            }
        }
    }
    printf("INFO: various trial wav's to be put into folder %s.\n", tmpd);

    free(buf);
    free(hd);
    free(tmpd);
    free(fn);
    return 0;
}
