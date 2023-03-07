/* originally from "equalarea Paul"
http://equalarea.com/paul/alsa-audio.html

as he puts it:

A typical audio application has this rough structure:

open_the_device();
set_the_parameters_of_the_device();
while (!done) {
// one or both of these
receive_audio_data_from_the_device();
deliver_audio_data_to_the_device();
}
close the device

Here "err" is a signed integer, and errors returns as negative integers!

However he had a number of errors.
- samprate has to be fed in as a pointer.

SO he' probably out of date.
In any case the example in alsa source (pcm.c) is probably a good deal better.
I've copied that file pcm.c to alpcm.c here.

 *
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alsa/asoundlib.h>

#define DESIREDSAMPRATE 48000
#define CHANQUAN 2 // 2 for stereo
#define BUFSIZE 12000 // the sound payload
#define BUFTIMES 10

static char *hwdevice = "plughw:2,0";			/* playback device */
// static char *hwdevice = "pulse:DEVICE=Virtual_Sink_2";
// static char *hwdevice = "pulse";

int main(int argc, char *argv[])
{
    int i;
    int err;
    short buf[BUFSIZE];
    snd_pcm_t *playback_handle;
    unsigned mysamprate = DESIREDSAMPRATE;

    // 1. trying opening device on current system, error out if not
    err = snd_pcm_open(&playback_handle, hwdevice, SND_PCM_STREAM_PLAYBACK, 0);
    if(err < 0) {
        fprintf (stderr, "cannot open audio device %s (%s)\n", hwdevice, snd_strerror (err));
        exit(EXIT_FAILURE);
    }

    // 2. test out the programs hardware parameters via memory alloc
    snd_pcm_hw_params_t *hw_params;
    err = snd_pcm_hw_params_malloc(&hw_params);
    if(err <0 ) {
        fprintf(stderr, "cannot allocate hardware parameter structure (%s)\n", snd_strerror (err));
        exit(EXIT_FAILURE);
    }

    // 3. so we have two valid variables now (you could say, "memory allocated"), so we can proceed to set the initial values i.e. "initialize"
    err = snd_pcm_hw_params_any(playback_handle, hw_params);
    if(err < 0) {
        fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n", snd_strerror (err));
        exit(EXIT_FAILURE);
    }

    /* access types: I didn't know about these, could this be a stereo issue? */
    err = snd_pcm_hw_params_set_access(playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if(err < 0) {
        fprintf (stderr, "cannot set access type (%s)\n", snd_strerror (err));
        exit(EXIT_FAILURE);
    }

    /* 4. OK, now we're in more operative stuff .. what PCM format? The conventional: signed 16bits (shorts) and little endian */
    err = snd_pcm_hw_params_set_format (playback_handle, hw_params, SND_PCM_FORMAT_S16_LE);
    if(err < 0) {
        fprintf (stderr, "cannot set sample format (%s)\n", snd_strerror (err));
        exit(EXIT_FAILURE);
    }

    // Sample rate see definitions above, defaults are the conventional
    err = snd_pcm_hw_params_set_rate_near(playback_handle, hw_params, &mysamprate, 0); // dunno what final zero is for.
    if(err < 0) {
        fprintf (stderr, "cannot set sample rate (%s)\n", snd_strerror (err));
        exit(EXIT_FAILURE);
    }

    // 6. num chans .. 2 for stereo ... things could be alot simpler with mono
    err = snd_pcm_hw_params_set_channels (playback_handle, hw_params, CHANQUAN);
    if(err < 0) {
        fprintf (stderr, "cannot set channel count (%s)\n", snd_strerror (err));
        exit(EXIT_FAILURE);
    }

    // 7. those last 4 have been about parameter setting. Initialization just ste everything to zero, we want to set above values now.
    err = snd_pcm_hw_params (playback_handle, hw_params);
    if(err < 0) {
        fprintf (stderr, "cannot set parameters (%s)\n", snd_strerror (err));
        exit(EXIT_FAILURE);
    }

    // why is this occurring here?
    snd_pcm_hw_params_free (hw_params);

    err = snd_pcm_prepare (playback_handle);
    if(err < 0) {
        fprintf (stderr, "cannot prepare audio interface for use (%s)\n", snd_strerror (err));
        exit(EXIT_FAILURE);
    }

    int properwritei; // for clarity, a new variable instead of his err. writei(): Write to interface.
    for (i=0; i<BUFTIMES; ++i) {
        properwritei = snd_pcm_writei(playback_handle, buf, BUFSIZE);
        // device = pulse survives up to above line, then we get segmentaion fault.
        // actually all that error checking should have prevented that.
        if(properwritei != BUFSIZE) {
            printf("i=%i | bufwritten=%i | bufsize=%i\n", i, properwritei, BUFSIZE); 
            fprintf (stderr, "Complete write to audio interface failed (%s)\n", snd_strerror (properwritei));
            exit(EXIT_FAILURE);
        }
    }

    snd_pcm_close (playback_handle);
    return 0;
}
