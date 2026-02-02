/*

Copyright (c) 2024, Dominic Szablewski - https://phoboslab.org
SPDX-License-Identifier: MIT

Based on Sonant, published under the Creative Commons Public License
(c) 2008-2009 Jake Taylor [ Ferris / Youth Uprising ] 


-- Synopsis

// Define `PL_SYNTH_IMPLEMENTATION` in *one* C/C++ file before including this
// library to create the implementation.

#define PL_SYNTH_IMPLEMENTATION
#include "pl_synth.h"

// Initialize the lookup table for the oscillators
void *synth_tab = malloc(PL_SYNTH_TAB_SIZE);
pl_synth_init(synth_tab);

// A sound is described by an instrument (synth), the row_len in samples and
// a note.
pl_synth_sound_t sound = {
	.synth = {7,0,0,0,192,0,7,0,0,0,192,0,0,200,2000,20000,192}, 
	.row_len = 5168, 
	.note = 135
};

// Determine the number of the samples for a sound effect and allocate the
// sample buffer for both (stereo) channels
int num_samples = pl_synth_sound_len(&sound);
uint16_t *sample_buffer = malloc(num_samples * 2 * sizeof(uint16_t));

// Generate the samples
pl_synth_sound(&sound, sample_buffer);

See below for a documentation of all functions exposed by this library.


*/

#ifndef PL_SYNTH_H
#define PL_SYNTH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
extern int PL_SYNTH_SAMPLERATE; // azaday support variable srate
#define PL_SYNTH_TAB_LEN 4096
#define PL_SYNTH_TAB_SIZE (sizeof(float) * PL_SYNTH_TAB_LEN * 4)

typedef struct {
	uint8_t osc0_oct;
	uint8_t osc0_det;
	uint8_t osc0_detune;
	uint8_t osc0_xenv;
	uint8_t osc0_vol;
	uint8_t osc0_waveform;

	uint8_t osc1_oct;
	uint8_t osc1_det;
	uint8_t osc1_detune;
	uint8_t osc1_xenv;
	uint8_t osc1_vol;
	uint8_t osc1_waveform;

	uint8_t noise_fader;

	uint32_t env_attack;
	uint32_t env_sustain;
	uint32_t env_release;
	uint32_t env_master;

	uint8_t fx_filter;
	uint32_t fx_freq;
	uint8_t fx_resonance;
	uint8_t fx_delay_time;
	uint8_t fx_delay_amt;
	uint8_t fx_pan_freq;
	uint8_t fx_pan_amt;

	uint8_t lfo_osc_freq;
	uint8_t lfo_fx_freq;
	uint8_t lfo_freq;
	uint8_t lfo_amt;
	uint8_t lfo_waveform;
} pl_synth_t;

typedef struct {
	pl_synth_t synth;
	uint32_t row_len;
	uint8_t note;
} pl_synth_sound_t;

typedef struct {
	uint8_t notes[32];
} pl_synth_pattern_t;

typedef struct {
	pl_synth_t synth;
	uint32_t sequence_len;
	uint8_t *sequence;
	pl_synth_pattern_t *patterns;
} pl_synth_track_t;

// azaday
// stores the state of pl_synth_gen so we can step it 1 sample at a time
typedef struct {
	int note; // last nonzero note

	// local variables of pl_synth_gen(...)
	// OUTSIDE of inner loop
	// updated once per note change
	float fx_pan_freq;
	float lfo_freq;
	double osc0_pos;
	double osc1_pos;

	float fx_resonance;
	float noise_vol;

	float low;
	float band;
	float high;

	float inv_attack;
	float inv_release;
	float lfo_amt;
	float pan_amt;

	float osc0_freq;
	float osc1_freq;

	int num_samples;

	int j;
} pl_synth_rt_note_t;




// azaday added
typedef struct {
	pl_synth_track_t track;

	int sequence_idx; // position in sequence array
	int note_idx;     // 0-31 position in pattern

	int16_t* delay_line_MALLOC; // for delay. realloced on tempo change
	int delay_line_size; // HALF the delay_line_MALLOC size (bc stereo)
	int delay_line_write_idx;


	// quirk about pl_synth: renders entire note into buffer at once,
	// so if a note takes longer than the time until the next note,
	// the instrument becomes polyphonic.
	// accurate emulation would require keeping a buffer of "active notes"
	// like so:
	pl_synth_rt_note_t active_notes[32];
	uint8_t num_active_notes;

	// for now i am assuming monophonic, only 1 active at a time
	// updates to sound happen on the granularity of *note* NOT sample
	// pl_synth_rt_note_t active_note;
} pl_synth_rt_track_t;


