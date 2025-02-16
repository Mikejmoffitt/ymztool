#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "3rdparty/inih/ini.h"
#include "3rdparty/dr_wav/dr_wav.h"
#include "3rdparty/adpcm/ymz_codec.h"
#include <ctype.h>

#define YMZ280B_CLOCK_NOMINAL 16934400

#define YMZ_BLOB_ENTRY_SIZE 16

// TODO: Either merge down to mono, create two separate entries for L and R, or
//       entirely reject Stereo data. The chip has panning support, but does not
//       really support stereo data per se.

typedef enum YmzFmt
{
	FMT_NG,
	FMT_ADPCM,
	FMT_PCM8,
	FMT_PCM16,
} YmzFmt;

typedef struct Info
{
	// Conversion params
	char src[256];           // Source filename.
	char symbol[256];        // Symbol name as enumerated
	char symbol_upper[256];

	// Source information about the file, unused in Conv context.
	uint32_t sample_rate;   // Sampling rate.

	// Playback information. This can be set in the INI but wav smpl data will overwrite it.
	int loop_start_pos;  // Default to start pos.
	int loop_end_pos;    // Default to end pos.

	// Destination information.
	uint32_t data_offs;       // Offset within data block.

	// YMZ-specific data
	YmzFmt fmt;          // Target format setting.
	uint32_t clock;      // Clock (in Hz)
	int tl;
	int panpot;
	bool loop;
} Info;

typedef struct Entry Entry;
struct Entry
{
	Entry *next;
	int id;
	Info info;

	uint8_t *data;
	uint32_t data_bytes;
	uint32_t length;        // Sample count (per channel)
	int channels;           // Only accepts 1 or 2.  // TODO: Entirely reject 2

	uint32_t start_address;
	uint32_t end_address;
	uint32_t loop_start_address;
	uint32_t loop_end_address;

	uint16_t fn_reg;     // fn reg value to play this back (assuming YMZ clock)
	int bits_per_sample;
};

typedef struct Conv
{
	// Linked list of sprites read
	Entry *entry_head;  // First in the entries link list.
	Entry *entry_tail;  // Pointer to the end of the entries list.

	// Config for an entry
	char out[256];           // Output base filename.
	Info info;               // Basic info. Some fields might go unused or ignored.
} Conv;

bool conv_validate(const Conv *s)
{
	if (s->info.symbol[0] == '\0')
	{
		fprintf(stderr, "[CONV] symbol not set!\n");
		return false;
	}
	if (s->info.fmt == FMT_NG)
	{
		fprintf(stderr, "[CONV] fmt NG!\n");
		return false;
	}
	if (s->info.tl < 0 || s->info.tl > 255)
	{
		fprintf(stderr, "[CONV] Invalid TL $%X\n", s->info.tl);
		return false;
	}
	if (s->info.panpot < 0 || s->info.panpot > 0xF)
	{
		fprintf(stderr, "[CONV] Invalid PANPOT $%X\n", s->info.panpot);
		return false;
	}
	if (s->out[0] == '\0')
	{
		fprintf(stderr, "[CONV] output not set!\n");
		return false;
	}
	return true;
}

