/*
 * main_operations.h
 *
 *  Created on: 17.01.2025.
 *      Author: Korisnik
 */

#ifndef DEBUG_MAIN_OPERATIONS_H_
#define DEBUG_MAIN_OPERATIONS_H_

#include "entropy_coding.h"
#define PI 3.14159265358979323846


typedef struct {
    int precision;
    int table_id;
    signed short values[64];
} jpeg_quant_table_t;


int zigzag_index[64] = {
     0,  1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};

jpeg_quant_table_t qt_luminance = {
	.precision = 0,
	.table_id  = 0,
	.values = {
		16, 11, 10, 16, 24, 40, 51, 61,
		12, 12, 14, 19, 26, 58, 60, 55,
	    14, 13, 16, 24, 40, 57, 69, 56,
	    14, 17, 22, 29, 51, 87, 80, 62,
	    18, 22, 37, 56, 68,109,103, 77,
	    24, 35, 55, 64, 81,104,113, 92,
	    49, 64, 78, 87,103,121,120,101,
	    72, 92, 95, 98,112,100,103, 99
	}
};

inline void center_pixels(signed short* restrict img_pixels,signed short pm* output_data) {
#pragma all_external_access_reg_optimized
#pragma vector_for
	for(int i = 0;i<64;i++)
	{
		output_data[i] = (signed short pm)(img_pixels[i])-128;
	}
}


void segment_into_blocks(const unsigned char* restrict not_centered_pixels, int width, int height, int* restrict num_blocks_x, int* restrict num_blocks_y, unsigned char* result) {
    // fill blocks
    for (int by = 0; by < *num_blocks_y; by++) {
        for (int bx = 0; bx < *num_blocks_x; bx++) {
            for (int y = 0; y < 8; y++) {
                for (int x = 0; x < 8; x++) {
                    int img_x = bx*8 + x;
                    int img_y = by*8 + y;
                    int block_idx = (by * (*num_blocks_x) + bx) * 64;
                    int pixel_idx_in_block = y * 8 + x;

                    int src_x = width - 1;
                    if(expected_true(img_x < width))
                    	src_x = img_x;
                    int src_y = height - 1;
                    if(expected_true(img_y < height))
                    	src_y = img_y;


                    int result_index = block_idx + pixel_idx_in_block;
                    int not_centered_pixels_index = src_y * width + src_x;
                    result[result_index] =
                    		not_centered_pixels[not_centered_pixels_index];
                }
            }
        }
    }
}
// the following functions operate on a single block, it is required to traverse through all blocks
inline void dct_and_quantize(signed short pm* block_ptr, const jpeg_quant_table_t* qt, float pm* restrict temp)
{
    // Forward 2D DCT
    for (int u = 8; u != 0; u--) {
        for (int v = 8; v != 0; v--) {
            float sum = 0.0;
            for (int y = 8; y != 0; y--) {
                for (int x = 8; x != 0; x--) {
                	int real_x = x-1;
                	int real_u = u-1;
                	int real_y = y-1;
                	int real_v = v-1;
                	int block_ptr_index = real_y*8+real_x;

                	float cosX = COS_LUT[real_y][real_u];
                	float cosY = COS_LUT[real_x][real_v];

                	sum += block_ptr[block_ptr_index] * cosX * cosY;

                }
            }

            float cu = C_LUT[u-1];
            float cv = C_LUT[v-1];

            int freq_idx = (u-1 << 3) + v-1;
            temp[freq_idx] = 0.25f * cv * cu * sum;
        }
    }

    for(int i = 64;i!=0;i--)
    {
    	int q_index = i-1;
    	short q = qt->values[q_index];
    	float val = temp[q_index] / q;
    	block_ptr[q_index] = (short)(val > 0 ? val + 0.5 : val - 0.5);
    }
}
inline void zig_zag(signed short* block_ptr) {
    int temp[64];

    for (int i = 0; i < 64; i++) {
        temp[i] = block_ptr[zigzag_index[i]];
    }

    for (int i = 0; i < 64; i++) {
        block_ptr[i] = temp[i];
    }
}

inline int category(int value) {
    int abs_val = (value < 0) ? -value : value;
    int cat = 0;
    while (abs_val) {
        abs_val >>= 1;
        cat++;
    }
    return cat;
}



int serialize_into_jpg(FILE* out, unsigned char* out_buffer, size_t out_buffer_size, signed short* zigzag_qt, int picture_height, int picture_width,
		int* code_lengths_DC, int* code_lengths_AC,int* AC_symbols,int* DC_symbols)
{

	unsigned char jfif_header[] = {
	    0xFF, 0xD8,             // SOI
	    0xFF, 0xEE,             // APP0 (JFIF)
	    0x00, 0x10,             // length = 16
	    'J','F','I','F',0x00,   // "JFIF\0"
	    0x01, 0x01,             // version 1.01
	    0x00,                   // units
	    0x00, 0x01,             // X density
	    0x00, 0x01,             // Y density
	    0x00, 0x00                    // no thumbnail
	};

	fwrite(jfif_header, 1, sizeof(jfif_header), out);

	unsigned char dqt_header[] = {
		0xFF, 0xDB, 0x00, 0x43, 0x00
	};

	for(int i = 0;i<5;i++)
	{
		fputc(dqt_header[i],out);
	}
	for(int i=0;i<64;i++)
	{
		fputc(zigzag_qt[i],out);
	}

	unsigned char dct_header[] = {
		0xFF, 0xC0, 0x00, 0x0B, 0x08, (unsigned char)((picture_height >> 8) & 0xFF), (unsigned char)(picture_height & 0xFF),
		(unsigned char)((picture_width >> 8) & 0xFF), (unsigned char)(picture_width & 0xFF), 0x01,0x01,0x11,0x00
	};
	for(int i =0;i<sizeof(dct_header);i++)
	{
		fputc(dct_header[i],out);
	}

	unsigned char dht_header_1[] = {
		0xFF,0xC4,0x00,0x1F,0x00
	};
	for(int i =0;i<sizeof(dht_header_1);i++)
	{
		fputc(dht_header_1[i],out);
	}

	for(int i =0;i<16;i++)
	{
		fputc(code_lengths_DC[i],out);
	}

	for(int i =0;i<12;i++)
	{
		fputc(DC_symbols[i],out);
	}

	unsigned char dht_header_2[] = {
		0xFF,0xC4,0x00,0xB5,0x10
	};

	for(int i =0;i<sizeof(dht_header_2);i++)
	{
		fputc(dht_header_2[i],out);
	}

	for(int i =0;i<16;i++)
	{
		fputc(code_lengths_AC[i],out);
	}

	for(int i =0;i<162;i++)
	{
		fputc(AC_symbols[i],out);
	}
	unsigned char SOS_header[] = {
		0xFF,0xDA, 0x00, 0x08, 0x01,0x01,0x00, 0x00, 0x3F,0x00
	};
	for(int i =0;i<sizeof(SOS_header);i++)
	{
		fputc(SOS_header[i],out);
	}

	for(int i = 0;i<out_buffer_size;i++)
	{
	    fputc(out_buffer[i],out);
	}

	fputc(0xFF, out);
	fputc(0xD9, out);

	fflush(out);

	return 0;
}


#endif

/* DEBUG_MAIN_OPERATIONS_H_ */


