/* Takes an edl file and extracts the chunks */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h> 

#define GBUF 64
#define WBUF 8

typedef unsigned char boole;

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

void free_wseq(wseq_t *wa)
{
    free(wa->wln);
    free(wa->wpla);
    free(wa);
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

int main(int argc, char *argv[])
{
    if(argc != 3) {
        printf("Usage: reads an edl file and outputs in several formats.\n");
        printf("2 arguments: 1) name of edl-file. 2) target mp3, ogg or flac.\n");
        exit(EXIT_FAILURE);
    }
    char *ext=strrchr(argv[2],'.')+1; // mp3splt will add this anyway
    char cmd[256]={0};
    char cmd0[16]={0};
    if(!strcmp(ext,"ogg"))
        strcpy(cmd0,"oggsplt");
    else if(!strcmp(ext,"mp3"))
        strcpy(cmd0,"mp3splt");
    int i, j, nr, nc, mins;
    float rsecs;
    double *mat=processinpf(argv[1], &nr, &nc);
    int divby3=(nr*nc)%3;
    if(divby3) {
        printf("Error: the EDL file is not a multiple of 3. Bailing out.\n");
        exit(EXIT_FAILURE);
    }
#ifdef DBG
    printf("nr: %d nc: %d\n", nr, nc); 
#endif
    int mins0=0;
    float rsecs0=.0;
    char *tmpd=mktmpd();
    int k=0;
    for(i=0;i<nr;++i) {
        for(j=0;j<nc;++j) {
            if(!((nc*i+j+1)%3)) /* no remainder after div by 3? we don't want it */
                continue;
            strcpy(cmd,cmd0);
            mins=(int)(mat[nc*i+j]/60.0);
            rsecs=mat[nc*i+j]-mins*60.0;
            sprintf(cmd,"%s %s %02d.%05.2f %02d.%05.2f -d %s -o %03d", cmd0, argv[2], mins0, rsecs0, mins, rsecs, tmpd, k);
            // real problems gettimg mp3splt to accept and output situation.
            printf("%s\n", cmd);
            system(cmd);
            k++;
            mins0=mins;
            rsecs0=rsecs;
        }
    }
    // The last one
    sprintf(cmd,"%s %s %02d.%05.2f %s -o %03d", cmd0, argv[2], mins0, rsecs0, "EOF", k);
    printf("%s\n", cmd);
    system(cmd);

    free(mat); /* we've rendered the edl matrix into a integers now, as in sampa */
    return 0;
}
