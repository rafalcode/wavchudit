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

static void print_confirmation_and_exit_if_error(splt_state *state, splt_code error) //Callback function that handles error code from libmp3splt.
{
    char *message = mp3splt_get_strerror(state, error);
    if (!message) {
        printf("State not associated with any error.\n"); 
        return;
    }

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
    if(argc == 1) {
        printf("Usage: argument 1) mp3 file 2) to n) Timings in mm:ss.hh format to serve as splitpoints.\n");
        exit(EXIT_FAILURE);
    }
    int i;

    /* OK, time for the mp3splt code */
    splt_code error = SPLT_OK; /* a start section I expect */
    splt_state *state = mp3splt_new_state(NULL);
    mp3splt_set_message_function(state, print_message_from_library, NULL);
    mp3splt_set_split_filename_function(state, print_split_filename, NULL);
    error = mp3splt_find_plugins(state);
    print_confirmation_and_exit_if_error(state, error);
    mp3splt_set_filename_to_split(state, argv[1]);

    // OK, let's declare the points
    int mm=0, ss=0, hh=0, fhh=0 /* so-called full hundreds format, result of combining previous three */;
    fhh = mm*6000 + ss*100 + hh;
    mp3splt_append_splitpoint(state, mp3splt_point_new(fhh, NULL));
    for(i=2;i<argc;++i) {
        sscanf(argv[i], "%d:%d.%d", &mm, &ss, &hh);
        fhh = mm*6000 + ss*100 + hh;
        mp3splt_append_splitpoint(state, mp3splt_point_new(fhh, NULL));
    }
    mm=9999; ss=59; hh=99;
    fhh = mm*6000 + ss*100 + hh;
    mp3splt_append_splitpoint(state, mp3splt_point_new(fhh, NULL));
    error = mp3splt_split(state);
    print_confirmation_and_exit_if_error(state, error);
    //free the memory of the main state
    mp3splt_free_state(state);

    return EXIT_SUCCESS;
}
