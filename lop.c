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

typedef struct /* time point type, tp_t */
{
    int h, m, s, hh;
} tp_t;

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
    printf("Usage: lops a wav file. Args: 1) option either -i or -e 2) H:mm:ss.hh string say to where wav must be treated 3) Name of wavfile.\n");
    printf("The time point itself is included in both cases. H is hours, hh is hundreths of a second.\n");
    printf("Timepoint defaults: with no : or ., timepoint is seconds. The time point itself is included in both cases. H is hours, hh is hundreths of a second.\n");
    printf("There's a small weakness in that if you have hours, and no hundreths, you have to specify zero hundreths with .0\n");
    printf("OK, so -i will be include and -e will be exclude.\n");
    printf("what this means is, that if you have only one timepoint, include -i means endlop\n");
    printf("i.e we include the first \"point\" bytes.\n");
    printf("and -e exclude is beglop, that is, the first \"tpoint\" bytes are exlcuded.\n");
    printf("When we have two time points, the situation changes because it looks like and interval, \n");
    printf("so we say that -i includes only that interval, and -e excludes it.\n");
    printf("Invoke example: lop so we say that -i includes only that interval, and -e excludes it.\n");
    return;
}

tp_t *s2tp2(char *str)
{
    tp_t *p=calloc(1, sizeof(tp_t));
    int rawns[4]={0};
	char tok[]=":.";
    int cou=0;

    // let's start
	char *tk=strtok(str, tok);
	if(tk ==NULL) {
		printf("strtok returned NULL, deault is seconds.\n");
        p->s=atoi(str);
        return p;
	} else {
		printf("First strtok: \"%s\" (size=%zu)\n", tk, strlen(tk));
        rawns[cou++]=atoi(tk);
    }
	while( (tk=strtok(NULL, tok)) != NULL) {
        if(cou>3) {
            printf("Time is malformed if it has so many [:.] tokens\n"); 
            exit(EXIT_FAILURE);
        }
		printf("\"%s\" (size=%zu)\n", tk, strlen(tk));
        rawns[cou++]=atoi(tk);
    }
    if(cou==2) {
        p->s=rawns[0];
        p->hh=rawns[1];
    } else if (cou==3) {
        p->m=rawns[0];
        p->s=rawns[1];
        p->hh=rawns[2];
    } else if (cou==4) {
        p->h=rawns[0];
        p->m=rawns[1];
        p->s=rawns[2];
        p->hh=rawns[3];
    }
    return p;
}

int main(int argc, char *argv[])
{
    unsigned char tpts2=0; // "time points 2"
    if((argc != 4) & (argc != 5)) {
        prtusage();
        exit(EXIT_FAILURE);
    } else if(argc == 5)
        tpts2=1;
    int infidx=3+tpts2; // infile index

    /* Before opening, let's use stat on the wav file */
    struct stat fsta;
    if(stat(argv[infidx], &fsta) == -1) {
        fprintf(stderr,"Can't stat input file %s. Is a file with that spelling in current folder? It needs to be.", argv[infidx]);
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
    inwavfp = fopen(argv[infidx],"rb");
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
    if((fsta.st_size - 8) != inhdr->glen) {
        printf("inhdr->glen not right in relation to file size!\n"); 
        exit(EXIT_FAILURE);
    }

    // we're going to have an output wav file, in the past I have overwritten the input wav header,
    // that's not such good practice, create a new one */
    wh_t *outhdr=malloc(hdrsz);
    memcpy(outhdr, inhdr, hdrsz);

    tp_t *p, *p2;
    int tpoint, tpoint2, tpdiff;
    p=s2tp2(argv[2]);
    printf("p1 ... stahours= %i, stamin=%u, stasec=%u, stahuns=%u\n", p->h, p->m, p->s, p->hh);
    tpoint=inhdr->byps*(p->h*3600 + p->m*60 + p->s) + p->hh*inhdr->byps/100;
    printf("tpoint to is %i inside %i which is %.1f%% in. Outside payload = %i\n", tpoint, inhdr->byid, (tpoint*100.)/inhdr->byid, inhdr->byid-tpoint);
    if(tpoint >= inhdr->byid) {
        printf("Timepoint at which to lop is over the size of the wav file. Aborting.\n");
        exit(EXIT_FAILURE);
    }
    if(tpts2) {
        p2=s2tp2(argv[3]);
        printf("p1 ... stahours= %i, stamin=%u, stasec=%u, stahuns=%u\n", p->h, p->m, p->s, p->hh);
        tpoint2=inhdr->byps*(p2->h*3600 + p2->m*60 + p2->s) + p2->hh*inhdr->byps/100;
        printf("tpoint2 to is %i inside %i which is %.1f%% in. Outside payload = %i\n", tpoint2, inhdr->byid, (tpoint2*100.)/inhdr->byid, inhdr->byid-tpoint2);
        if(tpoint2 >= inhdr->byid) {
            printf("Timepoint2 at which to lop is over the size of the wav file. Aborting.\n");
            exit(EXIT_FAILURE);
        }
        if(tpoint>=tpoint2) {
            printf("Timepoint2 must be greater than tpoint\n");
            exit(EXIT_FAILURE);
        }
        tpdiff=tpoint2-tpoint;
    }

    /* now we'll modify the inhdr to reflect what the out file will be */
    printf("BEF: outhdr->byid=%i\n", outhdr->byid);
    if(incorex==1) {
        if(!tpts2) {
            outhdr->glen -= (outhdr->byid - tpoint); // "decrease by amount that gets thrown out."
            outhdr->byid = tpoint; // we redefine
        } else {
            outhdr->glen -= (outhdr->byid - tpdiff); // "decrease by amount that gets thrown out."
            outhdr->byid = tpdiff; // we redefine
        }
    } else if (incorex==2) {
        if(!tpts2) {
            printf("Excluding the interval beyween timepoints not developed yet. It's not very common.\n");
            exit(EXIT_FAILURE);
        }
        outhdr->glen -= tpoint;
        outhdr->byid -= tpoint;
    }
    printf("AFT: outhdr->byid=%i\n", outhdr->byid);

    /* now we prepare the output filename */
    int infnsz=strlen(argv[infidx]);
    char *outfn=calloc(infnsz+5, sizeof(char));
    char *dot=strrchr(argv[infidx], '.'); // the final dot.
    strncpy(outfn, argv[infidx], (unsigned)(dot-argv[infidx]));
    strcat(outfn, "_lop.wav");
    FILE *outwavfp= fopen(outfn,"wb");
    fwrite(outhdr, hdrsz, 1, outwavfp);

    /* write: we can already write out inhdr to there */
    unsigned char *buf=malloc(outhdr->byid);
    if((incorex==2) | tpts2) // we have to do something for beglop (-e).  (inwavfp has already been read up to hdrsz above, no special case for incorex==1)
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
    free(p2);
    free(inhdr);
    free(outhdr);
    return 0;
}
