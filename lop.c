/* taking endlop and generally improving it, especially not reading in the whole file
Options Rationale:
OK, so -i will be include and -e will be exclude.
what this means is, that if you have only one timepoint, include -i means endlop
i.e we include the first "point" bytes.
and -e exclude is beglop, that is, the first "point " bytes are exlcuded.
When we have two time points, the situation changes because it looks like and interval, 
so we say that -i includes only that interval, and -e excludes it.
*/
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include<unistd.h>
#include<sys/time.h>
#include<sys/stat.h>
#include<dirent.h> 

#define MXINTV 0x7FFFFFFF /* max int value */

#define ARBSZ 128

typedef struct /* time point, tpt */
{
    int m, s, h;
} tpt;

typedef struct /*wav header */
{
    char id[4]; // should always contain "RIFF"
    int glen;    // general length: total file length minus 8, could because the types so far seen (id and the glen member itself) are actually 8 bytes
    char fstr[8]; // format string should be "WAVEfmt ": actually two 4 char types here.
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
    printf("Usage: lops a wav file. Args: 1) option either -i or -e 2) mm:ss.hh string say to where wav must be treated 3) Name of wavfile.\n");
    printf("The time point itself is included in both cases.\n");
    printf("OK, so -i will be include and -e will be exclude.\n");
    printf("what this means is, that if you have only one timepoint, include -i means endlop\n");
    printf("i.e we include the first \"point\" bytes.\n");
    printf("and -e exclude is beglop, that is, the first \"tpoint\" bytes are exlcuded.\n");
    printf("When we have two time points, the situation changes because it looks like and interval, \n");
    printf("so we say that -i includes only that interval, and -e excludes it.\n");
    return;
}

tpt *s2tp(char *str)
{
    tpt *p=malloc(sizeof(tpt));
    char *tc, staminchar[5]={0};
    if( (tc=strchr(str, ':')) == NULL) {
        printf("There are no minutes \n"); 
        p->m=0;
        tc=str;
    } else {
        strncpy(staminchar, str, (int)(tc-str));
        p->m=atoi(staminchar);
        tc++;
    }

    char *ttc, stasecchar[5]={0};
    if( (ttc=strchr(tc, '.')) == NULL) {
        printf("There are no seconds\n"); 
        p->s=0;
        ttc=tc;
    } else {
        strncpy(stasecchar, tc, (int)(ttc-tc));
        p->s=atoi(stasecchar);
        ttc++;
    }
    char stahunschar[5]={0};
    strcpy(stahunschar, ttc);
    p->h=atoi(stahunschar);
    if((strlen(stahunschar))==1)
        p->h*=10;

    return p;
}

int main(int argc, char *argv[])
{
    if((argc != 4) & (argc != 5)) {
        prtusage();
        exit(EXIT_FAILURE);
    }

    /* Before opening, let's use stat on the wav file */
    struct stat fsta;
    if(stat(argv[3], &fsta) == -1) {
        fprintf(stderr,"Can't stat input file %s", argv[1]);
        exit(EXIT_FAILURE);
    }
    printf("size of raw file is: %zu\n", fsta.st_size);
    // char *buf=malloc(fsta.st_size);

    // now to detemine whether include or exclude option has been chosen
    char incorex; // include or exclude?
    if(!strcmp(argv[1], "-i"))
       incorex=1; // 1 seems natural for include.
    else if (!strcmp(argv[1], "-e"))
       incorex=2;
    else {
        prtusage();
        exit(EXIT_FAILURE);
    }

    size_t hdrsz=sizeof(wh_t);
    FILE *inwavfp;
    inwavfp = fopen(argv[3],"rb");
    if ( inwavfp == NULL ) {
        fprintf(stderr,"Can't open input file %s", argv[1]);
        exit(EXIT_FAILURE);
    }

    // read header first
    wh_t *inhdr=malloc(hdrsz);
    if ( fread(inhdr, hdrsz, sizeof(char), inwavfp) < 1 ) {
        printf("Can't read file header\n");
        exit(EXIT_FAILURE);
    }

    // header checks
    if((fsta.st_size - hdrsz) != inhdr->byid) {
        printf("inhdr->byid not right in relation to file size!\n"); 
        exit(EXIT_FAILURE);
    }

    // we're going to have an output wav file, in the past I have overwritten the input wav header,
    // that's not such good practice, create a new one */
    wh_t *outhdr=malloc(hdrsz);
    memcpy(outhdr, inhdr, hdrsz);

    tpt *p=s2tp(argv[2]);

    printf("stamin=%u, stasec=%u, stahuns=%u\n", p->m, p->s, p->h);
    int tpoint=inhdr->byps*(p->m*60 + p->s) + p->h*inhdr->byps/100;
    printf("tpoint to is %i inside %i which is %.1f%% in. Outside payload = %i\n", tpoint, inhdr->byid, (tpoint*100.)/inhdr->byid, inhdr->byid-tpoint);
    if(tpoint >= inhdr->byid) {
        printf("Timepoint at which to lop is over the size of the wav file. Aborting.\n");
        exit(EXIT_FAILURE);
    }

    /* now we'll modify the inhdr to reflect what the out file will be */
    printf("BEF: inhdr->byid=%i\n", inhdr->byid);
    printf("BEF: outhdr->byid=%i\n", outhdr->byid);
    if(incorex==1) {
        outhdr->glen -= (outhdr->byid - tpoint); // "decrease by amount that gets thrown out."
        outhdr->byid = tpoint; // we redefine
    } else if (incorex==2) {
        // outhdr->glen -= (tpoint + outhdr->bypc*outhdr->nchans);
        // outhdr->byid -= (tpoint + outhdr->bypc*outhdr->nchans);
        outhdr->glen -= tpoint;
        outhdr->byid -= tpoint;
    }
    printf("AFT: outhdr->byid=%i\n", outhdr->byid);

    /* now we prepare the output filename */
    int infnsz=strlen(argv[3]);
    char *outfn=calloc(infnsz+5, sizeof(char));
    char *dot=strrchr(argv[3], '.'); // the final dot.
    strncpy(outfn, argv[3], (unsigned)(dot-argv[3]));
    strcat(outfn, "_lop.wav");
    FILE *outwavfp= fopen(outfn,"wb");
    fwrite(outhdr, hdrsz, 1, outwavfp);

    /* write: we can already write out inhdr to there */
    unsigned char *buf=malloc(outhdr->byid);
    if(incorex==2) // we have to do something for beglop (-e).  (inwavfp has already been read up to hdrsz above, no special case for incorex==1)
        fseek(inwavfp, tpoint, SEEK_CUR);
        // fseek(inwavfp, -fsta.st_size+tpoint, SEEK_END);
    // with outhdr->byid already settled above, and the filepointer in its right place, for both cases all we need is this:
    if ( fread(buf, outhdr->byid, sizeof(unsigned char), inwavfp) < 1 ) { /* Yup! we slurp in the baby! */
        printf("Sorry, trouble putting input file into array. Overshot maybe?\n"); 
        exit(EXIT_FAILURE);
    }
    fclose(inwavfp);
    fwrite(buf, sizeof(unsigned char), outhdr->byid, outwavfp);
    fclose(outwavfp);
    free(buf);

    free(outfn);
    free(p);
    free(inhdr);
    free(outhdr);
    return 0;
}