static bool conv_entry_add(Conv *s)
{
	if (!conv_validate(s)) return false;

	Entry *e = NULL;

	// If this is the first one, create the head
	if (!s->entry_head)
	{
		s->entry_head = calloc(sizeof(*e), 1);
		s->entry_head->id = 0;
		e = s->entry_head;
		s->entry_tail = s->entry_head;
	}
	else
	{
		e = calloc(sizeof(*e), 1);
		e->id = s->entry_tail->id + 1;
		s->entry_tail->next = e;
		s->entry_tail = e;
	}

	// Start by adopting whatever properties have been set by the INI.
	e->info = s->info;

	//
	// Load WAV data into buffer as raw PCM and pull basic data
	//
	const char *fname = e->info.src;
	drwav wav;
	if (!drwav_init_file(&wav, fname, NULL))
	{
		fprintf(stderr, "[CONV] Couldn't load \"%s\"\n", fname);
		drwav_uninit(&wav);
		return false;
	}
	if (wav.channels > 2 || wav.channels < 1)
	{
		fprintf(stderr, "[CONV] Not prepared to handle files with %d channels.\n", wav.channels);
		drwav_uninit(&wav);
		return false;
	}

	// Load data as 16-bit PCM.
	const int src_pcm_buffer_size = wav.totalPCMFrameCount * wav.channels * sizeof(int16_t);
	int16_t *srcpcm = malloc(src_pcm_buffer_size);
	if (!srcpcm)
	{
		fprintf(stderr, "[CONV] Couldn't allocate %d frames of buffer\n",
		        src_pcm_buffer_size);
		drwav_uninit(&wav);
		return false;
	}

	if (!drwav_read_pcm_frames_s16(&wav, wav.totalPCMFrameCount, srcpcm))
	{
		fprintf(stderr, "[CONV] Failed to read PCM frames.\n");
		drwav_uninit(&wav);
		free(srcpcm);
		return false;
	}

	// Source file information
	e->info.sample_rate = wav.sampleRate;
	e->length = wav.totalPCMFrameCount;
	e->channels = wav.channels;

	printf("wav rate $%d, pcm frames %d\n", e->info.sample_rate, e->length);

	// YMZ-specific data is either set from the INI or calculated later.
	// Playback information
	const bool has_smpl_loop = wav.smpl.numSampleLoops > 0;
	if (has_smpl_loop)
	{
		e->info.loop_start_pos = wav.smpl.loops[0].start;
		e->info.loop_end_pos = wav.smpl.loops[0].end;
	}

	if (e->info.loop_start_pos <= 0) e->info.loop_start_pos = 0;
	if (e->info.loop_end_pos <= 0) e->info.loop_end_pos = e->length;

	// Done with the WAV file now.
	drwav_uninit(&wav);

	// TODO: Handle more gracefully in the future.
	if (e->channels > 1)
	{
		fprintf(stderr, "[CONV] Stereo is not presently supported!\n");
		return false;
	}

	// Mark bytes used in data block.
	switch (e->info.fmt)
	{
		default:
			fprintf(stderr, "[CONV] Format NG!\n");
			return false;
		case FMT_ADPCM:
			e->bits_per_sample = e->channels * 8 * sizeof(uint16_t) / 4;  // 16 bits per sample --> 4 bits per sample
			break;
		case FMT_PCM8:
			e->bits_per_sample = e->channels * 8 * sizeof(uint16_t) / 2;  // 16 bits per sample --> 8 bits per sample
			break;
		case FMT_PCM16:
			e->bits_per_sample = e->channels * 8 * sizeof(uint16_t);  // 16 bits per sample
			break;
	}

	e->data_bytes = (e->bits_per_sample * e->length) / 8;
	printf("$%03X %s: %d samples * %d channels @ fmt %d --> %d bits per sample; total %d ($%X) bytes\n",
	       e->id, e->info.symbol_upper,
	       e->length, e->channels, e->info.fmt, e->bits_per_sample, e->data_bytes, e->data_bytes);

	e->data = malloc(e->data_bytes);
	if (!e->data)
	{
		fprintf(stderr, "[CONV] Couldn't allocate %d frames of output buffer\n",
		        e->data_bytes);
		free(srcpcm);
		return false;
	}

	// Calculate addresses. Start and end points are specified not by sample
	// index but by address.
	e->start_address = e->info.data_offs;
	e->end_address = e->start_address + e->data_bytes;

	e->loop_start_address = e->start_address + (e->bits_per_sample * e->info.loop_start_pos) / 8;
	e->loop_end_address =  e->info.data_offs + (e->bits_per_sample * e->info.loop_end_pos) / 8;
	printf("  sample count:       %d ($%06X)\n", e->length, e->length);
	printf("  loop start pos:     %d ($%06X)\n", e->info.loop_start_pos, e->info.loop_start_pos);
	printf("  loop end pos:       %d ($%06X)\n", e->info.loop_end_pos, e->info.loop_end_pos);
	printf("  start address:      %d ($%06X)\n", e->start_address, e->start_address);
	printf("  end address:        %d ($%06X)\n", e->end_address, e->end_address);
	printf("  loop start address: %d ($%06X)\n", e->loop_start_address, e->loop_start_address);
	printf("  loop end address:   %d ($%06X)\n", e->loop_end_address, e->loop_end_address);

	// Copy data.
	switch (e->info.fmt)
	{
		default:
			fprintf(stderr, "[CONV] Format NG!\n");
			return false;
		case FMT_ADPCM:
			ymz_encode(srcpcm, e->data, e->length);
			break;
		case FMT_PCM8:
			// Shift down to 8-bit.
			for (uint32_t i = 0; i < e->channels * e->length; i++)
			{
				e->data[i] = srcpcm[i] >> 8;
			}
			break;
		case FMT_PCM16:
			// Copy as-is.
			memcpy(e->data, srcpcm, e->data_bytes);
			break;
	}
	free(srcpcm);

	// Calculate fn reg value based on clock.
	// FN controls how many 192 cycle steps to process before proceeding to the next sample.
	// ADPCM needs two steps to decode, so it has a more limited range.
	//
	// 4-bit ADPCM: 0.172265626KHz to 44.100KHz
	// 8-bit APDCM: 0.172265626KHz to 88.200KHz
	//
	const float base_freq = (e->info.fmt == FMT_ADPCM) ? 44100 : 88200;
	const float adjusted_freq = (base_freq * e->info.clock) / (float)YMZ280B_CLOCK_NOMINAL;
	printf("Base freq @ %fHz = %fHz\n", base_freq, adjusted_freq);
	const int steps = (e->info.fmt == FMT_ADPCM) ? 256 : 512;
	printf("  fmt %d : fn %d steps\n", e->info.fmt, steps);
	e->fn_reg = (uint16_t)(((steps-1) * e->info.sample_rate) / adjusted_freq);
	printf("  src freq %dHz = fn $%03X\n", e->info.sample_rate, e->fn_reg);

	// Advance data block position for next file.
	s->info.data_offs += e->data_bytes;

	return true;
}

