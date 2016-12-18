/* Copyright (C) 1999-2011 Erik de Castro Lopo <erikd@mega-nerd.com>
 * mod rf.
 */
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<math.h>

#include	<sndfile.h>

#ifndef		M_PI
#define		M_PI		3.14159265358979323846264338
#endif

#define		SAMPLE_RATE			44100
#define     SAMPD   3 /* sample duration */
#define		SAMPLE_COUNT		(SAMPLE_RATE * SAMPD)
#define		LEFT_FREQ			(110.0 / SAMPLE_RATE)
#define		RIGHT_FREQ			(160.0 / SAMPLE_RATE)

/* Amplitude considerations: has got to be 4 million or higher to get anything.*/
// #define		AMPL			(1.0 * 0x7F000000) /* thi sis the AMPL which will make sine vary from -1 to +1 */
#define		AMPL			(1.0 * 0x7FFF) /* thi sis the AMPL which will make sine vary from -1 to +1 */
// #define		AMPL			4000000.0

int main (void)
{
    SNDFILE	*file ;
    SF_INFO	sfinfo ;
    int		k ;
    int	*buffer ; /* note this is our value holder array, note especially, it's an int */

    if (! (buffer = malloc (2 * SAMPLE_COUNT * sizeof (int)))) { /* the two for the number of channels */
        printf ("Malloc failed.\n") ;
        exit(EXIT_FAILURE);
    }

    memset (&sfinfo, 0, sizeof (sfinfo)) ;

    sfinfo.samplerate	= SAMPLE_RATE;
//    sfinfo.frames		= SAMPLE_COUNT;
    sfinfo.channels		= 2;
    sfinfo.format		= (SF_FORMAT_WAV | SF_FORMAT_PCM_16);

    if (! (file = sf_open ("sin00.wav", SFM_WRITE, &sfinfo))) {	
        printf ("Error : Not able to open output file.\n") ;
        exit(EXIT_FAILURE);
    }

    float TWOPIL=LEFT_FREQ*2*M_PI;
    float TWOPIR=RIGHT_FREQ*2*M_PI;
    for (k = 0 ; k < SAMPLE_COUNT ; k++) {
//         buffer [2 * k] = AMPL * 2 * ( TWOPIL*k - floor(.5+TWOPIL*k));
//         buffer [2 * k+1] = AMPL * 2 * ( TWOPIR*k - floor(.5+TWOPIR*k));
        buffer [2 * k] = AMPL * sin (TWOPIL*k);
        buffer [2 * k + 1] = AMPL * sin (TWOPIR*k);
    }

    if (sf_write_int (file, buffer, sfinfo.channels * SAMPLE_COUNT) != sfinfo.channels * SAMPLE_COUNT)
        puts (sf_strerror (file));

    sf_close (file) ;
    printf("ampl is %f\n", AMPL); 
    return	 0 ;
}