typedef struct {
	uint32_t row_len;
	uint8_t num_tracks;
	pl_synth_track_t *tracks;
} pl_synth_song_t;

typedef struct {
	uint32_t row_len;
	uint8_t num_tracks;
	pl_synth_rt_track_t *tracks;
} pl_synth_rt_song_t;

// Initialize the lookup table for all instruments. This needs to be done only
// once. The table will be written to the memory pointed to by tab_buffer, which
// must be PL_SYNTH_TAB_LEN elements long or PL_SYNTH_TAB_SIZE bytes in size.
void pl_synth_init(float *tab_buffer);

// Determine the number of samples needed for one channel of a particular sound
// effect.
int pl_synth_sound_len(pl_synth_sound_t *sound);

// Generate a stereo sound into the buffer pointed to by samples. The buffer 
// must be at least pl_synth_sound_len() * 2 elements long.
int pl_synth_sound(pl_synth_sound_t *sound, int16_t *samples);

// Determine the number of samples needed for one channel of a particular song.
int pl_synth_song_len(pl_synth_song_t *song);

// Generate a stereo song into the buffer pointed to by samples, with temporary
// storage provided to by temp_samples. The buffers samples and temp_samples 
// must each be at least pl_synth_song_len() * 2 elements long.
int pl_synth_song(pl_synth_song_t *song, int16_t *samples, int16_t *temp_samples);

// azaday
// realtime process 1 sample at a time of a track aka instrument
// samp is TOTAL elapsed samps
void pl_synth_track_tick(pl_synth_rt_track_t *track, int16_t *samples, int row_len, int samp);

#ifdef __cplusplus
}
#endif
#endif /* PL_SYNTH_H */



/* -----------------------------------------------------------------------------
	Implementation */

#ifdef PL_SYNTH_IMPLEMENTATION

#include <math.h> // powf, sinf, logf, ceilf
#include <string.h> // memset, memcpy

int PL_SYNTH_SAMPLERATE = 44100; // azaday support variable srate

#define PL_SYNTH_TAB_MASK (PL_SYNTH_TAB_LEN-1)
#define PL_SYNTH_TAB(WAVEFORM, K) pl_synth_tab[WAVEFORM][((int)((K) * PL_SYNTH_TAB_LEN)) & PL_SYNTH_TAB_MASK]

static float *pl_synth_tab[4];
static uint32_t pl_synth_rand = 0xd8f554a5;

void pl_synth_init(float *tab_buffer) {
	for (int i = 0; i < 4; i++) {
		pl_synth_tab[i] = tab_buffer + PL_SYNTH_TAB_LEN * i;
	}

	// sin
	for (int i = 0; i < PL_SYNTH_TAB_LEN; i++) { 
		pl_synth_tab[0][i] = sinf(i*(6.283184f/(float)PL_SYNTH_TAB_LEN));
	}
	// square
	for (int i = 0; i < PL_SYNTH_TAB_LEN; i++) {
		pl_synth_tab[1][i] = pl_synth_tab[0][i] < 0 ? -1 : 1;
	}
	// sawtooth
	for (int i = 0; i < PL_SYNTH_TAB_LEN; i++) {
		pl_synth_tab[2][i] = (float)i / PL_SYNTH_TAB_LEN - 0.5;
	}
	// triangle
	for (int i = 0; i < PL_SYNTH_TAB_LEN; i++) {
		pl_synth_tab[3][i] = i < PL_SYNTH_TAB_LEN/2 
			? (i/(PL_SYNTH_TAB_LEN/4.0)) - 1.0 
			: 3.0 - (i/(PL_SYNTH_TAB_LEN/4.0));
	} 
}

static inline float pl_synth_note_freq(int n, int oct, int semi, int detune) {
	return (0.00390625 * powf(1.059463094, n - 128 + (oct - 8) * 12 + semi)) * (1.0f + 0.0008f * detune);
}

static inline int pl_synth_clamp_s16(int v) {
	if ((unsigned int)(v + 32768) > 65535) {
		if (v < -32768) { return -32768; }
		if (v >  32767) { return  32767; }
	}
	return v;
}

