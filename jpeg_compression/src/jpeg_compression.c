/*****************************************************************************
 * jpeg_compression.c
 *****************************************************************************/

#include <sys/platform.h>
#include "adi_initialize.h"
#include "jpeg_compression.h"
#include "small_image.h"
#include <stdfix.h>
#include <builtins.h>
#include <def21489.h>
#include <sru21489.h>
#include <SYSREG.h>
#include <cycle_count.h>
#include "main_operations.h"

#pragma section("seg_block2")
unsigned char data2[200000];

#pragma section("seg_block1")
unsigned char data4[16000];

#pragma section("seg_pm_mem")
#pragma align 4
signed short pm current_block[64];

signed short uncentered_block[128];

#pragma section("seg_pm_mem")
ac_pair_t pm ac_coef[256];

#pragma section("seg_pm_mem")
bit_writer_t pm bw;

#pragma section("seg_pm_mem")
float pm temp[64];

void initialize_DMA()
{
	*pAMICTL1 |= (AMIEN | BW16 | WS1);
	*pMTMCTL   = MTMFLUSH;
	asm("nop;nop;nop;nop;nop;");
	*pMTMCTL  &= ~MTMFLUSH;
	*pIIEP0    = (unsigned int)uncentered_block;
	*pIMEP0     = 1;
	*pICEP0     = 64;
	*pEIEP0    = (unsigned int)data2;
	*pEMEP0    = 1;
	*pDMAC0   |= DEN;
}


cycle_t start_count;
cycle_t final_count;
cycle_t start_bottle_neck_count;
cycle_t final_bottle_neck_count;


int main(int argc, char *argv[])
{
	int current_buf = 0;
	bw_init(&bw, data4, sizeof(data4));
    int num_blocks_x = (test_image_width + 7) / 8;
    int num_blocks_y = (test_image_height + 7) / 8;
    int total_blocks = (num_blocks_x) * (num_blocks_y);
    START_CYCLE_COUNT(start_count);
    segment_into_blocks(data1, test_image_width, test_image_height, &num_blocks_x, &num_blocks_y, data2);
	initialize_DMA();
    int prev_dc = 0;
    int dc_diff=0;

    for (int block = 0; block < total_blocks; block++) {
    	int index = block * 64;

        while(*pECEP0 > 0);

        if (expected_true(block < total_blocks - 1))
        {
        	int next_buf = 1 - current_buf;
        	*pDMAC0 &= ~DEN;
        	*pEIEP0 = (unsigned int)(data2 + sizeof(char) * index);
        	*pIIEP0 = (unsigned int)&uncentered_block[next_buf*64];
        	*pICEP0 = 64;
        	*pDMAC0 |= DEN;
        }
    	center_pixels(&uncentered_block[current_buf*64], current_block);

    	START_CYCLE_COUNT(start_bottle_neck_count);
        dct_and_quantize(current_block, &qt_luminance,temp);
        STOP_CYCLE_COUNT(final_bottle_neck_count,start_bottle_neck_count);
        if (block == 0) PRINT_CYCLES("Number of total cycles for DCT and quantization: ",final_bottle_neck_count*total_blocks);
        zig_zag(current_block);
        memset(ac_coef, 0, sizeof(ac_coef));
        int num_of_ac_pairs = encode_block(current_block, &prev_dc, &dc_diff, ac_coef);
        if (num_of_ac_pairs > sizeof(ac_coef)/sizeof(ac_coef[0])) {
            printf("Error: too many AC pairs!\n");
            exit(1);
        }
        int num_bits_current_block = huffman_encode_block(dc_diff, ac_coef, num_of_ac_pairs, &bw);
        current_buf = 1 - current_buf;
    }

    bw_flush(&bw);
    STOP_CYCLE_COUNT(final_count,start_count);
    PRINT_CYCLES("Number of total cycles for 131x113 image: ",final_count);
    FILE* out = fopen("compressed_image.jpeg", "wb");
    zig_zag(qt_luminance.values);
    serialize_into_jpg(out, bw.buf, bw.size, qt_luminance.values, num_blocks_y*8, num_blocks_x*8,
    		huff_dc_luminance.code_lengths, huff_ac_luminance.code_lengths, huff_ac_luminance.symbols, huff_dc_luminance.symbols);

    fclose(out);
}


