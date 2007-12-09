///////////////////////////////////////////////////////////////////////////////
//
/// \file       test_filter_flags.c
/// \brief      Tests Filter Flags coders
//
//  Copyright (C) 2007 Lasse Collin
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
///////////////////////////////////////////////////////////////////////////////

#include "tests.h"


static uint8_t buffer[4096];
static lzma_options_filter known_flags;
static lzma_options_filter decoded_flags;
static lzma_stream strm = LZMA_STREAM_INIT;


static bool
encode(uint32_t known_size)
{
	memcrap(buffer, sizeof(buffer));

	uint32_t tmp;
	if (lzma_filter_flags_size(&tmp, &known_flags) != LZMA_OK)
		return true;

	if (tmp != known_size)
		return true;

	size_t out_pos = 0;
	if (lzma_filter_flags_encode(buffer, &out_pos, known_size,
			&known_flags) != LZMA_OK)
		return true;

	if (out_pos != known_size)
		return true;

	return false;
}


static bool
decode_ret(uint32_t known_size, lzma_ret ret_ok)
{
	memcrap(&decoded_flags, sizeof(decoded_flags));

	if (lzma_filter_flags_decoder(&strm, &decoded_flags) != LZMA_OK)
		return true;

	if (decoder_loop_ret(&strm, buffer, known_size, ret_ok))
		return true;

	return false;
}


static bool
decode(uint32_t known_size)
{
	if (decode_ret(known_size, LZMA_STREAM_END))
		return true;

	if (known_flags.id != decoded_flags.id)
		return true;

	return false;
}


static void
test_copy(void)
{
	// Test 1 (good)
	known_flags.id = LZMA_FILTER_COPY;
	known_flags.options = NULL;

	expect(!encode(1));
	expect(!decode(1));
	expect(decoded_flags.options == NULL);

	// Test 2 (invalid encoder options)
	known_flags.options = &known_flags;
	expect(encode(99));

	// Test 3 (good but unusual Filter Flags field)
	buffer[0] = 0xE0;
	buffer[1] = LZMA_FILTER_COPY;
	expect(!decode(2));
	expect(decoded_flags.options == NULL);

	// Test 4 (invalid Filter Flags field)
	buffer[0] = 0xE1;
	buffer[1] = LZMA_FILTER_COPY;
	buffer[2] = 0;
	expect(!decode_ret(3, LZMA_HEADER_ERROR));

	// Test 5 (good but weird Filter Flags field)
	buffer[0] = 0xFF;
	buffer[1] = LZMA_FILTER_COPY;
	buffer[2] = 0;
	expect(!decode(3));
	expect(decoded_flags.options == NULL);

	// Test 6 (invalid Filter Flags field)
	buffer[0] = 0xFF;
	buffer[1] = LZMA_FILTER_COPY;
	buffer[2] = 1;
	buffer[3] = 0;
	expect(!decode_ret(4, LZMA_HEADER_ERROR));
}


static void
test_subblock(void)
{
	// Test 1
	known_flags.id = LZMA_FILTER_SUBBLOCK;
	known_flags.options = NULL;

	expect(!encode(1));
	expect(!decode(1));
	expect(decoded_flags.options != NULL);
	expect(((lzma_options_subblock *)(decoded_flags.options))
			->allow_subfilters);

	// Test 2
	known_flags.options = decoded_flags.options;
	expect(!encode(1));
	expect(!decode(1));
	expect(decoded_flags.options != NULL);
	expect(((lzma_options_subblock *)(decoded_flags.options))
			->allow_subfilters);

	free(decoded_flags.options);
	free(known_flags.options);

	// Test 3
	buffer[0] = 0xFF;
	buffer[1] = LZMA_FILTER_SUBBLOCK;
	buffer[2] = 1;
	buffer[3] = 0;
	expect(!decode_ret(4, LZMA_HEADER_ERROR));
}


static void
test_simple(void)
{
	// Test 1
	known_flags.id = LZMA_FILTER_X86;
	known_flags.options = NULL;

	expect(!encode(1));
	expect(!decode(1));
	expect(decoded_flags.options == NULL);

	// Test 2
	lzma_options_simple options;
	options.start_offset = 0;
	known_flags.options = &options;
	expect(!encode(1));
	expect(!decode(1));
	expect(decoded_flags.options == NULL);

	// Test 3
	options.start_offset = 123456;
	known_flags.options = &options;
	expect(!encode(6));
	expect(!decode(6));
	expect(decoded_flags.options != NULL);

	lzma_options_simple *decoded = decoded_flags.options;
	expect(decoded->start_offset == options.start_offset);

	free(decoded);
}