static void pl_synth_gen(int16_t *samples, int write_pos, int row_len, int note, pl_synth_t *s) {
	float fx_pan_freq = powf(2, s->fx_pan_freq - 8) / row_len;
	float lfo_freq = powf(2, s->lfo_freq - 8) / row_len;
	
	// We need higher precision here, because the oscilator positions may be 
	// advanced by tiny values and error accumulates over time
	double osc0_pos = 0;
	double osc1_pos = 0;

	float fx_resonance = s->fx_resonance / 255.0f;
	float noise_vol = s->noise_fader * 4.6566129e-010f;
	float low = 0;
	float band = 0;
	float high = 0;

	float inv_attack = 1.0f / s->env_attack;
	float inv_release = 1.0f / s->env_release;
	float lfo_amt = s->lfo_amt / 512.0f;
	float pan_amt = s->fx_pan_amt / 512.0f;

	float osc0_freq = pl_synth_note_freq(note, s->osc0_oct, s->osc0_det, s->osc0_detune);
	float osc1_freq = pl_synth_note_freq(note, s->osc1_oct, s->osc1_det, s->osc1_detune);

	int num_samples = s->env_attack + s->env_sustain + s->env_release - 1;
	
	for (int j = num_samples; j >= 0; j--) {
		int k = j + write_pos;

		// LFO
		float lfor = PL_SYNTH_TAB(s->lfo_waveform, k * lfo_freq) * lfo_amt + 0.5f;

		float sample = 0;
		float filter_f = s->fx_freq;
		float temp_f;
		float envelope = 1;

		// Envelope
		if (j < s->env_attack) {
			envelope = (float)j * inv_attack;
		}
		else if (j >= s->env_attack + s->env_sustain) {
			envelope -= (float)(j - s->env_attack - s->env_sustain) * inv_release;
		}

		// Oscillator 1
		temp_f = osc0_freq;
		if (s->lfo_osc_freq) {
			temp_f *= lfor;
		}
		if (s->osc0_xenv) {
			temp_f *= envelope * envelope;
		}
		osc0_pos += temp_f;
		sample += PL_SYNTH_TAB(s->osc0_waveform, osc0_pos) * s->osc0_vol;

		// Oscillator 2
		temp_f = osc1_freq;
		if (s->osc1_xenv) {
			temp_f *= envelope * envelope;
		}
		osc1_pos += temp_f;
		sample += PL_SYNTH_TAB(s->osc1_waveform, osc1_pos) * s->osc1_vol;

		// Noise oscillator
		if (noise_vol) {
			int32_t r = (int32_t)pl_synth_rand;
			sample += (float)r * noise_vol * envelope;
			pl_synth_rand ^= pl_synth_rand << 13;
			pl_synth_rand ^= pl_synth_rand >> 17;
			pl_synth_rand ^= pl_synth_rand << 5;
		}

		sample *= envelope * (1.0f / 255.0f);

		// State variable filter
		if (s->fx_filter) {
			if (s->lfo_fx_freq) {
				filter_f *= lfor;
			}

			filter_f = PL_SYNTH_TAB(0, filter_f * (0.5f / PL_SYNTH_SAMPLERATE)) * 1.5f;
			low += filter_f * band;
			high = fx_resonance * (sample - band) - low;
			band += filter_f * high;
			sample = (float[5]){sample, high, low, band, low + high}[s->fx_filter];
		}

		// Panning & master volume
		temp_f = PL_SYNTH_TAB(0, k * fx_pan_freq) * pan_amt + 0.5f;
		sample *= 78 * s->env_master;


		samples[k * 2 + 0] += sample * (1-temp_f);
		samples[k * 2 + 1] += sample * temp_f;
	}
}

static void pl_synth_apply_delay(int16_t *samples, int len, int shift, float amount) {
	int len_2 = len * 2;
	int shift_2 = shift * 2;
	for (int i = 0, j = shift_2; j < len_2; i += 2, j += 2) {
		samples[j + 0] += samples[i + 1] * amount;
		samples[j + 1] += samples[i + 0] * amount;
	}
}

static int pl_synth_instrument_len(pl_synth_t *synth, int row_len) {
	int delay_shift = (synth->fx_delay_time * row_len) / 2;
	float delay_amount = synth->fx_delay_amt / 255.0;
	float delay_iter = ceilf(logf(0.1) / logf(delay_amount));
	return synth->env_attack + 
		synth->env_sustain + 
		synth->env_release + 
		delay_iter * delay_shift;
}

int pl_synth_sound_len(pl_synth_sound_t *sound) {
	return pl_synth_instrument_len(&sound->synth, sound->row_len);
}

int pl_synth_sound(pl_synth_sound_t *sound, int16_t *samples) {
	int len = pl_synth_sound_len(sound);
	pl_synth_gen(samples, 0, sound->row_len, sound->note, &sound->synth);

	if (sound->synth.fx_delay_amt) {
		int delay_shift = (sound->synth.fx_delay_time * sound->row_len) / 2;
		float delay_amount = sound->synth.fx_delay_amt / 256.0;
		pl_synth_apply_delay(samples, len, delay_shift, delay_amount);
	}

	return len;
}

