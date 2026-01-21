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

cycle_t start_count;
cycle_t final_count;

int main(int argc, char *argv[])
{
	bit_writer_t bw;
	bw_init(&bw, data4, sizeof(data4));
    int num_blocks_x = (test_image_width + 7) / 8;
    int num_blocks_y = (test_image_height + 7) / 8;
    int total_blocks = (num_blocks_x) * (num_blocks_y);
    segment_into_blocks(data1, test_image_width, test_image_height, &num_blocks_x, &num_blocks_y, data2);
    int prev_dc = 0;
    int dc_diff=0;
    ac_pair_t ac_coef[256];
    for (int block = 0; block < total_blocks; block++) {
    	int index = block * 64;
    	center_pixels(&data2[index], current_block);
        dct_and_quantize(current_block, &qt_luminance);
        zig_zag(current_block);
        memset(ac_coef, 0, sizeof(ac_coef));
        int num_of_ac_pairs = encode_block(current_block, &prev_dc, &dc_diff, ac_coef);
        if (num_of_ac_pairs > sizeof(ac_coef)/sizeof(ac_coef[0])) {
            printf("Error: too many AC pairs!\n");
            exit(1);
        }
        int num_bits_current_block = huffman_encode_block(dc_diff, ac_coef, num_of_ac_pairs, &bw);
    }

    bw_flush(&bw);
    FILE* out = fopen("compressed_image.jpeg", "wb");
    zig_zag(qt_luminance.values);
    serialize_into_jpg(out, bw.buf, bw.size, qt_luminance.values, num_blocks_y*8, num_blocks_x*8,
    		huff_dc_luminance.code_lengths, huff_ac_luminance.code_lengths, huff_ac_luminance.symbols, huff_dc_luminance.symbols);

    fclose(out);
}

