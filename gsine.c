/*  lopwav. the idea is to lop off either the start or the end of the file. All from the command line */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h> 
#include<ctype.h> // required for isprint
#include<unistd.h> // required for optopt, opterr and optarg.

#define MXINTV 0x7FFFFFFF /* max int value */

#define ARBSZ 128
#define GBUF 64

typedef struct  /* optstruct_raw, a struct for the options */
{
    unsigned char mflg; /* mono flag, if not set, stereo */
    unsigned char bval; /* bit per sample, 8 16 or 32, never over 255 */
    unsigned fval; /* sample frequency .. mostly 44100, but would like to experiment with others */
    char *istr; /* input filename */
} optstruct_raw;

typedef struct  /* opts_true: a structure for the options reflecting their true types */
{
    unsigned char m, b; /* if 0 no mono-ization. 1 for use left channel, 2, for right, 3 for arbitrarily choose */
    unsigned f;
    char *inpfn;
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

long fszfind(FILE *fp)
{
    rewind(fp);
    fseek(fp, 0, SEEK_END);
    long fbytsz = ftell(fp);
    rewind(fp);
    return fbytsz;
}

wh_t *hdr4chunk(int sfre, char nucha, int certainsz) /* a header for a file chunk of certain siez */
{
    wh_t *wh=malloc(sizeof(wh_t));
    strncpy(wh->id, "RIFF", 4);
    strncpy(wh->fstr, "WAVEfmt ", 8);
    strncpy(wh->datastr, "data", 4);
    wh->fmtnum=16;
    wh->pcmnum=1;
    wh->nchans=nucha; /* fed in to function */
    wh->sampfq=sfre; /* fed in to function */
    wh->glen=certainsz-8;
    wh->bipsamp=16;
    wh->bypc=wh->bipsamp/8;
    wh->byps=wh->nchans*wh->sampfq*wh->bypc;
    wh->byid=wh->glen-36;
    return wh;
}

int hdrchk(wh_t *inhdr)
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

    printf("There substantial evidence in the non-numerical fields of this file's header to think it is a wav file\n");

    printf("glen: %i\n", inhdr->glen);
    printf("byid: %i\n", inhdr->byid);
    printf("nchans: %d\n", inhdr->nchans);
    printf("sampfq: %i\n", inhdr->sampfq);
    printf("byps: %i\n", inhdr->byps);
    printf("bypc, bytes by capture (count of data in shorts): %d\n", inhdr->bypc);
    printf("bipsamp: %d\n", inhdr->bipsamp);

    if(inhdr->glen+8-44 == inhdr->byid)
        printf("Good, \"byid\" (%i) is 36 bytes smaller than \"glen\" (%i).\n", inhdr->byid, inhdr->glen);
    else {
        printf("WARNING: glen (%i) and byid (%i)do not show prerequisite normal relation(diff is %i).\n", inhdr->glen, inhdr->byid, inhdr->glen-inhdr->byid); 
    }
    // printf("Duration by glen is: %f\n", (float)(inhdr->glen+8-wh_tsz)/(inhdr->nchans*inhdr->sampfq*inhdr->byps));
    printf("Duration by byps is: %f\n", (float)(inhdr->glen+8-44)/inhdr->byps);

    if( (inhdr->bypc == inhdr->byps/inhdr->sampfq) && (inhdr->bypc == 2) )
        printf("bypc complies with being 2 and matching byps/sampfq. Data values can therefore be recorded as signed shorts.\n"); 

    return 0;
}

opts_true *processopts(optstruct_raw *rawopts)
{
    opts_true *trueopts=calloc(1, sizeof(opts_true));

    /* some are easy as in, they are direct tranlsations */
    trueopts->m=rawopts->mflg;
    trueopts->b=rawopts->bval;
    trueopts->f=rawopts->fval;

    if(rawopts->istr) {
        trueopts->inpfn=rawopts->istr;
        printf("The input wav filename option was defined and it is set at \"%s\".\n", trueopts->inpfn);
    } else {
        printf("A filename after the -i option is obligatory: it's the input wav filename option.\n");
        exit(EXIT_FAILURE);
    }
    return trueopts;
}