static int pl_synth_track_curr_note(pl_synth_rt_track_t* rt_track) {
	pl_synth_track_t* track = &rt_track->track;
	if (rt_track->sequence_idx >= track->sequence_len)  return 0;
	int pattern = track->sequence[rt_track->sequence_idx];
	if (pattern == 0) return 0;
	int note = track->patterns[pattern - 1].notes[rt_track->note_idx - 1];
	return note;
}

void pl_synth_track_tick(pl_synth_rt_track_t *track, int16_t *samples, int row_len, int samp) {
	pl_synth_t* s = &track->track.synth;
	int16_t left = 0;
	int16_t right = 0;

	// logic for if we hit new note_on
	int new_row = (samp % row_len == 0);
	// printf("row_len: %d, j: %d\n", row_len, track->active_note.j);
	if (new_row) {
		// printf("row_len: %d, j: %d\n", row_len, track->active_note.j);
		// wrap around
		track->note_idx++;
		if (track->note_idx > 32) {
			track->note_idx = 1;
			track->sequence_idx++;
		}

		if (track->sequence_idx == track->track.sequence_len) {
			// TODO handle looping
		}

		int note = pl_synth_track_curr_note(track);
		// printf("right after ret %d", a->note);
		if (note) {
			// get new active note
			pl_synth_rt_note_t* active_note = &track->active_notes[track->num_active_notes++];
			// printf("note: %d\n", note);

			// zero everything in active note except write_pos;
			memset(active_note, 0, sizeof(*active_note));

			// reinit outer loop variables
			active_note->note = note;
			active_note->fx_pan_freq = powf(2, s->fx_pan_freq - 8) / row_len;
			active_note->lfo_freq = powf(2, s->lfo_freq - 8) / row_len;
			active_note->fx_resonance = s->fx_resonance / 255.0f;
			active_note->noise_vol = s->noise_fader * 4.6566129e-010f;
			active_note->inv_attack = 1.0f / s->env_attack;
			active_note->inv_release = 1.0f / s->env_release;
			active_note->lfo_amt = s->lfo_amt / 512.0f;
			active_note->pan_amt = s->fx_pan_amt / 512.0f;
			active_note->osc0_freq = pl_synth_note_freq(note, s->osc0_oct, s->osc0_det, s->osc0_detune);
			active_note->osc1_freq = pl_synth_note_freq(note, s->osc1_oct, s->osc1_det, s->osc1_detune);
			active_note->num_samples = s->env_attack + s->env_sustain + s->env_release - 1;
		}
		// printf("wtf: %d\n", track->track.patterns[0].notes[0]);
	}

	// iterate over active notes and synthesize audio for current tick
	// (go backwards so swap-delete doesn't mess up iteration)
	for (int active_note_idx = track->num_active_notes-1; active_note_idx >= 0; --active_note_idx) {
		pl_synth_rt_note_t* a = &track->active_notes[active_note_idx];

		// reached end of note envelope, remove from active list
		if (a->j >= a->num_samples) {
			memcpy(a, &track->active_notes[--track->num_active_notes], sizeof(*a));
			continue;
		}
		
		// synthesize 1 sample, exluding delay line
		{
			uint64_t k = samp;

			// LFO
			float lfor = PL_SYNTH_TAB(s->lfo_waveform, k * a->lfo_freq) * a->lfo_amt + 0.5f;

			float sample = 0;
			float filter_f = s->fx_freq;
			float temp_f;
			float envelope = 1;

			// Envelope
			if (a->j < s->env_attack) {
				envelope = (float)a->j * a->inv_attack;
			}
			else if (a->j >= s->env_attack + s->env_sustain) {
				envelope -= (float)(a->j - s->env_attack - s->env_sustain) * a->inv_release;
			}

			// Oscillator 1
			temp_f = a->osc0_freq;
			if (s->lfo_osc_freq) {
				temp_f *= lfor;
			}
			if (s->osc0_xenv) {
				temp_f *= envelope * envelope;
			}
			a->osc0_pos += temp_f;
			sample += PL_SYNTH_TAB(s->osc0_waveform, a->osc0_pos) * s->osc0_vol;

			// Oscillator 2
			temp_f = a->osc1_freq;
			if (s->osc1_xenv) {
				temp_f *= envelope * envelope;
			}
			a->osc1_pos += temp_f;
			sample += PL_SYNTH_TAB(s->osc1_waveform, a->osc1_pos) * s->osc1_vol;

			// Noise oscillator
			if (a->noise_vol) {
				int32_t r = (int32_t)pl_synth_rand;
				sample += (float)r * a->noise_vol * envelope;
				pl_synth_rand ^= pl_synth_rand << 13;
				pl_synth_rand ^= pl_synth_rand >> 17;
				pl_synth_rand ^= pl_synth_rand << 5;
			}

			sample *= envelope * (1.0f / 255.0f);

			// State variable filter
			if (s->fx_filter) {
				if (s->lfo_fx_freq) {
					filter_f *= lfor;
				}

				filter_f = PL_SYNTH_TAB(0, filter_f * (0.5f / PL_SYNTH_SAMPLERATE)) * 1.5f;
				a->low += filter_f * a->band;
				a->high = a->fx_resonance * (sample - a->band) - a->low;
				a->band += filter_f * a->high;
				sample = (float[5]){sample, a->high, a->low, a->band, a->low + a->high}[s->fx_filter];
			}

			// Panning & master volume
			temp_f = PL_SYNTH_TAB(0, k * a->fx_pan_freq) * a->pan_amt + 0.5f;
			sample *= 78 * s->env_master;

			left += sample * (1-temp_f);
			right += sample * temp_f;
		}

		// bump note sample count
		++a->j;
	}

	// apply delay
	if (track->track.synth.fx_delay_amt) {
		int delay_shift = (track->track.synth.fx_delay_time * row_len) / 2;
		float delay_amount = track->track.synth.fx_delay_amt / 255.0;

		int delay_idx = track->delay_line_write_idx - delay_shift;
		if (delay_idx < 0) delay_idx += track->delay_line_size;

		// printf("delay_idx : %d, delay_shift: %d, delay_line_size: %d, delay_amount: %f\n", delay_idx, delay_shift, track->delay_line_size, delay_amount);

		// indices flipped for ping pong effect
		left += track->delay_line_MALLOC[2 * delay_idx + 1] * delay_amount;
		right += track->delay_line_MALLOC[2 * delay_idx+ 0] * delay_amount;
	}

	// clamp
	left = pl_synth_clamp_s16(left);
	right = pl_synth_clamp_s16(right);

	// write to delay line
	track->delay_line_MALLOC[2*track->delay_line_write_idx + 0] = left;
	track->delay_line_MALLOC[2*track->delay_line_write_idx + 1] = right;

	// write to output
	samples[0] += left;
	samples[1] += right;
	
	// wrap write head
	if (++track->delay_line_write_idx >= track->delay_line_size) {
		track->delay_line_write_idx = 0; 
	}
}

