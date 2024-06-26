/* Takes a wav file and an mplayer-generated edl file and extracts the chunks */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h> 

#define GBUF 4
#define WBUF 4

typedef unsigned char boole;

typedef struct /*wh_t */
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

typedef struct /* wseq_t */
{
	size_t *wln;
	size_t wsbuf;
	size_t quan;
	size_t lbuf;
	size_t numl;
	size_t *wpla; /* words per line array number of words per line */
} wseq_t;

wseq_t *create_wseq_t(size_t initsz)
{
	wseq_t *words=malloc(sizeof(wseq_t));
	words->wsbuf = initsz;
	words->quan = initsz;
	words->wln=calloc(words->wsbuf, sizeof(size_t));
	words->lbuf=WBUF;
	words->numl=0;
	words->wpla=calloc(words->lbuf, sizeof(size_t));
	return words;
}

void free_wseq(wseq_t *wa)
{
	free(wa->wln);
	free(wa->wpla);
	free(wa);
}

int *pull_tnums(char *fname, int *m, int *n)
{
    /* In order to make no assumptions, the file is treated as lines containing the same amount of words each,
     * except for lines starting with #, which are ignored (i.e. comments). These words are checked to make sure they contain only [0-9:.]-type
     * characters only, one string variable is continually written over and copied into a growing floating point array ech time */

    /* declarations */
    FILE *fp=fopen(fname,"r");
    if(!fp) {
        printf("Error: unsuccessful opening of the EDL file associated with the audio file. Remember it needs\n");
        printf("to have the same rootname. And, of course, it also needs to be present in the current directory.\n"); 
        exit(EXIT_FAILURE);
    }
    int i;
    size_t couc /*count chars */, couw=0 /* count words */, oldcouw = 0;
    char c;
    boole inword=0;
    wseq_t *wa=create_wseq_t(GBUF);
    size_t bwbuf=WBUF;
    char *bufword=calloc(bwbuf, sizeof(char)); /* this is the string we'll keep overwriting. */

    int *mat=malloc(GBUF*sizeof(int));
    char *tmp0, mstr[16]={0};

    while( (c=fgetc(fp)) != EOF) {
        /*  take care of  */
        if( (c== '\n') | (c == ' ') | (c == '\t') | (c=='#')) {
            if( inword==1) { /* we end a word */
                wa->wln[couw]=couc;
                bufword[couc++]='\0';
                bufword = realloc(bufword, couc*sizeof(char)); /* normalize */
                tmp0=strchr(bufword, ':');
                strncpy(mstr,bufword, (tmp0-bufword)*sizeof(char));
                mat[couw]=6000*atoi(mstr) + 100*atoi(tmp0+1);
                couc=0;
                couw++;
            }
            if(c=='#') {
                while( (c=fgetc(fp)) != '\n') ;
                continue;
            } else if(c=='\n') {
                if(wa->numl == wa->lbuf-1) {
                    wa->lbuf += WBUF;
                    wa->wpla=realloc(wa->wpla, wa->lbuf*sizeof(size_t));
                    memset(wa->wpla+(wa->lbuf-WBUF), 0, WBUF*sizeof(size_t));
                }
                wa->wpla[wa->numl] = couw-oldcouw;
                oldcouw=couw;
                wa->numl++;
            }
            inword=0;
        } else if( (inword==0) && ((c == '.') | (c == ':') | ((c >= 0x30) && (c <= 0x39))) ) { /* deal with first character of new word, + and - also allowed */
            if(couw == wa->wsbuf-1) {
                wa->wsbuf += GBUF;
                wa->wln=realloc(wa->wln, wa->wsbuf*sizeof(size_t));
                mat=realloc(mat, wa->wsbuf*sizeof(int));
                for(i=wa->wsbuf-GBUF;i<wa->wsbuf;++i)
                    wa->wln[i]=0;
            }
            couc=0;
            bwbuf=WBUF;
            bufword=realloc(bufword, bwbuf*sizeof(char)); /* don't bother with memset, it's not necessary */
            bufword[couc++]=c; /* no need to check here, it's the first character */
            inword=1;
        } else if( (c == '.') | (c == ':') | ((c >= 0x30) && (c <= 0x39)) ) {
            if(couc == bwbuf-1) { /* the -1 so that we can always add and extra (say 0) when we want */
                bwbuf += WBUF;
                bufword = realloc(bufword, bwbuf*sizeof(char));
            }
            bufword[couc++]=c;
        } else {
            printf("Error. Non-float character detected. This program is only for reading floats\n"); 
            free_wseq(wa);
            exit(EXIT_FAILURE);
        }

    } /* end of big for statement */
    fclose(fp);
    free(bufword);

    /* normalization stage */
    wa->quan=couw;
    wa->wln = realloc(wa->wln, wa->quan*sizeof(size_t)); /* normalize */
    mat = realloc(mat, wa->quan*sizeof(int)); /* normalize */
    wa->wpla= realloc(wa->wpla, wa->numl*sizeof(size_t));

    *m= wa->numl;
    int k=wa->wpla[0];
    for(i=1;i<wa->numl;++i)
        if(k != wa->wpla[i])
            printf("Warning: Numcols is not uniform at %i words per line on all lines. This file has one with %zu.\n", k, wa->wpla[i]); 
    *n= k; 
    free_wseq(wa);

    return mat;
}

