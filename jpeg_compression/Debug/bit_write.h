/*
 * bit_write.h
 *
 *  Created on: 17.01.2025.
 *      Author: Korisnik
 */

#ifndef DEBUG_BIT_WRITE_H_
#define DEBUG_BIT_WRITE_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef struct {
    unsigned char *buf;
    size_t   capacity;
    size_t   size;
    unsigned int bitbuf;
    int      bitcount;
} bit_writer_t;

void bw_init(bit_writer_t pm *bw, unsigned char *buffer, size_t cap) {
    bw->buf = buffer;
    bw->capacity = cap;
    bw->size = 0;
    bw->bitbuf = 0;
    bw->bitcount = 0;
}

inline void bw_write_bits(bit_writer_t pm*bw, unsigned short bits, int nbits)
{
    bits &= ((1U << nbits) - 1);

    bw->bitbuf = (bw->bitbuf << nbits) | bits;
    bw->bitcount += nbits;

    while (bw->bitcount >= 8) {
        unsigned char byte =
            (bw->bitbuf >> (bw->bitcount - 8)) & 0xFF;
        bw->bitcount -= 8;

        bw->buf[bw->size++] = byte;
        if (byte == 0xFF)
            bw->buf[bw->size++] = 0x00;
    }

    // remove consumed bits
    bw->bitbuf &= (1ULL << bw->bitcount) - 1;
}


void bw_flush(bit_writer_t pm* bw)
{
    if (bw->bitcount == 0)
        return;

    int pad_bits = (8 - (bw->bitcount & 7)) & 7;

    if (pad_bits) {
        bw->bitbuf = (bw->bitbuf << pad_bits) | ((1U << pad_bits) - 1);
        bw->bitcount += pad_bits;
    }

    while (bw->bitcount >= 8) {
        unsigned char byte =
            (bw->bitbuf >> (bw->bitcount - 8)) & 0xFF;
        bw->bitcount -= 8;

        bw->buf[bw->size++] = byte;
        if (byte == 0xFF)
            bw->buf[bw->size++] = 0x00;
    }

    bw->bitbuf = 0;
    bw->bitcount = 0;
}


#endif /* DEBUG_BIT_WRITE_H_ */