int pl_synth_song_len(pl_synth_song_t *song) {
	int num_samples = 0;
	for (int t = 0; t < song->num_tracks; t++) {
		int track_samples = song->tracks[t].sequence_len * song->row_len * 32 +
			pl_synth_instrument_len(&song->tracks[t].synth, song->row_len);

		if (track_samples > num_samples) {
			num_samples = track_samples;
		}
	}
	
	return num_samples;
}

int pl_synth_song(pl_synth_song_t *song, int16_t *samples, int16_t *temp_samples) {
	int len = pl_synth_song_len(song);
	int len_2 = len * 2;
	memset(samples, 0, sizeof(int16_t) * len_2);

	for (int t = 0; t < song->num_tracks; t++) {
		pl_synth_track_t *track = &song->tracks[t];
		memset(temp_samples, 0, sizeof(int16_t) * len_2);

		for (int si = 0; si < track->sequence_len; si++) {
			int write_pos = song->row_len * si * 32;
			int pi = track->sequence[si];
			if (pi > 0) {
				unsigned char *pattern = track->patterns[pi-1].notes;
				for (int row = 0; row < 32; row++) {
					int note = pattern[row];
					if (note > 0) {
						pl_synth_gen(temp_samples, write_pos, song->row_len, note, &track->synth);
					}
					write_pos += song->row_len;
				}
			}
		}
		
		if (track->synth.fx_delay_amt) {
			int delay_shift = (track->synth.fx_delay_time * song->row_len) / 2;
			float delay_amount = track->synth.fx_delay_amt / 255.0;
			pl_synth_apply_delay(temp_samples, len, delay_shift, delay_amount);
		}

		for (int i = 0; i < len_2; i++) {
			samples[i] = pl_synth_clamp_s16(samples[i] + (int)temp_samples[i]);
		}
	}
	return len;
}


#endif /* PL_SYNTH_IMPLEMENTATION */
