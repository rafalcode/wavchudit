/* example_c_encode_file - Simple FLAC file encoder using libFLAC
 * Copyright (C) 2007  Josh Coalson
 * mod rf
 *
 * Notes: I'm gathering now that flac is predominantly big endian */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FLAC/metadata.h"
#include "FLAC/stream_encoder.h"
// #include "FLAC/ordinals.h" /* has defns of u64 etc */

#define READSIZE 1024

/* globals */
static FLAC__byte buffer[READSIZE/*samples*/ * 2/*bytes_per_sample*/ * 2/*channels*/]; /* we read the WAVE data into here */
static FLAC__int32 pcm[READSIZE/*samples*/ * 2/*channels*/];

void progress_callback(const FLAC__StreamEncoder *encoder, FLAC__uint64 bytes_written, FLAC__uint64 samples_written, unsigned frames_written, unsigned total_frames_estimate, unsigned total_samples, void *client_data)
{
	(void)encoder, (void)client_data;

	fprintf(stderr, "wrote %llu bytes, %llu/%u samples, %u/%u frames\n", (long long unsigned int)bytes_written, (long long unsigned int)samples_written, total_samples, frames_written, total_frames_estimate);
}

int main(int argc, char *argv[])
{
	FLAC__StreamEncoderInitStatus init_status;
	FLAC__StreamMetadata *metadata[2];
	FILE *fin;

	if(argc != 3) {
		fprintf(stderr, "usage: %s infile.wav outfile.flac\n", argv[0]);
		return 1;
	}

	if((fin = fopen(argv[1], "rb")) == NULL) {
		fprintf(stderr, "ERROR: opening %s for output\n", argv[1]);
		return 1;
	}

	/* read wav header and validate it, note everything is put into the byte-typed buffer array */
	if(
		fread(buffer, 1, 44, fin) != 44 ||
		memcmp(buffer, "RIFF", 4) ||
		memcmp(buffer+8, "WAVEfmt \020\000\000\000\001\000\002\000", 16) ||
		memcmp(buffer+32, "\004\000\020\000data", 8)
	) {
		fprintf(stderr, "ERROR: invalid/unsupported WAVE file, only 16bps stereo WAVE in canonical form allowed\n");
		fclose(fin);
		return 1;
	}
   
	/* declare and allocate the encoder */
	FLAC__StreamEncoder *encoder = 0;
	if((encoder = FLAC__stream_encoder_new()) == NULL) {
		fprintf(stderr, "ERROR: allocating encoder\n");
		fclose(fin);
		return 1;
	}

	FLAC__bool ok = true;
	ok &= FLAC__stream_encoder_set_verify(encoder, true);
	ok &= FLAC__stream_encoder_set_compression_level(encoder, 5);

	unsigned channels = 2;
	ok &= FLAC__stream_encoder_set_channels(encoder, channels);

	unsigned bps = 16;
	ok &= FLAC__stream_encoder_set_bits_per_sample(encoder, bps);

	unsigned sample_rate = ((((((unsigned)buffer[27] << 8) | buffer[26]) << 8) | buffer[25]) << 8) | buffer[24];
	ok &= FLAC__stream_encoder_set_sample_rate(encoder, sample_rate);

        unsigned total_samples = (((((((unsigned)buffer[43] << 8) | buffer[42]) << 8) | buffer[41]) << 8) | buffer[40]) / 4;
	ok &= FLAC__stream_encoder_set_total_samples_estimate(encoder, total_samples);

	/* now add some metadata; we'll add some tags and a padding block */
	FLAC__StreamMetadata_VorbisComment_Entry entry;
	if(ok) {
		if(
			(metadata[0] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT)) == NULL ||
			(metadata[1] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING)) == NULL ||
			/* there are many tag (vorbiscomment) functions but these are convenient for this particular use: */
			// !FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "ARTIST", "Any Old Artist") ||
			!FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "ALBUM", "Some Album") ||
			!FLAC__metadata_object_vorbiscomment_append_comment(metadata[0], entry, /*copy=*/false) || /* copy=false: let metadata object take control of entry's allocated string */
			!FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, "YEAR", "1984") ||
			!FLAC__metadata_object_vorbiscomment_append_comment(metadata[0], entry, /*copy=*/false)
		) {
			fprintf(stderr, "ERROR: out of memory or tag error\n");
			ok = false;
		}

		metadata[1]->length = 1234; /* set the padding length */

		ok = FLAC__stream_encoder_set_metadata(encoder, metadata, 2);
	}

	/* initialize encoder */
	if(ok) {
		// init_status = FLAC__stream_encoder_init_file(encoder, argv[2], progress_callback, /*client_data=*/NULL);
        // verfied that NULL instead of progress_callback is allowed and quite acceptable.
		init_status = FLAC__stream_encoder_init_file(encoder, argv[2], NULL, /*client_data=*/NULL);
		if(init_status != FLAC__STREAM_ENCODER_INIT_STATUS_OK) {
			fprintf(stderr, "ERROR: initializing encoder: %s\n", FLAC__StreamEncoderInitStatusString[init_status]);
			ok = false;
		}
	}

	/* read blocks of samples from WAVE file and feed to encoder */
	if(ok) {
		size_t left = (size_t)total_samples;
		while(ok && left) {
			size_t need = (left>READSIZE? (size_t)READSIZE : (size_t)left);
			if(fread(buffer, channels*(bps/8), need, fin) != need) {
				fprintf(stderr, "ERROR: reading from WAVE file\n");
				ok = false;
			} else {
				/* convert the packed little-endian 16-bit PCM samples from WAVE into an interleaved FLAC__int32 buffer for libFLAC */
				size_t i;
				for(i = 0; i < need*channels; i++) /* inefficient but simple and works on big- or little-endian machines */
					pcm[i] = (FLAC__int32)(((FLAC__int16)(FLAC__int8)buffer[2*i+1] << 8) | (FLAC__int16)buffer[2*i]);

				/* feed samples to encoder */
				ok = FLAC__stream_encoder_process_interleaved(encoder, pcm, need);
			}
			left -= need;
		}
	}

	ok &= FLAC__stream_encoder_finish(encoder);

    /* print end states: */
	fprintf(stderr, "encoding: %s\n", ok? "succeeded" : "FAILED");
	fprintf(stderr, "   state: %s\n", FLAC__StreamEncoderStateString[FLAC__stream_encoder_get_state(encoder)]);
    /* should get:
    encoding: succeeded
    state: FLAC__STREAM_ENCODER_UNINITIALIZED
    if theendcoderstate is queried before finishing it, you get:
    FLAC__STREAM_ENCODER_OK
    At this stage of the procedure, this is what we want I reckon
    */

    /* now that encoding is finished, the metadata can be freed */
    FLAC__metadata_object_delete(metadata[0]);
    FLAC__metadata_object_delete(metadata[1]);

    FLAC__stream_encoder_delete(encoder);
    fclose(fin);

    return 0;
}
