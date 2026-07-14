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

#if defined(__x86_64__) || defined(__amd64) || defined(_M_AMD64) || defined(_M_X64) || defined(__I386__) || \
defined(__i386__) || defined(__THW_INTEL) || defined(_M_IX86)

#include <stdint.h>
#include <stddef.h>
#include <immintrin.h>

#include "library.h"
#include "crc16.h"

static inline void CRC8_chunk(uint16_t *crc, uint64_t lane)
{
    uint32_t one = (uint32_t)lane ^ (uint32_t)(*crc);
    uint32_t two = (uint32_t)(lane >> 32);

    uint16_t c = crc16_table[0][(two >> 24) & 0xFF] ^ crc16_table[1][(two >> 16) & 0xFF] ^ crc16_table[2][
                     (two >> 8) & 0xFF] ^ crc16_table[3][two & 0xFF] ^ crc16_table[4][(one >> 24) & 0xFF] ^ crc16_table[
                     5][(one >> 16) & 0xFF] ^ crc16_table[6][(one >> 8) & 0xFF] ^ crc16_table[7][one & 0xFF];

    *crc = c;
}

AARU_EXPORT TARGET_WITH_AVX2 int AARU_CALL crc16_update_avx2(crc16_ctx *ctx, const uint8_t *data, uint32_t len)
{
    if(!ctx || !data) return -1;

    uint16_t       crc = ctx->crc;
    const uint8_t *p   = data;

    // Head: align to 32B for nicer load grouping
    uintptr_t mis = (32 - ((uintptr_t)p & 31)) & 31;
    while(len && mis)
    {
        crc = (crc >> 8) ^ crc16_table[0][(crc & 0xFF) ^ *p++];
        len--;
        mis--;
    }

    // Main: 128B per iteration (4x 32B), software-pipelined
    while(len >= 128)
    {
        // Prefetch ahead to keep L1 warm; nudge L2 for bigger strides
        _mm_prefetch((const char*)(p + 256), _MM_HINT_T0);
        _mm_prefetch((const char*)(p + 512), _MM_HINT_T1);

        // Load 4x 32B
        __m256i v0 = _mm256_loadu_si256((const __m256i *)(p + 0));
        __m256i v1 = _mm256_loadu_si256((const __m256i *)(p + 32));
        __m256i v2 = _mm256_loadu_si256((const __m256i *)(p + 64));
        __m256i v3 = _mm256_loadu_si256((const __m256i *)(p + 96));

        // Spill the 16x 64-bit lanes to memory. Storing (rather than _mm_cvtsi128_si64 /
        // _mm_extract_epi64) keeps this portable to 32-bit x86, where those 64-bit GPR
        // intrinsics do not exist; the lanes are consumed scalarly anyway.
        uint64_t lanes[16];
        _mm256_storeu_si256((__m256i *)(lanes + 0), v0);
        _mm256_storeu_si256((__m256i *)(lanes + 4), v1);
        _mm256_storeu_si256((__m256i *)(lanes + 8), v2);
        _mm256_storeu_si256((__m256i *)(lanes + 12), v3);

        // Process in strict stream order (slicing-by-8 semantics)
        CRC8_chunk(&crc, lanes[0]);
        CRC8_chunk(&crc, lanes[1]);
        CRC8_chunk(&crc, lanes[2]);
        CRC8_chunk(&crc, lanes[3]);

        CRC8_chunk(&crc, lanes[4]);
        CRC8_chunk(&crc, lanes[5]);
        CRC8_chunk(&crc, lanes[6]);
        CRC8_chunk(&crc, lanes[7]);

        CRC8_chunk(&crc, lanes[8]);
        CRC8_chunk(&crc, lanes[9]);
        CRC8_chunk(&crc, lanes[10]);
        CRC8_chunk(&crc, lanes[11]);

        CRC8_chunk(&crc, lanes[12]);
        CRC8_chunk(&crc, lanes[13]);
        CRC8_chunk(&crc, lanes[14]);
        CRC8_chunk(&crc, lanes[15]);

        p += 128;
        len -= 128;
    }

    // Drain remaining 32..96 bytes in 32B steps (keeps hot path tight)
    while(len >= 32)
    {
        _mm_prefetch((const char*)(p + 128), _MM_HINT_T0);

        __m256i v = _mm256_loadu_si256((const __m256i *)p);

        uint64_t lanes[4];
        _mm256_storeu_si256((__m256i *)lanes, v);

        CRC8_chunk(&crc, lanes[0]);
        CRC8_chunk(&crc, lanes[1]);
        CRC8_chunk(&crc, lanes[2]);
        CRC8_chunk(&crc, lanes[3]);

        p += 32;
        len -= 32;
    }

    // Tail (<=31 bytes): byte-by-byte, identical to scalar
    while(len--) { crc = (crc >> 8) ^ crc16_table[0][(crc & 0xFF) ^ *p++]; }

    ctx->crc = crc;
    return 0;
}

#endif