int catchopts(optstruct_raw *opstru, int oargc, char **oargv)
{
    int c;
    opterr = 0;

    while ((c = getopt (oargc, oargv, "mb:f:i:")) != -1)
        switch (c) {
            case 'm':
                opstru->mflg = 1;
                break;
            case 'b':
                opstru->bval = (unsigned char)atoi(optarg);
                break;
            case 'f':
                opstru->fval = (unsigned)atoi(optarg);
                break;
            case 'i':
                opstru->istr = optarg;
                break;
            case '?':
                /* general error state? */
                if (optopt == 'i')
                    fprintf (stderr, "Option -%c requires an filename argument.\n", optopt);
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
    printf("This program cuts wav (specified by -i, into chunks according to time (mm:ss.hh) interval specified by -t.\n");
    printf("This program converts 32bit sampled to 16 bit by simple shifting.\n"); 
    printf("The input wav is indicated by the -i option. The -m option is a flag for also mono-izing the file.\n");
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

    /* create and set wavheader members */
    wh_t *whd=malloc(sizeof(wh_t));
    whd->nchans = (trueopts->m)? 1: 2;
    whd->bipsamp=trueopts->b;
    whd->bypc=whd->bipsamp/8;
    whd->sampfq = trueopts->f;
    whd->byps = whd->nchans * whd->sampfq * whd->bypc;
    memcpy(whd->id, "RIFF",4*sizeof(char));
    memcpy(whd->datastr, "data",4*sizeof(char));
    memcpy(whd->fstr, "WAVEfmt ", 8*sizeof(char));
    whd->fmtnum = 16;
    whd->pcmnum = 1;
    /* two members left ... these are the size dependent headers */
    whd->byid = point/downsz;
        }

    /* Before opening, let's use stat on the wav file */
    struct stat fsta;
    if(stat(trueopts->inpfn, &fsta) == -1) {
        fprintf(stderr,"Can't stat input file %s", trueopts->inpfn);
        exit(EXIT_FAILURE);
    }
    size_t statglen=fsta.st_size-8; //filesz less 8
    size_t statbyid=statglen-36; // filesz less 8+36, so that's less the wav header

    FILE *inwavfp;
    inwavfp = fopen(trueopts->inpfn, "rb");
    if ( inwavfp == NULL ) {
        fprintf(stderr,"Can't open input file %s", trueopts->inpfn);
        exit(EXIT_FAILURE);
    }

    if(statbyid%2 == 0)
        printf("statbyid is even, good.\n");  // So, what is the significance of that. Say please.
    else {
        printf("Ooops statbyid is not even.\n"); // I suppose only 8bitmono wavs can be odd
        exit(EXIT_FAILURE);
    }

    /* of course we also need the header for other bits of data */
    wh_t *inhdr=malloc(sizeof(wh_t));
    if ( fread(inhdr, sizeof(wh_t), sizeof(char), inwavfp) < 1 ) {
        printf("Can't read file header\n");
        exit(EXIT_FAILURE);
    }

    printf("stamin=%u, stasec=%u, stahuns=%u\n", trueopts->mm, trueopts->ss, trueopts->hh);
    int point=inhdr->byps*(trueopts->mm*60 + trueopts->ss) + trueopts->hh*inhdr->byps/100;
    if(point >= inhdr->byid) {
        printf("Timepoint at which to lop is over the size of the wav file. Aborting.\n");
        exit(EXIT_FAILURE);
    }
    long iwsz=fszfind(inwavfp);
    // perhaps the following guys should be called output chunks, as in ochunks, because someone might think that the chunks were input chunks. Actually there are input chunks
    /* note in the following, is ->s is set there will be one split only, (although thsi is inevitable if the poijnt is over half the size of the file */
    int fullchunkz= (trueopts->s)? 1 : (iwsz-44)/point;
    int partchunk= (trueopts->s)? point*((iwsz-44)/point -1) + (iwsz-44)%point : (iwsz-44)%point;
#ifdef DBG
    printf("fullchkz: %d, partchk: %d\n", fullchunkz, partchunk);
#endif
    int chunkquan=(partchunk==0)? fullchunkz : fullchunkz+1;
    int bytesinchunk;

    if(trueopts->s)
        printf("One splitpoint from file of size %d: first of size %d, second of size %d (these 2 added: %d)\n", inhdr->byid, point, partchunk, point + partchunk); 
    else
        printf("Point is %i and goes into the total data payload is %li times plus %.1f%% left over.\n", point, (iwsz-44)/point, ((iwsz-44)%point)*100./point);

    char *tmpd=mktmpd();
    printf("Your split chunks will go into directory \"%s\"\n", tmpd);
    char *fn=calloc(GBUF, sizeof(char));
    // char *bf=malloc(inhdr->byid); // fir slurping the entire file
    char *bf=NULL;
    if(partchunk>point)
        bf=malloc(partchunk*sizeof(char)); // maximum amoutn we'll be reading from the file
    else
        bf=malloc(point*sizeof(char)); // ref. above: too generous ... in fact it need only be this big
    FILE *outwavfp;

    /* we're also going to mono ize and reduce 32 bit samples to 16, so modify the header */
    wh_t *outhdr=malloc(sizeof(wh_t));
    memcpy(outhdr, inhdr, 44*sizeof(char));
    int i, j;
    int downsz;
    if(trueopts->m) {
        if(trueopts->d)
            downsz=4;
        else
            downsz=2;
    } else {
        if(trueopts->d)
            downsz=2;
        else
            downsz=1;
    }

    if(trueopts->m)
        outhdr->nchans = 1; // if not, keep to original (assured bia memcpy above)
    outhdr->bipsamp=16; //force 16 bit sampling.
    outhdr->bypc=outhdr->bipsamp/8;
    outhdr->byps = outhdr->nchans * outhdr->sampfq * outhdr->bypc;
    for(j=0;j<chunkquan;++j) {

        if( (j==chunkquan-1) && partchunk) {
            bytesinchunk = partchunk;
            outhdr->byid = partchunk/downsz;
        } else { // j==chunkquan-1 with partchunk zero will also get here.
            bytesinchunk = point;
            outhdr->byid = point/downsz;
        }
        outhdr->glen = outhdr->byid+36;

        sprintf(fn, "%s/%05i.wav", tmpd, j);
        outwavfp= fopen(fn,"wb");

        fwrite(outhdr, sizeof(char), 44, outwavfp);

        /* OK read in one chunk of size bytesinchunk. fread runs its fd position down the file */
#ifdef DBG
        printf("byid for this fread = %d, bytesinchunk=%d\n",outhdr->byid, bytesinchunk); 
#endif
        if ( fread(bf, bytesinchunk, sizeof(char), inwavfp) < 1 ) {
            printf("Sorry, trouble putting input file into array. Overshot maybe?\n"); 
            exit(EXIT_FAILURE);
        }

        /* for the mon mix ... it will be lossy of course */
        short combine, combine2;
        int outcome;
        float intermediate;

        if(trueopts->d) { // if we are downsizing from 32 to 16.
            for(i=0;i<outhdr->byid;i+=2) { // this is entirely for small-endian. big endian haughtily ignored. Sorry!
                if(trueopts->m==1) {
                    bf[i]=bf[4*i+4];
                    bf[i+1]=bf[4*i+5];
                } else if(trueopts->m==2) {
                    bf[i]=bf[4*i+6];
                    bf[i+1]=bf[4*i+7];
                } else if(trueopts->m==3) {
                    combine=0; // assure ourselves we have a clean int.
                    combine = (short)bf[4*i+7]; // casually slot into first byte;
                    combine <<= 8; // move first byte into second
                    combine |= (short)bf[4*i+6]; //merge with least signficant
                    combine2=0; // assure ourselves we have a clean int.
                    combine2 = (short)bf[4*i+5]; // casually slot into first byte;
                    combine2 <<= 8; // move first byte into second
                    combine2 |= (short)bf[4*i+4]; //merge with least signficant
                    intermediate=(float)(combine + combine2)*.5;
                    outcome=(int)(intermediate+.5);
                    bf[i]=(char)(outcome&0xff);
                    bf[i+1]=(char)((outcome&0xff00)>>8);
                } else if(!trueopts->m) {
                    bf[i]=bf[2*i+2];
                    bf[i+1]=bf[2*i+3];
                }
            }
        } else if(trueopts->m) { // i.e no downsizing, but yes mono-izing.
            for(i=0;i<outhdr->byid;i+=2) { // this is entirely for small-endian. big endian haughtily ignored. Sorry!
                if(trueopts->m==1) { // want the first channel
                    bf[i]=bf[2*i];
                    bf[i+1]=bf[2*i+1];
                } else if(trueopts->m==2) {
                    bf[i]=bf[2*i+2];
                    bf[i+1]=bf[2*i+3];
                } else if(trueopts->m==3) {
                    combine=0; // assure ourselves we have a clean int.
                    combine = (short)bf[2*i+3]; // casually slot into first byte;
                    combine <<= 8; // move first byte into second
                    combine |= (short)bf[2*i+2]; //merge with least signficant
                    combine2=0; // assure ourselves we have a clean int.
                    combine2 = (short)bf[2*i+1]; // casually slot into first byte;
                    combine2 <<= 8; // move first byte into second
                    combine2 |= (short)bf[2*i]; //merge with least signficant
                    intermediate=(float)(combine + combine2)*.5;
                    outcome=(int)(intermediate+.5);
                    bf[i]=(char)(outcome&0xff);
                    bf[i+1]=(char)((outcome&0xff00)>>8);
                }
            }
        }

        fwrite(bf, sizeof(char), outhdr->byid, outwavfp);
        fclose(outwavfp);
    }
    fclose(inwavfp);
    free(tmpd);
    free(bf);
    free(fn);
    free(trueopts);
    free(inhdr);
    free(outhdr);
    return 0;
}
