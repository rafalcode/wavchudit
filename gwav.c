/*  lopwav. the idea is to lop off either the start or the end of the file. All from the command line */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>

#define nr 2
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

int main(int argc, char *argv[])
{

    int i, j, k;
    wh_t *hd=calloc(1, sizeof(wh_t));
    memcpy(hd->fstr, "WAVEfmt ", 8*sizeof(char));
    memcpy(hd->id, "RIFF", 4*sizeof(char));
    memcpy(hd->datastr, "data", 4*sizeof(char));
    hd->fmtnum = 16;
    hd->pcmnum = 1;
    hd->bipsamp = 16;
    hd->bypc = hd->bipsamp/8;

    int r[nr]={44100, 48000};
    int c[nc]={1, 2};
    short b[nb]={16, 24, 32};
    for(i=0;i<nr;++i) {
        for(j=0;j<nc;++j) {
            for(k=0;k<nb;++k) {
                hd->sampfq = r[i];
                hd->nchans = c[j];
                hd->bipsamp = b[k];
                printf("\"%s\" %d %d %d\n", hd->datastr, hd->nchans, hd->sampfq, hd->bipsamp);
            }
        }
    }

    free(hd);
    return 0;
}