static void conv_shutdown(Conv *s)
{
	Entry *e = s->entry_head;
	while (e)
	{
		if (e->data) free(e->data);
		Entry *next = e->next;
		free(e);
		e = next;
	}
}


//
// Configuration parsing
//

static int handler(void *user, const char *section, const char *name, const char *value)
{
	Conv *s = (Conv *)user;

	// New section - copy in name
	if (strcmp(section, s->info.symbol) != 0)
	{
		memset(s->info.symbol, 0, sizeof(s->info.symbol));
		strncpy(s->info.symbol, section, sizeof(s->info.symbol));
		s->info.symbol[sizeof(s->info.symbol)-1] = '\0';
		memset(s->info.symbol_upper, 0, sizeof(s->info.symbol_upper));
		for (int i = 0; i < strlen(s->info.symbol); i++)
		{
			s->info.symbol_upper[i] = toupper(s->info.symbol[i]);
		}
	}

	// Setting the source is what kicks it off
	if (strcmp("src", name) == 0)
	{
		strncpy(s->info.src, value, sizeof(s->info.src));
		s->info.src[sizeof(s->info.src)-1] = '\0';
		if (!conv_entry_add(s)) return 0;
	}
	else if (strcmp("out", name) == 0)
	{
		strncpy(s->out, value, sizeof(s->out));
		s->out[sizeof(s->out)-1] = '\0';
	}
	else if (strcmp("format", name) == 0)
	{
		if (strcmp("adpcm", value) == 0) s->info.fmt = FMT_ADPCM;
		else if (strcmp("pcm8", value) == 0) s->info.fmt = FMT_PCM8;
		else if (strcmp("pcm16", value) == 0) s->info.fmt = FMT_PCM16;
	}
	else if (strcmp("loop_start", name) == 0)
	{
		s->info.loop_start_pos = strtoul(value, NULL, 0);
	}
	else if (strcmp("loop_end", name) == 0)
	{
		s->info.loop_end_pos = strtoul(value, NULL, 0);
	}
	else if (strcmp("data_offs", name) == 0)
	{
		s->info.data_offs = strtoul(value, NULL, 0);
	}
	else if (strcmp("clock", name) == 0)
	{
		s->info.clock = strtoul(value, NULL, 0);
	}
	else if (strcmp("tl", name) == 0)
	{
		s->info.tl = strtoul(value, NULL, 0);
	}
	else if (strcmp("panpot", name) == 0)
	{
		s->info.panpot = strtoul(value, NULL, 0);
	}
	else if (strcmp("loop", name) == 0)
	{
		s->info.loop = strtoul(value, NULL, 0) ? true : false;
	}

	return 1;
}

static void conv_init(Conv *conv)
{
	memset(conv, 0, sizeof(*conv));
	conv->info.clock = YMZ280B_CLOCK_NOMINAL;
	conv->info.fmt = FMT_ADPCM;
	conv->info.tl = 0xFF;
	conv->info.panpot = 0x08;
	conv->info.loop = false;
}

