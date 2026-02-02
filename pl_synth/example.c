#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#define PL_SYNTH_IMPLEMENTATION
#include "pl_synth.h"


// WAV writer ------------------------------------------------------------------

#define CHUNK_ID(S) \
	(((unsigned int)(S[3])) << 24 | ((unsigned int)(S[2])) << 16 | \
	 ((unsigned int)(S[1])) <<  8 | ((unsigned int)(S[0])))

// le for little endian
void fwrite_u32_le(unsigned int v, FILE *fh) {
	uint8_t buf[sizeof(unsigned int)];
	buf[0] = 0xff & (v      );
	buf[1] = 0xff & (v >>  8);
	buf[2] = 0xff & (v >> 16);
	buf[3] = 0xff & (v >> 24);
	int wrote = fwrite(buf, sizeof(unsigned int), 1, fh);
	assert(wrote);
}

void fwrite_u16_le(unsigned short v, FILE *fh) {
	uint8_t buf[sizeof(unsigned short)];
	buf[0] = 0xff & (v      );
	buf[1] = 0xff & (v >>  8);
	int wrote = fwrite(buf, sizeof(unsigned short), 1, fh);
	assert(wrote);
}

int wav_write(const char *path, short *samples, int samples_len, short channels, int samplerate) {
	unsigned int data_size = samples_len * channels * sizeof(short);
	short bits_per_sample = 16;

	/* Lifted from https://www.jonolick.com/code.html - public domain
	Made endian agnostic using fwrite() */
	FILE *fh = fopen(path, "wb");
	assert(fh);
	fwrite("RIFF", 1, 4, fh);
	fwrite_u32_le(data_size + 44 - 8, fh);
	fwrite("WAVEfmt \x10\x00\x00\x00\x01\x00", 1, 14, fh);
	fwrite_u16_le(channels, fh);
	fwrite_u32_le(samplerate, fh);
	fwrite_u32_le(channels * samplerate * bits_per_sample/8, fh);
	fwrite_u16_le(channels * bits_per_sample/8, fh);
	fwrite_u16_le(bits_per_sample, fh);
	fwrite("data", 1, 4, fh);
	fwrite_u32_le(data_size, fh);
	fwrite((void*)samples, data_size, 1, fh);
	fclose(fh);
	return data_size  + 44 - 8;
}



// Song data -------------------------------------------------------------------

// #include "azaday-test-song.h"
// #include "drop-remake.h"
// #include "quake-remake.h"
#include "frank.h"

// drop arp
// pl_synth_song_t song = {
// 	.row_len = 8481,
// 	.num_tracks = 1,
// 	.tracks = (pl_synth_track_t[]){
// 		{
// 			.synth = {7,0,0,0,121,1,7,0,0,0,91,3,0,100,1212,5513,100,0,6,19,3,121,6,21,0,1,1,29},
// 			.sequence_len = 2,
// 			.sequence = (uint8_t[]){1,2},
// 			.patterns = (pl_synth_pattern_t[]){
// 				{.notes = {138,145,138,150,138,145,138,150,138,145,138,150,138,145,138,150,136,145,138,148,136,145,138,148,136,145,138,148,136,145,138,148}},
// 				{.notes = {135,0,138,147,135,145,138,147,135,145,138,147,135,145,138,147,135,143,138,146,135,143,138,146,135,143,138,146,135,143,138,146}}
// 			}
// 		},
// 	}
// };

// quake bass
// pl_synth_song_t song = {
// 	.row_len = 6014,
// 	.num_tracks = 1,
// 	.tracks = (pl_synth_track_t[]){
// 		{
// 			.synth = {6,0,0,0,192,2,6,0,16,0,192,2,63,55090,75765,79084,247,2,444,255,3,172},
// 			.sequence_len = 18,
// 			.sequence = (uint8_t[]){0,0,2,2,3,4,2,2,3,4,2,2,3,4,2,2,3,4},
// 			.patterns = (pl_synth_pattern_t[]){
// 				{.notes = {}},
// 				{.notes = {132}},
// 				{.notes = {133,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128}},
// 				{.notes = {125}}
// 			}
// 		},
// 	}
// };

// pl_synth_song_t song = {
// 	.row_len = 8481,
// 	.num_tracks = 1,
// 	.tracks = (pl_synth_track_t[]){
// 		{
// 			.synth = {7,0,0,0,121,1,7,0,0,0,91,3,0,100,1212,5513,100,0,6,19,3,121,6,21,0,1,1,29},
// 			.sequence_len = 12,
// 			.sequence = (uint8_t[]){1,2,1,2,1,2,0,0,1,2,1,2},
// 			.patterns = (pl_synth_pattern_t[]){
// 				{.notes = {138,145,138,150,138,145,138,150,138,145,138,150,138,145,138,150,136,145,138,148,136,145,138,148,136,145,138,148,136,145,138,148}},
// 				{.notes = {135,145,138,147,135,145,138,147,135,145,138,147,135,145,138,147,135,143,138,146,135,143,138,146,135,143,138,146,135,143,138,146}}
// 			}
// 		},
// 	}
// };

int main(int argc, char **argv) {
	// Initialize the instrument lookup table
	void *synth_tab = malloc(PL_SYNTH_TAB_SIZE);
	pl_synth_init(synth_tab);

	// Determine the number of samples needed for the song
	int num_samples = pl_synth_song_len(&song);
	printf("generating %d samples\n", num_samples);

	// Allocate buffers
	int buffer_size = num_samples * 2 * sizeof(int16_t);
	int16_t *output_samples = malloc(buffer_size);
	int16_t *temp_samples = malloc(buffer_size);


	// Generate realtime
	if (1) { // convert tracks to rt_tracks
		pl_synth_rt_track_t rt_tracks[song.num_tracks];
		memset(rt_tracks, 0, sizeof(rt_tracks));

		for (int i = 0; i < song.num_tracks; ++i) {
			rt_tracks[i].track = song.tracks[i];
			printf("track %d, sequence len: %d\n", i, rt_tracks[i].track.sequence_len);
			// alloc memory for delay buff
				// calculate delay exactly
				// pl_synth_t* synth = &song.tracks[i].synth;
				// int delay_shift = (synth->fx_delay_time * song.row_len) / 2;
				// float delay_amount = synth->fx_delay_amt / 255.0;
				// float delay_iter = ceilf(logf(0.1) / logf(delay_amount));
				// int delay_len_samps = (delay_iter * delay_shift) + 1
			// simpler: just max out delay to 32 rows
			int delay_samps = 32 * song.row_len;
			rt_tracks[i].delay_line_MALLOC = calloc(delay_samps * 2, sizeof(int16_t));
			rt_tracks[i].delay_line_size = delay_samps;
		}

		for (int i = 0; i < num_samples; i++) {
			for (int track_idx = 0; track_idx < song.num_tracks; track_idx++) {
				pl_synth_track_tick(rt_tracks + track_idx, output_samples + 2 * i, song.row_len, i);
				// printf("%d, %d\n", output_samples[2 * i], output_samples[2 * i + 1]);
			}

			// clamp samps from this tick
			output_samples[2*i] = pl_synth_clamp_s16(output_samples[2*i]);
			output_samples[2*i+1] = pl_synth_clamp_s16(output_samples[2*i+1]);
		}
	}
	// Generate offline
	else pl_synth_song(&song, output_samples, temp_samples);

	// Temp buffer not needed anymore
	free(temp_samples);

	// Write the generated samples to example.wav
	wav_write("try.wav", output_samples, num_samples, 2, 44100);

	free(output_samples);
	free(synth_tab);

	return 0;
}
