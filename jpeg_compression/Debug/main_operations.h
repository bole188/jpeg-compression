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

void center_pixels(signed short* restrict img_pixels,signed short pm* output_data) {
	for(int i = 64;i!=0;i--)
	{
		int index = 64 -i;
		output_data[index] = (signed short pm)(img_pixels[index])-128;
	}
}


void calculate_inv_qt(float pm* restrict inv_qt)
{
	for(int i  = 64;i != 0; i--)
	{
		int index = i-1;
	    inv_qt[index] = 1.0f / (float)qt_luminance.values[index];
	}
}


void segment_into_blocks(const unsigned char* restrict not_centered_pixels, int width, int height, int* restrict num_blocks_x, int* restrict num_blocks_y, unsigned char* result) {

    for (int by = *num_blocks_y; by != 0 ; by--) {
    	int real_by = *num_blocks_y - by;
        for (int bx = *num_blocks_x; bx != 0; bx--) {
        	int real_bx = *num_blocks_x - bx;
        	int block_idx = (real_by * (*num_blocks_x) + real_bx) * 64;
        	unsigned char* result_p = result + block_idx;
            for (int y = 8; y != 0; y--) {
                for (int x = 8; x != 0; x--) {
                	int real_x = 8 - x;
                	int real_y = 8 - y;
                    int img_x = real_bx*8 + real_x;
                    int img_y = real_by*8 + real_y;
                    int src_x;
                    int src_y;
                    if(expected_true(img_x < width))
                    	src_x = img_x;
                    else
                    	src_x = width -1;

                    if(expected_true(img_y < height))
                    	src_y = img_y;
                    else
                    	src_y = height-1;

                    int not_centered_pixels_index = src_y * width + src_x;
                    *result_p++ =
                    		not_centered_pixels[not_centered_pixels_index];
                }
            }
        }
    }
}
// the following functions operate on a single block, it is required to traverse through all blocks
void dct(signed short pm* block_ptr, float pm* restrict temp)
{

    float intermediate[64];
    float intermediate_2[64];
    float * inter_p = &intermediate[63];
    signed short* block_p = block_ptr;
    const float* cos_lifted_1_T_p = COS_LIFTED_1_T + 63;
    int intermediate_index;
    int block_ptr_index;

	#pragma loop_count(8)
    for (int y = 8; y != 0; y--) {
        int ry = y - 1;
        cos_lifted_1_T_p = COS_LIFTED_1_T + 63;
		#pragma loop_count(8)
        for (int u = 8; u != 0; u--) {
            float sum = 0.0f;
            block_p = block_ptr+(ry*8) + 7;
			#pragma loop_count(8)
            for (int x = 8; x != 0; x--) {
                sum += (float)(*block_p--) * *cos_lifted_1_T_p--;
            }
            *inter_p-- = sum;
        }
    }

    for (int u = 8; u != 0; u--) {
        int ru = u - 1;
        for (int v = 8; v != 0; v--) {
            int rv = v - 1;
            float sum = 0.0f;
            for (int y = 8; y != 0; y--) {
                int ry = y - 1;
                intermediate_index = ry * 8 + ru;
                sum += intermediate[intermediate_index] * COS_LIFTED_2[ry][rv];
            }

            int freq_idx = (rv * 8) + ru;
            temp[freq_idx]  = sum;
        }
    }
}

void quantization(signed short pm* restrict block_ptr,float pm* restrict temp, float pm* restrict inv_qt)
{
	for(int i = 64;i!=0;i--)
	{
		int index = i - 1;
		block_ptr[index] = (short)(temp[index] * inv_qt[index]);
	}
}

void zig_zag(signed short*restrict block_ptr) {
    int temp[64];

    for (int i = 64; i != 0; i--) {
    	int index = 64 - i;
        temp[index] = block_ptr[zigzag_index[index]];
    }
    for (int i = 64; i != 0; i--) {
    	int index = 64 - i;
        block_ptr[index] = temp[index];
    }
}

int category(int value) {
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

	fwrite(dqt_header, 1,sizeof(dqt_header),out);
	fwrite(zigzag_qt, 1,64,out);

	unsigned char dct_header[] = {
		0xFF, 0xC0, 0x00, 0x0B, 0x08, (unsigned char)((picture_height >> 8) & 0xFF), (unsigned char)(picture_height & 0xFF),
		(unsigned char)((picture_width >> 8) & 0xFF), (unsigned char)(picture_width & 0xFF), 0x01,0x01,0x11,0x00
	};
	fwrite(dct_header,1,sizeof(dct_header),out);

	unsigned char dht_header_1[] = {
		0xFF,0xC4,0x00,0x1F,0x00
	};

	fwrite(dht_header_1,1,sizeof(dht_header_1),out);

	fwrite(code_lengths_DC,1,16,out);

	fwrite(DC_symbols,1,12,out);

	unsigned char dht_header_2[] = {
		0xFF,0xC4,0x00,0xB5,0x10
	};

	fwrite(dht_header_2,1,sizeof(dht_header_2),out);

	fwrite(code_lengths_AC,1,16,out);

	fwrite(AC_symbols,1,162,out);

	unsigned char SOS_header[] = {
		0xFF,0xDA, 0x00, 0x08, 0x01,0x01,0x00, 0x00, 0x3F,0x00
	};

	fwrite(SOS_header,1,sizeof(SOS_header),out);

	fwrite(out_buffer,1,out_buffer_size,out);

	fputc(0xFF, out);
	fputc(0xD9, out);

	fflush(out);

	return 0;
}


#endif

/* DEBUG_MAIN_OPERATIONS_H_ */


