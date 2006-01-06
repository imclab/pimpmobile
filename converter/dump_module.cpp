#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <map>

#include "converter.h"
#include "../src/internal.h"

#include <vector>

/* little endian, datatypes aligned to their size */
class BinChunk
{
public:
	bool align(int a)
	{
		if ((data.size() % a) != 0)
		{
			int b = a - (data.size() % a);
			for (;b;--b) data.push_back(0);
			return false;
		}
		return true;
	}
	
	void dump(const unsigned char b)
	{
		data.push_back(b);
	}

	void dump(const unsigned short h)
	{
		align(2);
		data.push_back((unsigned char)(h >> 0));
		data.push_back((unsigned char)(h >> 8));
	}

	void dump(const unsigned int i)
	{
		align(4);
		data.push_back((unsigned char)(i >>  0));
		data.push_back((unsigned char)(i >>  8));
		data.push_back((unsigned char)(i >> 16));
		data.push_back((unsigned char)(i >> 24));
	}

private:
	std::vector<unsigned char> data;
};

unsigned buffer_size = 0;
unsigned char *data = 0;

unsigned pos = 0;

void check_size(int needed_size)
{
	if (buffer_size < (pos + needed_size))
	{
		buffer_size *= 2;
		data = (unsigned char*)realloc(data, buffer_size);
		memset(&data[buffer_size / 2], 0, buffer_size / 2);
	}
}

void align(int alignment)
{
	if ((pos % alignment) != 0)
	{
		int align_amt = alignment - (pos % alignment);
		check_size(align_amt);
		pos += align_amt;
	}
}

void dump_byte(unsigned char b)
{
	check_size(1);
	data[pos + 0] = b;
	pos++;
}

void dump_halfword(unsigned short h)
{
	align(2);
	check_size(2);
	pos += pos % 2;
	data[pos + 0] = (unsigned char)(h >> 0);
	data[pos + 1] = (unsigned char)(h >> 8);
	pos += 2;
}

void dump_word(unsigned int w)
{
	align(4);
	check_size(4);
	pos += pos % 4;
	data[pos + 0] = (unsigned char)(w >> 0);
	data[pos + 1] = (unsigned char)(w >> 8);
	data[pos + 2] = (unsigned char)(w >> 16);
	data[pos + 3] = (unsigned char)(w >> 24);
	pos += 4;
}

void dump_string(const char *str, const size_t len)
{
	size_t slen = strlen(str) + 1;
	size_t real_len;

	if (len > 0) real_len = (len > slen) ? slen : len;
	else real_len = slen;
	
	for (int i = 0; i < real_len; ++i)
	{
		data[pos + i] = str[i];
	}
	
	if (len > 0) pos += len;
	else pos += slen;
}

void dump_datastruct(const char *format, ...)
{
	va_list marker;
	va_start(marker, format);
	
	unsigned int i;

	while (*format != '\0')
	{
		if (buffer_size < (pos + 4))
		{
			buffer_size *= 2;
			data = (unsigned char*)realloc(data, buffer_size);
			memset(&data[buffer_size / 2], 0, buffer_size / 2);
		}
		
		switch (*format++)
		{
			case 'x':
				pos++;
			break;
			
			case 'b':
				i = va_arg(marker, int);
				dump_byte((unsigned char)i);
			break;
			
			case 'h':
				i = va_arg(marker, int);
				dump_halfword((unsigned short)i);
			break;
			
			case 'i':
				i = va_arg(marker, int);
				dump_word((unsigned int)i);
			break;
			
			default: assert(0);
		}
	}
	va_end(marker);
}

/*
struct foo
{
	int a __attribute__ ((aligned(8)));
	int b __attribute__ ((aligned(8)));
};
*/


/*
	// these are offsets relative to the begining of the pimp_module_t-structure
	unsigned order_ptr;
	unsigned pattern_data_ptr;
	unsigned channel_ptr;
	unsigned instrument_ptr;
*/

using std::map;
using std::multimap;
using std::make_pair;

