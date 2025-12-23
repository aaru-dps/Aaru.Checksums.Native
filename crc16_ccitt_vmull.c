/*
* This file is part of the Aaru Data Preservation Suite.
 * Copyright (c) 2019-2026 Natalia Portillo.
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#if defined(__aarch64__) || defined(_M_ARM64) || defined(__arm__) || defined(_M_ARM)

#include <arm_neon.h>
#include <stddef.h>

#include "library.h"
#include "crc16_ccitt.h"

#ifndef CRC16_CCITT_POLY
#define CRC16_CCITT_POLY 0x1021u
#endif

// Scalar GF(2) multiply/reduce
static inline uint16_t gf2_mul16_mod(uint16_t a, uint16_t b)
{
    uint32_t p = 0;
    int      i;
    for(i = 0; i < 16; ++i) if(b & (1u << i)) p ^= (uint32_t)a << i;
    for(i = 31; i >= 16; --i) if(p & (1u << i)) p ^= (uint32_t)CRC16_CCITT_POLY << (i - 16);
    return (uint16_t)p;
}

static inline uint16_t gf2_pow_x8(size_t len)
{
    uint16_t result = 1u;
    uint16_t base   = (uint16_t)(1u << 8);
    while(len)
    {
        if(len & 1) result = gf2_mul16_mod(result, base);
        base = gf2_mul16_mod(base, base);
        len >>= 1;
    }
    return result;
}

static inline uint16_t crc16_block_slice_by_8_vmull(const uint8_t *p, size_t n)
{
    uint16_t c = 0;

    // Align head to 8 bytes
    while(n && ((uintptr_t)p & 7))
    {
        c = (uint16_t)((c << 8) ^ crc16_ccitt_table[0][((c >> 8) ^ *p++) & 0xFF]);
        n--;
    }

    // Process 8-byte groups
    while(n >= 8)
    {
        uint8x8_t bytes = vld1_u8(p);

        // First lane needs XOR with high byte of CRC
        uint8_t idx0 = vget_lane_u8(bytes, 0) ^ (c >> 8);
        uint8_t idx1 = vget_lane_u8(bytes, 1) ^ (c & 0xFF);

        // Remaining lanes are direct
        uint8_t idx2 = vget_lane_u8(bytes, 2);
        uint8_t idx3 = vget_lane_u8(bytes, 3);
        uint8_t idx4 = vget_lane_u8(bytes, 4);
        uint8_t idx5 = vget_lane_u8(bytes, 5);
        uint8_t idx6 = vget_lane_u8(bytes, 6);
        uint8_t idx7 = vget_lane_u8(bytes, 7);

        // Lookups in slice-by-8 order
        c = crc16_ccitt_table[7][idx0] ^ crc16_ccitt_table[6][idx1] ^ crc16_ccitt_table[5][idx2] ^ crc16_ccitt_table[4][
                idx3] ^ crc16_ccitt_table[3][idx4] ^ crc16_ccitt_table[2][idx5] ^ crc16_ccitt_table[1][idx6] ^
            crc16_ccitt_table[0][idx7];

        p += 8;
        n -= 8;
    }

    // Tail
    while(n--) c = (uint16_t)((c << 8) ^ crc16_ccitt_table[0][((c >> 8) ^ *p++) & 0xFF]);

    return c;
}

AARU_EXPORT TARGET_WITH_NEON int AARU_CALL crc16_ccitt_update_vmull(crc16_ccitt_ctx *ctx, const uint8_t *data,
                                                                    uint32_t         len)
{
    if(!ctx || !data) return -1;
    uint16_t crc = ctx->crc;

    // Align to 4 bytes, byte-at-a-time
    uintptr_t unaligned_length = (4 - (((uintptr_t)data) & 3)) & 3;
    while(len && unaligned_length)
    {
        crc = (uint16_t)((crc << 8) ^ crc16_ccitt_table[0][((crc >> 8) ^ *data++) & 0xFF]);
        len--;
        unaligned_length--;
    }

    const size_t   BLOCK     = 64;
    const uint16_t pow_block = gf2_pow_x8(BLOCK);

    while(len >= BLOCK)
    {
        uint16_t block_crc = crc16_block_slice_by_8_vmull(data, BLOCK);
        uint16_t folded    = gf2_mul16_mod(crc, pow_block);
        crc                = (uint16_t)(folded ^ block_crc);
        data += BLOCK;
        len -= BLOCK;
    }

    if(len >= 8)
    {
        const uint16_t pow8 = gf2_pow_x8(8);
        while(len >= 8)
        {
            uint16_t chunk_crc = crc16_block_slice_by_8_vmull(data, 8);
            uint16_t folded    = gf2_mul16_mod(crc, pow8);
            crc                = (uint16_t)(folded ^ chunk_crc);
            data += 8;
            len -= 8;
        }
    }

    while(len--) crc = (uint16_t)((crc << 8) ^ crc16_ccitt_table[0][((crc >> 8) ^ *data++) & 0xFF]);

    ctx->crc = crc;
    return 0;
}

#endif // __ARM_NEON
