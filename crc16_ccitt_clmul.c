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

#if defined(__x86_64__) || defined(__amd64) || defined(_M_AMD64) || defined(_M_X64) || defined(__I386__) || \
defined(__i386__) || defined(__THW_INTEL) || defined(_M_IX86)

#include <bits/stdint-uintn.h>
#include <stdint.h>
#include <stddef.h>
#include <immintrin.h>   // for _mm_clmulepi64_si128
#include <wmmintrin.h>   // some compilers need this for PCLMUL

#include "library.h"
#include "crc16_ccitt.h"

#ifndef CRC16_CCITT_POLY
#define CRC16_CCITT_POLY 0x1021u  // x^16 + x^12 + x^5 + 1
#endif

// Carry-less multiply of two 16-bit values -> 32-bit polynomial product.
TARGET_WITH_CLMUL static inline uint32_t clmul16(uint16_t a, uint16_t b)
{
    __m128i va   = _mm_set_epi64x(0, (uint64_t)a);
    __m128i vb   = _mm_set_epi64x(0, (uint64_t)b);
    __m128i prod = _mm_clmulepi64_si128(va, vb, 0x00);
#if defined(_M_X64) || defined(__x86_64__)
    return (uint32_t)_mm_cvtsi128_si64(prod);
#else
    // On 32-bit targets, extract low 64 then cast.
    uint64_t low64; _mm_storel_epi64((__m128i *)&low64, prod); return (uint32_t)low64;
#endif
}

// Reduce a 32-bit polynomial modulo 0x1021 to 16 bits (MSB-first semantics).
static inline uint16_t gf2_reduce32_to16(uint32_t x)
{
    int i;
    // For each set bit at position i >= 16, xor poly shifted by (i-16).
    for(i = 31; i >= 16; --i) { if(x & (1u << i)) x ^= (uint32_t)CRC16_CCITT_POLY << (i - 16); }
    return (uint16_t)x;
}

// GF(2) multiply modulo 0x1021 for 16-bit operands, using PCLMUL for the product.
static inline uint16_t gf2_mul16_mod(uint16_t a, uint16_t b)
{
    uint32_t prod = clmul16(a, b);  // 32-bit polynomial product
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

// Compute CRC of a block starting from crc=0, using YOUR exact slice order (T[7] first).
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

AARU_EXPORT TARGET_WITH_CLMUL int AARU_CALL crc16_ccitt_update_clmul(crc16_ccitt_ctx *ctx, const uint8_t *data,
                                                                     uint32_t         len)
{
    if(!ctx || !data) return -1;

#if defined(__x86_64__) || defined(__amd64) || defined(_M_AMD64) || defined(_M_X64) || defined(__I386__) || \
defined(__i386__) || defined(__THW_INTEL) || defined(_M_IX86)
    if(have_clmul())
        return crc16_ccitt_update_clmul(ctx, data, len);
#endif

    uint16_t crc = ctx->crc;

    // align to 4 bytes, byte-at-a-time.
    uintptr_t unaligned_length = (4 - (((uintptr_t)data) & 3)) & 3;
    while(len && unaligned_length)
    {
        crc = (uint16_t)((crc << 8) ^ crc16_ccitt_table[0][((crc >> 8) ^ *data++) & 0xFF]);
        len--;
        unaligned_length--;
    }

    // Process large blocks via: crc = mul(crc, x^(8*B)) ^ crc_block(0, block)
    // Choose a block size that balances pow() cost and locality.
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

    // Handle the remainder: you can either combine once more, or fall back bytewise.
    // To stay faithful and still leverage PCLMUL combine, do one more combine for the tail.
    if(len >= 8)
    {
        // Combine full 8-byte chunks with a single pow per chunk length (8).
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

#endif