int *producenewmat2(char *edlf, int *matsz)
{
    int i, j, m, n;

    int *mat=pull_tnums(edlf, &m, &n);
    if(n!=1) {
        printf("Error. The tmg files should be made up of only one column of timings. Bailing out.\n");
        free(mat);
        exit(EXIT_FAILURE);
    }

    *matsz=m;
    return mat;
}

double *processinpf(char *fname, int *m, int *n)
{
	/* In order to make no assumptions, the file is treated as lines containing the same amount of words each,
	 * except for lines starting with #, which are ignored (i.e. comments). These words are checked to make sure they contain only floating number-type
	 * characters [0123456789+-.] only, one string variable is icontinually written over and copied into a growing floating point array each time */

	/* declarations */
	FILE *fp=fopen(fname,"r");
	int i;
	size_t couc /*count chars */, couw=0 /* count words */, oldcouw = 0;
	char c;
	boole inword=0;
	wseq_t *wa=create_wseq_t(GBUF);
	size_t bwbuf=WBUF;
	char *bufword=calloc(bwbuf, sizeof(char)); /* this is the string we'll keep overwriting. */

	double *mat=malloc(GBUF*sizeof(double));

	while( (c=fgetc(fp)) != EOF) {
		/*  take care of  */
		if( (c== '\n') | (c == ' ') | (c == '\t') | (c=='#')) {
			if( inword==1) { /* we end a word */
				wa->wln[couw]=couc;
				bufword[couc++]='\0';
				bufword = realloc(bufword, couc*sizeof(char)); /* normalize */
				mat[couw]=atof(bufword);
				couc=0;
				couw++;
			}
			if(c=='#') {
				while( (c=fgetc(fp)) != '\n') ;
				continue;
			} else if(c=='\n') {
				if(wa->numl == wa->lbuf-1) {
					wa->lbuf += WBUF;
					wa->wpla=realloc(wa->wpla, wa->lbuf*sizeof(size_t));
					memset(wa->wpla+(wa->lbuf-WBUF), 0, WBUF*sizeof(size_t));
				}
				wa->wpla[wa->numl] = couw-oldcouw;
				oldcouw=couw;
				wa->numl++;
			}
			inword=0;
		} else if( (inword==0) && ((c == 0x2B) | (c == 0x2D) | (c == 0x2E) | ((c >= 0x30) && (c <= 0x39))) ) { /* deal with first character of new word, + and - also allowed */
			if(couw == wa->wsbuf-1) {
				wa->wsbuf += GBUF;
				wa->wln=realloc(wa->wln, wa->wsbuf*sizeof(size_t));
				mat=realloc(mat, wa->wsbuf*sizeof(double));
				for(i=wa->wsbuf-GBUF;i<wa->wsbuf;++i)
					wa->wln[i]=0;
			}
			couc=0;
			bwbuf=WBUF;
			bufword=realloc(bufword, bwbuf*sizeof(char)); /* don't bother with memset, it's not necessary */
			bufword[couc++]=c; /* no need to check here, it's the first character */
			inword=1;
		} else if( (c == 0x2E) | ((c >= 0x30) && (c <= 0x39)) ) {
			if(couc == bwbuf-1) { /* the -1 so that we can always add and extra (say 0) when we want */
				bwbuf += WBUF;
				bufword = realloc(bufword, bwbuf*sizeof(char));
			}
			bufword[couc++]=c;
		} else {
			printf("Error. Non-float character detected. This program is only for reading floats\n"); 
			free_wseq(wa);
			exit(EXIT_FAILURE);
		}

	} /* end of big for statement */
	fclose(fp);
	free(bufword);

	/* normalization stage */
	wa->quan=couw;
	wa->wln = realloc(wa->wln, wa->quan*sizeof(size_t)); /* normalize */
	mat = realloc(mat, wa->quan*sizeof(double)); /* normalize */
	wa->wpla= realloc(wa->wpla, wa->numl*sizeof(size_t));

	*m= wa->numl;
	int k=wa->wpla[0];
	for(i=1;i<wa->numl;++i)
		if(k != wa->wpla[i])
			printf("Warning: Numcols is not uniform at %i words per line on all lines. This file has one with %zu.\n", k, wa->wpla[i]); 
	*n= k; 
	free_wseq(wa);

	return mat;
}

