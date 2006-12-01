#include <src/pimp_mixer.h>
#include "../framework/test.h"

/*

typedef struct
{
	u32                   sample_length;
	u32                   loop_start;
	u32                   loop_end;
	pimp_mixer_loop_type  loop_type;
	const u8             *sample_data;
	u32                   sample_cursor;
	s32                   sample_cursor_delta;
	u32                   event_cursor;
	s32                   volume;
} pimp_mixer_channel_state;

typedef struct
{
	pimp_mixer_channel_state channels[CHANNELS];
	s32 *mix_buffer;
} pimp_mixer;

void __pimp_mixer_mix(pimp_mixer *mixer, s8 *target, int samples);
*/

void test_mem(const void *array, const void *reference, int size)
{
	char temp[256];
	int err = 0;
	
	int i;
	for (i = 0; i < size; ++i)
	{
		char val = ((char*)array)[i];
		char ref = ((char*)reference)[i];
		if (val != ref)
		{
			snprintf(temp, 256, "byte #%d not equal, got %X - expected %X", i, val, ref);
			err = 1;
			break;
		}
	}
	
	if (0 != err) test_fail(temp);
	else test_pass();
}

void test_int_array(const int *array, const int *reference, int size)
{
	char temp[256];
	int err = 0;
	
	int i;
	for (i = 0; i < size; ++i)
	{
		char val = array[i];
		char ref = reference[i];
		if (val != ref)
		{
			snprintf(temp, 256, "element #%d not equal, got %X - expected %X", i + 1, val, ref);
			err = 1;
			break;
		}
	}
	
	if (0 != err) test_fail(temp);
	else test_pass();
}


#define TEST_INTS_EQUAL(value, expected) TEST((value) == (expected), test_printf("ints not equal, got %d - expected %d", (int)(value), (int)(expected)))
#define TEST_MEM_EQUAL(array, reference, size) test_mem(array, reference, size)
#define TEST_INT_ARRAYS_EQUAL(array, reference, size) test_int_array(array, reference, size)

#define MAX_TARGET_SIZE 1024
s8  target[MAX_TARGET_SIZE + 2];
s32 mix_buffer[MAX_TARGET_SIZE + 2];

static void test_mixer_basic(void)
{
	/*
		cases that need testing:
			- that mixer never writes outside mix- and target buffers
			- that odd mix-buffer sizes does not write outside buffer
	*/
	int target_size;
	
	pimp_mixer mixer;
	__pimp_mixer_reset(&mixer);
	mixer.mix_buffer = mix_buffer + 1;
	
	/* try all buffer sizes */	
	for (target_size = 0; target_size < MAX_TARGET_SIZE; ++target_size)
	{
		s8 rnd = rand();
		
		target[0] = rnd;
		target[target_size + 1] = rnd;
		mix_buffer[0] = rnd;
		mix_buffer[target_size + 1] = rnd;
		
		__pimp_mixer_mix(&mixer, target + 1, target_size);
		
		TEST_INTS_EQUAL(target[0], rnd);
		TEST_INTS_EQUAL(target[target_size + 1], rnd);
		TEST_INTS_EQUAL(mix_buffer[0], rnd);
		TEST_INTS_EQUAL(mix_buffer[target_size + 1], rnd);
	}
}

#define ARRAY_SIZE(x) (sizeof((x)) / sizeof((x)[0]))

static void test_looping(void)
{
	/*
	cases that need testing:
		- forward looping
		- ping pong looping
		- that a loop never reads outside of the start and stop points
		- that a loop happens at the correct sub-sample
	*/

	pimp_mixer_channel_state chan;
	
	const int target_size = 8;
	const u8 sample_data[] = { 0x00, 0x01, 0x02, 0x03, 0x04 };
	
	chan.sample_length       = ARRAY_SIZE(sample_data);
	chan.loop_start          = 0;
	chan.loop_end            = 4;
	chan.loop_type           = LOOP_TYPE_FORWARD;
	chan.sample_data         = sample_data;
	chan.sample_cursor       = 0 << 12;
	chan.sample_cursor_delta = 1 << 12;
	chan.event_cursor        = 8 << 12;
	chan.volume              = 1;
	
	memset(mix_buffer, 0, target_size * sizeof(u32));
	__pimp_mixer_mix_channel(&chan, mix_buffer, target_size);
	
	const s32 forward_loop_ref[] = { 0x00, 0x01, 0x02, 0x03, 0x00, 0x01, 0x02, 0x03 };
	TEST_MEM_EQUAL(mix_buffer, forward_loop_ref, sizeof(forward_loop_ref));
		
	chan.loop_type           = LOOP_TYPE_PINGPONG;
	memset(mix_buffer, 0, target_size * sizeof(u32));
	__pimp_mixer_mix_channel(&chan, mix_buffer, target_size);
	
	const s32 forward_loop_ref2[] = { 0x00, 0x01, 0x02, 0x03, 0x03, 0x02, 0x01, 0x00 };
	TEST_INT_ARRAYS_EQUAL(mix_buffer, forward_loop_ref2, 8);
	
//	TEST_MEM_EQUAL(mix_buffer, forward_loop_ref, sizeof(forward_loop_ref2));

}

void test_mixer(void)
{
	test_mixer_basic();
	test_looping();
}
