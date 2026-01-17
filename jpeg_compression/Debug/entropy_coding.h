/*
 * entropy_coding.h
 *
 *  Created on: 17.01.2025.
 *      Author: Korisnik
 */

#ifndef DEBUG_ENTROPY_CODING_H_
#define DEBUG_ENTROPY_CODING_H_

#include "bit_write.h"
#include "dct.h"

typedef struct {
	int table_class;
	int table_id;
	int code_lengths[16];
	int symbols[256];
	int num_symbols;
} jpeg_huffman_table_t;

typedef struct {
    int run;           // number of zeros
    int size;          // number of bits
    int amplitude;     // non-zero AC value
} ac_pair_t;

int encode_block(signed short* block_ptr, int* prev_dc, int* dc_diff, ac_pair_t* ac_pairs) {
    *dc_diff = block_ptr[0] - *prev_dc;
    *prev_dc = block_ptr[0];
    int run = 0;
    int ac_idx = 0;

    for (int i = 1; i < 64; i++) {
        if (block_ptr[i] == 0) {
            run++;
        } else {
            while (run > 15) { // ZRL
                ac_pairs[ac_idx].run = 15;
                ac_pairs[ac_idx].size = 0;   // 0 size for ZRL
                ac_pairs[ac_idx].amplitude = 0;
                ac_idx++;
                run -= 16;
            }
            ac_pairs[ac_idx].run = run;
            ac_pairs[ac_idx].size = category(block_ptr[i]);
            ac_pairs[ac_idx].amplitude = block_ptr[i];
            ac_idx++;
            run = 0;
        }
    }
    if (run > 0) {
        ac_pairs[ac_idx].run = 0;
        ac_pairs[ac_idx].size = 0;   // EOB marker
        ac_pairs[ac_idx].amplitude = 0;
        ac_idx++;
    }

    return ac_idx;
}


int huffman_encode_block(
    int dc_diff,
    ac_pair_t* ac_pairs,
    int ac_count,
    bit_writer_t *bw
)
{
    // DC ENCODING
    int dc_size = category(dc_diff);
    HuffmanCode dc_huff = huff_dc_lum[dc_size];

    bw_write_bits(bw, dc_huff.code, dc_huff.len);

    int amp=0;

    if (dc_size > 0) {
        amp = (dc_diff >= 0)
            ? dc_diff
            : (1 << dc_size) - 1 + dc_diff;

        bw_write_bits(bw, amp, dc_size);
    }

    // AC ENCODING
    for (int k = 0; k < ac_count; k++) {
        int symbol = (ac_pairs[k].run << 4) | ac_pairs[k].size;
        HuffmanCode ac_huff = huff_ac_lum[symbol];

        bw_write_bits(bw, ac_huff.code, ac_huff.len);

        if (ac_pairs[k].size > 0) {
            int amp = (ac_pairs[k].amplitude >= 0)
                ? ac_pairs[k].amplitude
                : (1 << ac_pairs[k].size) - 1 + ac_pairs[k].amplitude;

            bw_write_bits(bw, amp, ac_pairs[k].size);
        }

        if (ac_pairs[k].run == 0 && ac_pairs[k].size == 0) {
        	break;  // EOB — stop encoding this block
        }
    }

    return 0;
}

jpeg_huffman_table_t huff_dc_luminance = {
	    .table_class = 0,
	    .table_id = 0,
	    .code_lengths = {0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0},
	    .symbols = {0,1,2,3,4,5,6,7,8,9,10,11},
	    .num_symbols = 12
	};

jpeg_huffman_table_t huff_ac_luminance = {
	.table_class = 1,
	.table_id = 0,
	.code_lengths = {0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,0x7d},
	.symbols = {
		0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,
		0x21,0x31,0x41,0x06,0x13,0x51,0x61,0x07,
	    0x22,0x71,0x14,0x32,0x81,0x91,0xA1,0x08,
	    0x23,0x42,0xB1,0xC1,0x15,0x52,0xD1,0xF0,
	    0x24,0x33,0x62,0x72,0x82,0x09,0x0A,0x16,
	    0x17,0x18,0x19,0x1A,0x25,0x26,0x27,0x28,
	    0x29,0x2A,0x34,0x35,0x36,0x37,0x38,0x39,
	    0x3A,0x43,0x44,0x45,0x46,0x47,0x48,0x49,
	    0x4A,0x53,0x54,0x55,0x56,0x57,0x58,0x59,
	    0x5A,0x63,0x64,0x65,0x66,0x67,0x68,0x69,
	    0x6A,0x73,0x74,0x75,0x76,0x77,0x78,0x79,
	    0x7A,0x83,0x84,0x85,0x86,0x87,0x88,0x89,
	    0x8A,0x92,0x93,0x94,0x95,0x96,0x97,0x98,
	    0x99,0x9A,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,
	    0xA8,0xA9,0xAA,0xB2,0xB3,0xB4,0xB5,0xB6,
	    0xB7,0xB8,0xB9,0xBA,0xC2,0xC3,0xC4,0xC5,
	    0xC6,0xC7,0xC8,0xC9,0xCA,0xD2,0xD3,0xD4,
	    0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xE1,0xE2,
	    0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,
	    0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,
	    0xF9,0xFA
	},
	.num_symbols = 162
};



#endif /* DEBUG_ENTROPY_CODING_H_ */
