#include <stdio.h>
#include <stdlib.h>

#include <libmp3splt/mp3splt.h>

//Main program
//Please note that not all errors are handled in this example.
//Compile with:
//    $ gcc minimal.c -o minimal -lmp3splt
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
  if (type == SPLT_MESSAGE_INFO) {
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
    /* notes.
     * looking for total time I'm sure .... it in splt_struct: long total_time
     * that's found as split in _splt_state
     * note underscore.
     * where is it normalised?
     * found in include/libmp3splt.h
     * typedef struct _splt_state splt_state;
     * However this is a very OO type c code
     * claim to have private var and funcs.
     * maybe that total timeis not allowed. 
     * Need to "get" it
     */




  if (argc != 2) {
    fprintf(stderr, "Please provide the input file to be split as the first argument.\n");
    fflush(stderr);
    return EXIT_FAILURE;
  }

  splt_code error = SPLT_OK;

  //initialisation of the main state, the main activity container, so to speak.
  splt_state *state = mp3splt_new_state(NULL);

  //register callback functions
  mp3splt_set_message_function(state, print_message_from_library, NULL);
  mp3splt_set_split_filename_function(state, print_split_filename, NULL);

  //look for the available plugins
  error = mp3splt_find_plugins(state);
  print_confirmation_and_exit_if_error(state, error);

  //set the input filename to be split
  mp3splt_set_filename_to_split(state, argv[1]);

  splt_struct *m=state->split;
  // printf("tt: %li\n", state->split->total_time); 
  //append two splitpoints
  splt_point *first_point = mp3splt_point_new(0, NULL);
  mp3splt_append_splitpoint(state, first_point);
  splt_point *second_point = mp3splt_point_new(100 * 60 * 1, NULL);
  mp3splt_append_splitpoint(state, second_point);

  //do the effective split
  error = mp3splt_split(state);
  print_confirmation_and_exit_if_error(state, error);

  //free the memory of the main state
  mp3splt_free_state(state);

  return EXIT_SUCCESS;
}

