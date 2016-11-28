/* Takes an edl file and extracts the chunks */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h> 
#include <libmp3splt/mp3splt.h>

#define GBUF 64
#define WBUF 8

// edl times pushed 2 secs ahead: compenstae
#define PBACK 400

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

void free_wseq(wseq_t *wa)
{
    free(wa->wln);
    free(wa->wpla);
    free(wa);
}

int *processinpf(char *fname, int *m, int *n)
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

    int *mat=malloc(GBUF*sizeof(int));

    while( (c=fgetc(fp)) != EOF) {
        /*  take care of  */
        if( (c== '\n') | (c == ' ') | (c == '\t') | (c=='#')) {
            if( inword==1) { /* we end a word */
                wa->wln[couw]=couc;
                bufword[couc++]='\0';
                bufword = realloc(bufword, couc*sizeof(char)); /* normalize */
                mat[couw]=(int)(100.5*atof(bufword)); /* note EDLs come in seconds, and libmp3 is seconds-hundreths, so multiplying and orunding to nearest is fine */
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
                mat=realloc(mat, wa->wsbuf*sizeof(int));
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

static void print_confirmation_and_exit_if_error(splt_state *state, splt_code error) //Callback function that handles error code from libmp3splt.
{
    char *message = mp3splt_get_strerror(state, error);
    if (!message)
        return;

    if (error < 0) {
        fprintf(stderr, "%s\n", message);
        fflush(stderr);
        mp3splt_free_state(state);
        exit(1);
    } else {
        fprintf(stdout, "%s\n", message);
        fflush(stdout);
    }
    free(message);
}

static void print_message_from_library(const char *message, splt_message_type type, void *data) //Callback function printing any messages from libmp3splt.
{
    if (type == SPLT_MESSAGE_INFO) { /* then, message to STDOUT */
        fprintf(stdout, message);
        fflush(stdout);
        return;
    }
    fprintf(stderr, message);
    fflush(stderr);
}

static void print_split_filename(const char *filename, void *data) //Callback function printing the created filenames.
{
    fprintf(stdout, "   %s created.\n", filename);
    fflush(stdout);
}

int main(int argc, char *argv[])
{
    if(argc != 3) {
        printf("Usage: argument 1) mp3 file 2) associated EDL file.\n");
        exit(EXIT_FAILURE);
    }
    int i, j, nr, nc;
    int *mat=processinpf(argv[2], &nr, &nc);
    int divby3=(nr*nc)%3;
    if(divby3) { // EDL files have 3 columns ... bail out if there are not three columns.
        printf("Error: the EDL file is not a multiple of 3. Bailing out.\n");
        exit(EXIT_FAILURE);
    }
#ifdef DBG
    printf("nr: %d nc: %d\n", nr, nc); 
#endif
    int *newmat=malloc((2*nr*nc/3)*sizeof(int)); // we'll create a new matrix without the third column.
    int k=0;
    for(i=0;i<nr;++i)
        for(j=0;j<2;++j)
            newmat[k++]=mat[nc*i+j] -PBACK;
    free(mat);
    /* OK, time for the mp3splt code */
    splt_code error = SPLT_OK; /* a start section I expect */
    //initialisation of the main state, always necessary.
    splt_state *state = mp3splt_new_state(NULL);
    //register callback functions, necessary cos that's the way this library is built. Note references to above defined function.
    mp3splt_set_message_function(state, print_message_from_library, NULL);
    mp3splt_set_split_filename_function(state, print_split_filename, NULL);
    //look for the available plugins .. maybe not necessary, but go with it for time being.
    error = mp3splt_find_plugins(state);
    print_confirmation_and_exit_if_error(state, error);
    //set the input filename to be split
    mp3splt_set_filename_to_split(state, argv[1]);
    // OK, let's declare the points
//    splt_point *p1 = NULL, *p2=NULL;
    printf("EDL timings printed now\n"); 
    for(i=0;i<nr;++i) {
        printf("%dm%ds%dh -> ", newmat[2*i]/6000, (newmat[2*i] - 6000*(newmat[2*i]/6000))/100, (newmat[2*i] - 6000*(newmat[2*i]/6000)) - 100*((newmat[2*i] - 6000*(newmat[2*i]/6000))/100)); // yes, these minute second conversions are hellish, no way around it.
        mp3splt_append_splitpoint(state, mp3splt_point_new(newmat[2*i], NULL));
        printf("%dm%ds%dh\n", newmat[2*i+1]/6000, (newmat[2*i+1] - 6000*(newmat[2*i+1]/6000))/100, (newmat[2*i+1] - 6000*(newmat[2*i+1]/6000)) - 100*((newmat[2*i+1] - 6000*(newmat[2*i+1]/6000))/100)); // note these conversion exactly reflect what libmp3splt things it's doing. See auto output filename.
        mp3splt_append_splitpoint(state, mp3splt_point_new(newmat[2*i+1], NULL));
    }
    printf("\n"); 
    error = mp3splt_split(state);
    print_confirmation_and_exit_if_error(state, error);
    //free the memory of the main state
    mp3splt_free_state(state);

    free(newmat); /* we've rendered the edl matrix into a integers now, as in sampa */
    return EXIT_SUCCESS;
}