static void
test_delta(void)
{
	// Test 1
	known_flags.id = LZMA_FILTER_DELTA;
	known_flags.options = NULL;
	expect(encode(99));

	// Test 2
	lzma_options_delta options = { 0 };
	known_flags.options = &options;
	expect(encode(99));

	// Test 3
	options.distance = LZMA_DELTA_DISTANCE_MIN;
	expect(!encode(2));
	expect(!decode(2));
	expect(((lzma_options_delta *)(decoded_flags.options))
				->distance == options.distance);

	free(decoded_flags.options);

	// Test 4
	options.distance = LZMA_DELTA_DISTANCE_MAX;
	expect(!encode(2));
	expect(!decode(2));
	expect(((lzma_options_delta *)(decoded_flags.options))
				->distance == options.distance);

	free(decoded_flags.options);

	// Test 5
	options.distance = LZMA_DELTA_DISTANCE_MAX + 1;
	expect(encode(99));
}


static void
validate_lzma(void)
{
	const lzma_options_lzma *known = known_flags.options;
	const lzma_options_lzma *decoded = decoded_flags.options;

	expect(known->dictionary_size <= decoded->dictionary_size);

	if (known->dictionary_size == 1)
		expect(decoded->dictionary_size == 1);
	else
		expect(known->dictionary_size + known->dictionary_size / 2
				> decoded->dictionary_size);

	expect(known->literal_context_bits == decoded->literal_context_bits);
	expect(known->literal_pos_bits == decoded->literal_pos_bits);
	expect(known->pos_bits == decoded->pos_bits);
}


static void
test_lzma(void)
{
	// Test 1
	known_flags.id = LZMA_FILTER_LZMA;
	known_flags.options = NULL;
	expect(encode(99));

	// Test 2
	lzma_options_lzma options = {
		.dictionary_size = 0,
		.literal_context_bits = 0,
		.literal_pos_bits = 0,
		.pos_bits = 0,
		.preset_dictionary = NULL,
		.preset_dictionary_size = 0,
		.mode = LZMA_MODE_INVALID,
		.fast_bytes = 0,
		.match_finder = LZMA_MF_INVALID,
		.match_finder_cycles = 0,
	};

	// Test 3 (empty dictionary not allowed)
	known_flags.options = &options;
	expect(encode(99));

	// Test 4 (brute-force test some valid dictionary sizes)
	while (options.dictionary_size != LZMA_DICTIONARY_SIZE_MAX) {
		if (++options.dictionary_size == 5000)
			options.dictionary_size = LZMA_DICTIONARY_SIZE_MAX - 5;

		expect(!encode(3));
		expect(!decode(3));
		validate_lzma();

		free(decoded_flags.options);
	}

	// Test 5 (too big dictionary size)
	options.dictionary_size = LZMA_DICTIONARY_SIZE_MAX + 1;
	expect(encode(99));

	// Test 6 (brute-force test lc/lp/pb)
	options.dictionary_size = 1;
	for (uint32_t lc = LZMA_LITERAL_CONTEXT_BITS_MIN;
			lc <= LZMA_LITERAL_CONTEXT_BITS_MAX; ++lc) {
		for (uint32_t lp = LZMA_LITERAL_POS_BITS_MIN;
				lp <= LZMA_LITERAL_POS_BITS_MAX; ++lp) {
			for (uint32_t pb = LZMA_POS_BITS_MIN;
					pb <= LZMA_POS_BITS_MAX; ++pb) {
				options.literal_context_bits = lc;
				options.literal_pos_bits = lp;
				options.pos_bits = pb;

				expect(!encode(3));
				expect(!decode(3));
				validate_lzma();

				free(decoded_flags.options);
			}
		}
	}
}


int
main(void)
{
	lzma_init();

	test_copy();
	test_subblock();
	test_simple();
	test_delta();
	test_lzma();

	lzma_end(&strm);

	return 0;
}
