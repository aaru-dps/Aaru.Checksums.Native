/*
* This file is part of the Aaru Data Preservation Suite.
 * Copyright (c) 2019-2025 Natalia Portillo.
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

#include <stdint.h>
#include <stddef.h>
#include <arm_neon.h>   // NEON + PMULL (vmull_p64)

#include "library.h"
#include "simd.h"
#include "crc16_ccitt.h"

#ifndef CRC16_CCITT_POLY
#define CRC16_CCITT_POLY 0x1021u  // x^16 + x^12 + x^5 + 1
#endif

// Carry-less multiply of two 16-bit values -> 32-bit polynomial product.
TARGET_WITH_CRYPTO static inline uint32_t pmull16(uint16_t a, uint16_t b)
{
    int i;
#if defined(__ARM_FEATURE_CRYPTO)
    // Treat a, b as 64-bit polynomials with only low 16 bits set, then PMULL.
    poly64_t pa = (poly64_t)(uint64_t)a; poly64_t pb = (poly64_t)(uint64_t)b; poly128_t pr = vmull_p64(pa, pb);
    // 64x64 -> 128 (GF(2))
    uint64_t lo = vgetq_lane_u64(vreinterpretq_u64_p128(pr), 0); return (uint32_t)lo; // low 32 bits hold 16x16 product
#else
    // Portable GF(2) 16x16 multiply (bitwise) if PMULL not available at build time.
    uint32_t p = 0;
    for(i = 0; i < 16; ++i) if(b & (1u << i)) p ^= (uint32_t)a << i;
    return p;
#endif
}

// Reduce a 32-bit polynomial modulo 0x1021 to 16 bits (MSB-first semantics).
static inline uint16_t gf2_reduce32_to16(uint32_t x)
{
    int i;
    for(i = 31; i >= 16; --i) if(x & (1u << i)) x ^= (uint32_t)CRC16_CCITT_POLY << (i - 16);
    return (uint16_t)x;
}

// GF(2) multiply modulo 0x1021 for 16-bit operands, using PMULL for the product when available.
static inline uint16_t gf2_mul16_mod(uint16_t a, uint16_t b)
{
    uint32_t prod = pmull16(a, b);  // 32-bit polynomial product
    return gf2_reduce32_to16(prod); // reduce to 16-bit remainder
}

// Compute x^(8*len) mod P (MSB-first), using exponentiation by squaring.
static inline uint16_t gf2_pow_x8(size_t len)
{
    uint16_t result = 1u;                  // multiplicative identity
    uint16_t base   = (uint16_t)(1u << 8); // x^8 mod P (degree 8 < 16, so unchanged)
    while(len)
    {
        if(len & 1) result = gf2_mul16_mod(result, base);
        base = gf2_mul16_mod(base, base);
        len >>= 1;
    }
    return result;
}

// Compute CRC of a block starting from crc=0, using your exact slice order (T[7] first).
static inline uint16_t crc16_block_slice_by_8(const uint8_t *p, size_t n)
{
    uint16_t c = 0;
    // Align small heads to 8
    while(n && ((uintptr_t)p & 7))
    {
        c = (uint16_t)((c << 8) ^ crc16_ccitt_table[0][((c >> 8) ^ *p++) & 0xFF]);
        n--;
    }
    while(n >= 8)
    {
        c = crc16_ccitt_table[7][p[0] ^ (c >> 8)] ^ crc16_ccitt_table[6][p[1] ^ (c & 0xFF)] ^ crc16_ccitt_table[5][p[2]]
            ^ crc16_ccitt_table[4][p[3]] ^ crc16_ccitt_table[3][p[4]] ^ crc16_ccitt_table[2][p[5]] ^ crc16_ccitt_table[
                1][p[6]] ^ crc16_ccitt_table[0][p[7]];
        p += 8;
        n -= 8;
    }
    while(n--) c = (uint16_t)((c << 8) ^ crc16_ccitt_table[0][((c >> 8) ^ *p++) & 0xFF]);

    return c;
}

AARU_EXPORT TARGET_WITH_CRYPTO int AARU_CALL crc16_ccitt_update_pmull(crc16_ccitt_ctx *ctx, const uint8_t *data,
                                                                      uint32_t         len)
{
    if(!ctx || !data) return -1;

    uint16_t crc = ctx->crc;

    // Align to 4 bytes, byte-at-a-time.
    uintptr_t unaligned_length = (4 - (((uintptr_t)data) & 3)) & 3;
    while(len && unaligned_length)
    {
        crc = (uint16_t)((crc << 8) ^ crc16_ccitt_table[0][((crc >> 8) ^ *data++) & 0xFF]);
        len--;
        unaligned_length--;
    }

    // Process large blocks via: crc = mul(crc, x^(8*B)) ^ crc_block(0, block)
    const size_t   BLOCK     = 64; // 64 bytes per block
    const uint16_t pow_block = gf2_pow_x8(BLOCK);

    while(len >= BLOCK)
    {
        uint16_t block_crc = crc16_block_slice_by_8(data, BLOCK);
        uint16_t folded    = gf2_mul16_mod(crc, pow_block);
        crc                = (uint16_t)(folded ^ block_crc);

        data += BLOCK;
        len -= BLOCK;
    }

    // Handle remainder in 8-byte chunks using the same combine rule.
    if(len >= 8)
    {
        const uint16_t pow8 = gf2_pow_x8(8);
        while(len >= 8)
        {
            uint16_t chunk_crc = crc16_block_slice_by_8(data, 8);
            uint16_t folded    = gf2_mul16_mod(crc, pow8);
            crc                = (uint16_t)(folded ^ chunk_crc);

            data += 8;
            len -= 8;
        }
    }

    // Final tiny tail (<=7 bytes)
    while(len--) crc = (uint16_t)((crc << 8) ^ crc16_ccitt_table[0][((crc >> 8) ^ *data++) & 0xFF]);

    ctx->crc = crc;
    return 0;
}

#endif // ARM/NEON+PMULL