void dump_module(module_t *mod, const char *filename)
{
	assert(mod != 0);

	printf("\ndumping module \"%s\"\n\n", mod->name);
	pos = 0;
	buffer_size = 1024;
	
	multimap<void *, unsigned> pointer_map;
	map<void *, unsigned> pointer_back_map;

	data = (unsigned char*)malloc(buffer_size);
	memset(data, 0, buffer_size);
	
	unsigned flags = 0;
	if (mod->use_linear_frequency_table)   flags |= FLAG_LINEAR_PERIODS;
	if (mod->instrument_vibrato_use_linear_frequency_table) flags |= FLAG_LINEAR_VIBRATO;
	if (mod->volume_slide_in_tick0)        flags |= FLAG_VOL_SLIDE_TICK0;
	if (mod->vibrato_in_tick0)             flags |= FLAG_VIBRATO_TICK0;
	if (mod->vol0_optimizations)           flags |= FLAG_VOL0_OPTIMIZE;
	if (mod->tremor_extra_delay)           flags |= FLAG_TEMOR_EXTRA_DELAY;
	if (mod->tremor_has_memory)            flags |= FLAG_TEMOR_MEMORY;
	if (mod->retrig_kills_note)            flags |= FLAG_RETRIG_KILLS_NOTE;
	if (mod->note_cut_kills_note)          flags |= FLAG_NOTE_CUT_KILLS_NOTE;
	if (mod->allow_nested_loops)           flags |= FLAG_ALLOW_NESTED_LOOPS;
	if (mod->retrig_note_source_is_period) flags |= FLAG_RETRIG_NOTE_PERIOD;
	if (mod->delay_global_volume)          flags |= FLAG_DELAY_GLOBAL_VOLUME;
	if (mod->sample_offset_clamp)          flags |= FLAG_SAMPLE_OFFSET_CLAMP;
	if (mod->tone_porta_share_memory)      flags |= FLAG_PORTA_NOTE_SHARE_MEMORY;
	if (mod->remember_tone_porta_target)   flags |= FLAG_PORTA_NOTE_MEMORY;
	
	dump_string(mod->name, 32);
	dump_word(flags);
	dump_word(0); // reserved for future flags
	
	// order_ptr
	pointer_map.insert(make_pair(&mod->order[0], pos));
	dump_word((unsigned)&mod->order[0]);

	// pattern_ptr
	pointer_map.insert(make_pair(&mod->patterns[0], pos));
	dump_word((unsigned)&mod->patterns[0]);
	
	// channel_ptr
	pointer_map.insert(make_pair(&mod->channels[0], pos));
	dump_word((unsigned)&mod->channels[0]);
	
	// instrument_ptr
	pointer_map.insert(make_pair(&mod->instruments[0], pos));
	dump_word((unsigned)&mod->instruments[0]);
	
	dump_halfword(mod->period_low_clamp);
	dump_halfword(mod->period_high_clamp);
	dump_halfword(mod->order.size());
	
	dump_byte(mod->repeat_pos);
	dump_byte(mod->initial_global_volume);
	dump_byte(mod->initial_tempo);
	dump_byte(mod->initial_bpm);
	
	dump_byte(mod->instruments.size());
	dump_byte(mod->patterns.size());
	dump_byte(mod->channels.size());
	
	// array of bytes, no need for alignment
	pointer_back_map.insert(make_pair(&mod->order[0], pos));
	for (int i = 0; i < mod->order.size(); ++i)
	{
		dump_byte(mod->order[i]);
	}

	// patterns
	align(4);
	pointer_back_map.insert(make_pair(&mod->patterns[0], pos));
	for (int i = 0; i < mod->patterns.size(); ++i)
	{
		align(4);
		pointer_map.insert(make_pair(&mod->patterns[i].pattern_data[0], pos));
		dump_word((unsigned)&mod->patterns[i].pattern_data[0]); // data_ptr
		dump_halfword(mod->patterns[i].num_rows);
	}
	
	// pattern data
	for (int i = 0; i < mod->patterns.size(); ++i)
	{
		pattern_header_t &pat = mod->patterns[i];
		// no need for alignment as the struct only contains bytes
		pointer_back_map.insert(make_pair(&pat.pattern_data[0], pos));
		
		// write the actual data
		for (unsigned i = 0; i < pat.num_rows; ++i)
		{
			for (unsigned j = 0; j < mod->channels.size(); ++j)
			{
				pattern_entry_t &pe = pat.pattern_data[i * mod->channels.size() + j];
				dump_byte(pe.note);
				dump_byte(pe.instrument);
				dump_byte(pe.volume_command);
				dump_byte(pe.effect_byte);
				dump_byte(pe.effect_parameter);
			}
		}
	}

	// channel settings
	align(4);
	pointer_back_map.insert(make_pair(&mod->channels[0], pos));
	for (int i = 0; i < mod->channels.size(); ++i)
	{
		dump_byte(mod->channels[i].default_pan);
		dump_byte(mod->channels[i].initial_volume);
		dump_byte(mod->channels[i].mute_state);
	}

#if 0
	envelope_t *volume_envelope;
	envelope_t *panning_envelope;
	envelope_t *pitch_envelope;
	u16         fadeout_rate;
	new_note_action_t        new_note_action;
	duplicate_check_type_t   duplicate_check_type;
	duplicate_check_action_t duplicate_check_action;
	s8 pitch_pan_separation; /* no idea what this one does */
	u8 pitch_pan_center;     /* not this on either; this one seems to be a note index */
	u8 sample_count;                 /* number of samples tied to instrument */
	sample_header_t *sample_headers; /* pointer to an array of sample headers for the instrument */
	u8 sample_map[120];
#endif

	// instruments
	align(4);
	pointer_back_map.insert(make_pair(&mod->instruments[0], pos));
	for (int i = 0; i < mod->instruments.size(); ++i)
	{
		instrument_t &instr = mod->instruments[i];
//		printf("instrument: %s\n", instr.name);
		
		align(4);
		if (&instr.samples[0] != 0) pointer_map.insert(make_pair(&instr.samples[0], pos));
		dump_word((unsigned)&instr.samples[0]);
		
//		if (instr.volume_envelope != 0) pointer_map.insert(make_pair(instr.volume_envelope, pos));
		dump_word((unsigned)instr.volume_envelope);
		
//		if (instr.panning_envelope != 0) pointer_map.insert(make_pair(instr.panning_envelope, pos));
		dump_word((unsigned)instr.panning_envelope);
#if 0
		// IT ONLY 
		if (instr.pitch_envelope != 0) pointer_map.insert(make_pair(instr.pitch_envelope, pos));
		dump_word((unsigned)instr.pitch_envelope);
#endif
		dump_halfword(instr.fadeout_rate);
		dump_halfword(instr.samples.size());
		
		for (int s = 0; s < 120; ++s)
		{
			dump_byte(instr.sample_map[s]);
		}
/*
		for (int s = 0; s < instr.samples.size(); ++s)
		{
			sample_header_t &samp = instr.samples[s];
			assert(samp.rel_ptr >= 0);
			
		}
*/
	}

	// instrument data
	for (int i = 0; i < mod->instruments.size(); ++i)
	{
		instrument_t &instr = mod->instruments[i];
		
		align(4);
		pointer_back_map.insert(make_pair(&instr.samples[0], pos));
		
		for (int s = 0; s < instr.samples.size(); ++s)
		{
			sample_header_t &samp = instr.samples[s];
			assert(samp.rel_ptr >= 0);
			
			align(4);
			dump_word(samp.rel_ptr);
			dump_word(samp.length);
//			dump_word(10010);
			dump_word(samp.loop_start);
			dump_word(samp.loop_end - samp.loop_start);
			
			dump_halfword(samp.finetune);
			dump_halfword(samp.sample_note_offset);
			
			dump_byte(samp.default_volume);
			dump_byte(samp.loop_type);
			dump_byte(samp.default_pan_position);
			
			dump_byte(samp.vibrato_speed);
			dump_byte(samp.vibrato_depth);
			dump_byte(samp.vibrato_sweep);
			dump_byte(samp.vibrato_waveform);
		}
//		if (&instr.samples[0] != 0) pointer_map.insert(make_pair(&instr.samples[0], pos));
//		dump_word((unsigned)&instr.samples[0]);
	}
	
	
	// fixback pointers
	for (multimap<void *, unsigned>::iterator it = pointer_map.begin(); it != pointer_map.end(); ++it)
	{
		if (pointer_back_map.count(it->first) == 0)
		{
			printf("big, hairy error! (pointers are fucked up in the dumping-code)\n");
			exit(1);
		}
		
		unsigned *target = (unsigned*)(&data[it->second]);
		
		// verify that the data pointed to is really the original pointer (just another sanity-check)
		if (*target != (unsigned)it->first)
		{
			printf("POOOTATOOOOO!\n");
			exit(1);
		}
		
		*target = pointer_back_map[it->first];
	}
	
	FILE *fp = fopen(filename, "wb");
	if (!fp)
	{
		printf("error: failed to open output-file\n");
		exit(1);
	}
	
	for (unsigned i = 0; i < pos; ++i)
	{
		fwrite(&data[i], 1, 1, fp);
	}
	fclose(fp);

/*	
    NOT HERE! THIS IS A GLOBAL EXPORT!
	
	FILE *fp = fopen("samples.bin", "wb");
	for (unsigned i = 0; i < mod->instrument_count; ++i)
	{
		
	}
	fclose(fp);
*/
	
	free(data);
}