size_t *edl2mat(char *tmgfn, int *nr, int *nc, wh_t *inhdr, boole edlformat)
{
    /* we want the sampfq from the inhdr */
    int i, j, k;
	double *mat=processinpf(tmgfn, nr, nc);
    int nrd=*nr; /*deref */
    int ncd=*nc;

    int divby3=0;
    if(edlformat) {
        divby3=(nrd*ncd)%3;
        if(divby3) {
            printf("Error: the EDL file is not a multiple of 3. Bailing out.\n");
            exit(EXIT_FAILURE);
        }
    }
#ifdef DBG2
    printf("nrd: %d ncd: %d\n", nrd, ncd); 
#endif
    int tsampasz=nrd*(ncd-1); /* we get rid of one colum: mne: samp-pt array size */
    size_t *sampa=malloc(tsampasz*sizeof(size_t));
    k=0;
    for(i=0;i<nrd;++i) 
        for(j=0;j<ncd;++j) {
            if(edlformat) {
                if(!((ncd*i+j+1)%3)) /* no remainder after div by 3? we don't want it */
                    continue;
#ifdef DBG2
                printf("%d ", (ncd*i+j+1)%3); 
#endif
                sampa[k++]=(size_t)(.5 + ((double)inhdr->sampfq)*mat[ncd*i+j]);
            } else {
                sampa[k++]=(size_t)(.5 + ((double)inhdr->sampfq)*mat[ncd*i+j]);
            }
#ifdef DBG2
                printf("%zu done\n", sampa[k-1]);
#endif
        }
#ifdef DBG2
    putchar('\n');
#endif
    free(mat);
    return sampa;
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

int hdrchkbasic(wh_t *inhdr)
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

    return 0;
}

void prtusage(void)
{
    printf("Usage: divides wav file according to an mplayer-generated EDL or general timing (TMG) file.\n");
    // printf("2 or 3 arguments: if only 2, then it should be 1) name of wavfile 2) its samplefq (44100 or 48000)\n");
    printf("1 or 2 arguments: if only 1, then it should be 1) name of wavfile\n");
    printf("(its base filename will be used to deduce the edl or tmg file)\n");
    // printf("if 2 arguments: 1) Name of wavfile. 2) samplefreq (441/480,00) and 3) Name of edl- or tmg- file.\n");
    printf("if 2 arguments: 1) Name of wavfile and 2) Name of edl- or tmg- file.\n");
    return;
}