int main(int argc, char **argv)
{
	int ret = -1;
	if (argc < 2)
	{
		printf("Usage: %s CONFIG\n", argv[0]);
		return -1;
	}

	Conv conv;
	conv_init(&conv);

	// the actual conversion is kicked off in the INI handler, when `convert` is set.
	ret = ini_parse(argv[1], &handler, &conv);
	// TODO: handle INI parser error

	// Now emit a pile of CHR data
	char fname_buf[512];

	// TODO: f_h for C header
	FILE *f_inc = NULL;
	FILE *f_bin = NULL;
	FILE *f_ymz = NULL;

	// YMZ binary data
	snprintf(fname_buf, sizeof(fname_buf), "%s.ymz", conv.out);
	f_ymz = fopen(fname_buf, "wb");
	if (!f_ymz)
	{
		fprintf(stderr, "Couldn't open %s for writing\n", fname_buf);
		ret = -1;
		goto done;
	}

	// DAT
	snprintf(fname_buf, sizeof(fname_buf), "%s.bin", conv.out);
	f_bin = fopen(fname_buf, "wb");
	if (!f_bin)
	{
		fprintf(stderr, "Couldn't open %s for writing\n", fname_buf);
		ret = -1;
		goto done;
	}

	// INC assembly header
	snprintf(fname_buf, sizeof(fname_buf), "%s.inc", conv.out);
	f_inc = fopen(fname_buf, "wb");
	if (!f_inc)
	{
		fprintf(stderr, "Couldn't open %s for writing\n", fname_buf);
		ret = -1;
		goto done;
	}

	fprintf(f_inc, "; ┌────────────────────────────────────────────────────────────────────────────┐\n");
	fprintf(f_inc, "; │                                                                            │\n");
	fprintf(f_inc, "; │                               YMZ280B DATA INDEX                           │\n");
	fprintf(f_inc, "; │                                                                            │\n");
	fprintf(f_inc, "; └────────────────────────────────────────────────────────────────────────────┘\n");
	fprintf(f_inc, "\n");

	Entry *e = conv.entry_head;
	while (e)
	{
		// Binary Data
		const uint8_t reg00_bits = e->fn_reg & 0xFF;  // key on, mode bits, loop not set, high fn bit
		const uint8_t reg01_bits = 0x80 | (e->info.loop ? 0x10 : 0x00) | (e->fn_reg>>8) | (e->info.fmt << 5);  // key on, mode bits, loop not set, high fn bit
		fputc(reg01_bits, f_bin);
		fputc(reg00_bits, f_bin);
		fputc(e->info.tl, f_bin);  // tl
		fputc(e->info.panpot, f_bin);  // panpot
		const uint32_t start_address = e->start_address;
		const uint32_t end_address = e->end_address;
		const uint32_t loop_start = e->loop_start_address;
		const uint32_t loop_end = e->loop_end_address;
		fputc((start_address>>16) & 0xFF, f_bin);
		fputc((start_address>>8) & 0xFF, f_bin);
		fputc(start_address & 0xFF, f_bin);
		if (e->info.loop)
		{
			fputc((loop_start>>16) & 0xFF, f_bin);
			fputc((loop_start>>8) & 0xFF, f_bin);
			fputc(loop_start & 0xFF, f_bin);
			fputc((loop_end>>16) & 0xFF, f_bin);
			fputc((loop_end>>8) & 0xFF, f_bin);
			fputc(loop_end & 0xFF, f_bin);
		}
		fputc((end_address>>16) & 0xFF, f_bin);
		fputc((end_address>>8) & 0xFF, f_bin);
		fputc(end_address & 0xFF, f_bin);

		// Write inc entry
		fprintf(f_inc, "; Entry $%03X \"%s\"\n", e->id, e->info.symbol);
		fprintf(f_inc, "%s_INDEX = $%04X\n", e->info.symbol_upper, e->id);
		fprintf(f_inc, "%s_BLOB_OFFS = $%04X\n", e->info.symbol_upper, e->id*YMZ_BLOB_ENTRY_SIZE);
		fprintf(f_inc, "%s_DATA_OFFS = $%05X\n", e->info.symbol_upper, e->info.data_offs);
		fprintf(f_inc, "%s_SAMPLING_RATE = %d\n", e->info.symbol_upper, e->info.sample_rate);
		fprintf(f_inc, "%s_FN_REG = $%02X\n", e->info.symbol_upper, e->fn_reg);
		fprintf(f_inc, "%s_SAMPLES = $%05X\n", e->info.symbol_upper, e->length);
		fprintf(f_inc, "%s_CHANNELS = $%05X\n", e->info.symbol_upper, e->channels-1);
		fprintf(f_inc, "%s_START_ADDRESS = $%05X\n", e->info.symbol_upper, start_address);
		fprintf(f_inc, "%s_END_ADDRESS = $%05X\n", e->info.symbol_upper, end_address);
		if (e->info.loop)
		{
			fprintf(f_inc, "%s_LOOP_START_ADDRESS = $%05X\n", e->info.symbol_upper, loop_start);
			fprintf(f_inc, "%s_LOOP_END_ADDRESS = $%05X\n", e->info.symbol_upper, loop_end);
		}
		fprintf(f_inc, "\n");

		// Pack YMZ data
		fwrite(e->data, sizeof(uint8_t), e->data_bytes, f_ymz);
		e = e->next;
	}

done:
	if (f_ymz) fclose(f_ymz);
	if (f_bin) fclose(f_bin);
	if (f_inc) fclose(f_inc);
	conv_shutdown(&conv);

	return ret;
}