int main(int argc, char *argv[])
{
    unsigned char wesname=0; /* wav-edl-tmg same name? */
    boole edlformat=1; /* s the timing file in EDL format? If not it has to be tmg format */
    if( (argc !=2) & (argc != 3)) {
        prtusage();
        exit(EXIT_FAILURE);
    } else if (argc == 2)
        wesname=1; // edl or tmg file with same root name as wav file, and therefore implicit */

    struct stat fsta;
    char *cptr=strrchr(argv[1], '.');
    if(strcmp(cptr+1, "wav")) {
        printf("Input file must have a wav file extension. Bailing out.\n"); 
        exit(EXIT_FAILURE);
    }
    unsigned wflen=1+strlen(argv[1]); /* wav filename length */
    char *tmgfn=calloc(wflen, sizeof(char));
    if(wesname) {
        sprintf(tmgfn, "%.*s%s", (int)(cptr-argv[1]), argv[1], "edl");
        if(stat(tmgfn, &fsta) == -1) {
            sprintf(tmgfn, "%.*s%s", wflen-4, argv[1], "tmg");
            if(stat(tmgfn, &fsta) == -1) {
                printf("Neither an implicit *.edl nor *.tmg file exists for timings. Bailing out.\n"); 
                exit(EXIT_FAILURE);
            }
            edlformat=0;
        }
    } else {
        cptr=strchr(argv[2], '.');
        if(!strcmp(cptr+1, "tmg")) // so tmg extension is obligatory, if not edl
            edlformat=1;
        strcpy(tmgfn, argv[2]);
    }
    // printf("wflen:%u; tmgfn= %s\n", wflen, tmgfn); 

    int i, j, k, nr, nc;
    /* we expect som pretty big wavs so we can't rely on byid and gleni:
     * let's use stat.h to work them out instead */
    if(stat(argv[1], &fsta) == -1) {
        fprintf(stderr,"Can't open input file %s", argv[1]);
        exit(EXIT_FAILURE);
    }
#ifdef DBG
    printf("tot filesize=%zu\n", fsta.st_size); 
#endif
    size_t statglen=fsta.st_size-8;
    size_t tstatbyid=statglen-36; /* total stabyid, because we'll have a "current" stabyid for the chunks */
    /* open our wav file: we get the header in early that way */
    FILE *inwavfp;
    inwavfp = fopen(argv[1],"rb");
    /* of course we also need the header for other bits of data */
    wh_t *inhdr=malloc(sizeof(wh_t));
    if ( fread(inhdr, sizeof(wh_t), sizeof(char), inwavfp) < 1 ) {
        printf("Can't read file header\n");
        exit(EXIT_FAILURE);
    }
    if(hdrchkbasic(inhdr)) {
        printf("Header failed some basic WAV/RIFF file checks.\n");
        exit(EXIT_FAILURE);
    }
    /* in terms of the WAV, we're going to finish off by calculating the number of samples, not from
     * the inhdr->byid, but from the statbyid, and the nhdr->nchans and inhdr->bipsamp */
    size_t totsamps=(tstatbyid/inhdr->nchans)/(inhdr->bipsamp/8);

    /* I started a failed mod which would pass inhdr up to edl2mat() for a reason
     * that currently escapes me */
    size_t *sampa=edl2mat(tmgfn, &nr, &nc, inhdr, edlformat);

    int chunkquan=nr*(nc-1)+1; /* include pre-first edlstartpt, post-last edlendpt, and edlstart and edlend interstitials. */

    char *tmpd=mktmpd();
    printf("INFO: split files to be put into folder %s.\n", tmpd);
    char *fn=calloc(64, sizeof(char));
    unsigned char *bf=NULL;
    FILE *outwavfp;
    size_t frompt, topt, staby, cstatbyid /* current statbyid */;
    int byidmultiplier=inhdr->nchans*inhdr->bipsamp/8;

    for(j=0;j<chunkquan;++j) { /* note we will reuse the inhdr, only changing byid and glen */

        frompt = (j==0)? 0: sampa[j-1];
        topt = (j==chunkquan-1)? totsamps: sampa[j]; /* initially had cstatbyid instead of totsamps ... kept obvershooting file */
        cstatbyid = (topt - frompt)*byidmultiplier;
        bf=realloc(bf, cstatbyid*sizeof(unsigned char));
        staby=frompt*byidmultiplier; /* starting byte */
#ifdef DBG
        printf("j: %d frompt: %zu, topt: %zu, cstatbyid: %zu, staby: %zu ", j, frompt, topt, cstatbyid, staby);
#endif

        /* here we revert to wav's limitations on max file size */
        inhdr->byid=(int)cstatbyid; /* overflow a distinct possibility */
        inhdr->glen = inhdr->byid+36;

        sprintf(fn, "%s/%03i.wav", tmpd, j); /* purposely only allow FOR 1000 edits */
        outwavfp= fopen(fn,"wb");

        fwrite(inhdr, sizeof(unsigned char), 44, outwavfp);

        fseek(inwavfp, 44+staby, SEEK_SET); /* originally forgot to skip the 44 bytes of the header! */
#ifdef DBG
        printf("ftell: %li ftell+cstatbyid: %li\n", ftell(inwavfp), cstatbyid+ftell(inwavfp)); 

#endif
        if ( fread(bf, cstatbyid, sizeof(unsigned char), inwavfp) < 1 ) {
            printf("Sorry, trouble putting input file into array. Overshot maybe?\n"); 
            exit(EXIT_FAILURE);
        }
        fwrite(bf, sizeof(unsigned char), cstatbyid, outwavfp);
        fclose(outwavfp);
    }
    fclose(inwavfp);
    free(tmpd);
    free(bf);
    free(sampa);
    free(fn);
    free(inhdr);
    free(tmgfn);
    return 0;
}
