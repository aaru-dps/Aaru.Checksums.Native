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

#include <stdint.h>
#include <stddef.h>
#include <arm_neon.h>

#include "library.h"
#include "crc16.h"

#ifndef AARU_LIKELY
#  define AARU_LIKELY(x)   __builtin_expect(!!(x), 1)
#  define AARU_UNLIKELY(x) __builtin_expect(!!(x), 0)
#endif

// Core table-driven 8-byte update (same as your scalar logic)
static inline __attribute__((always_inline)) void CRC8_chunk(uint16_t *crc, uint64_t lane)
{
    uint32_t one = (uint32_t)lane ^ (uint32_t)(*crc);
    uint32_t two = (uint32_t)(lane >> 32);

    uint16_t c = crc16_table[0][(two >> 24) & 0xFF] ^ crc16_table[1][(two >> 16) & 0xFF] ^ crc16_table[2][
                     (two >> 8) & 0xFF] ^ crc16_table[3][two & 0xFF] ^ crc16_table[4][(one >> 24) & 0xFF] ^ crc16_table[
                     5][(one >> 16) & 0xFF] ^ crc16_table[6][(one >> 8) & 0xFF] ^ crc16_table[7][one & 0xFF];

    *crc = c;
}

// Feed two sequential 64-bit lanes from a 16B vector
static inline __attribute__((always_inline)) void CRC8_from_vec(uint16_t *crc, uint8x16_t v)
{
    uint64x2_t lanes = vreinterpretq_u64_u8(v);
    CRC8_chunk(crc, vgetq_lane_u64(lanes, 0));
    CRC8_chunk(crc, vgetq_lane_u64(lanes, 1));
}

// Unrolled helper: apply CRC8_from_vec over 16 vectors (256B)
#define CRC8_FROM_16V(crc, base)                          \
    do {                                                  \
        uint8x16_t v0  = vld1q_u8((base) +   0);          \
        uint8x16_t v1  = vld1q_u8((base) +  16);          \
        uint8x16_t v2  = vld1q_u8((base) +  32);          \
        uint8x16_t v3  = vld1q_u8((base) +  48);          \
        uint8x16_t v4  = vld1q_u8((base) +  64);          \
        uint8x16_t v5  = vld1q_u8((base) +  80);          \
        uint8x16_t v6  = vld1q_u8((base) +  96);          \
        uint8x16_t v7  = vld1q_u8((base) + 112);          \
        uint8x16_t v8  = vld1q_u8((base) + 128);          \
        uint8x16_t v9  = vld1q_u8((base) + 144);          \
        uint8x16_t v10 = vld1q_u8((base) + 160);          \
        uint8x16_t v11 = vld1q_u8((base) + 176);          \
        uint8x16_t v12 = vld1q_u8((base) + 192);          \
        uint8x16_t v13 = vld1q_u8((base) + 208);          \
        uint8x16_t v14 = vld1q_u8((base) + 224);          \
        uint8x16_t v15 = vld1q_u8((base) + 240);          \
        /* mix while L2 prefetch of the tail is in flight */ \
        CRC8_from_vec(&(crc), v0);                        \
        CRC8_from_vec(&(crc), v1);                        \
        CRC8_from_vec(&(crc), v2);                        \
        CRC8_from_vec(&(crc), v3);                        \
        CRC8_from_vec(&(crc), v4);                        \
        CRC8_from_vec(&(crc), v5);                        \
        CRC8_from_vec(&(crc), v6);                        \
        CRC8_from_vec(&(crc), v7);                        \
        CRC8_from_vec(&(crc), v8);                        \
        CRC8_from_vec(&(crc), v9);                        \
        CRC8_from_vec(&(crc), v10);                       \
        CRC8_from_vec(&(crc), v11);                       \
        CRC8_from_vec(&(crc), v12);                       \
        CRC8_from_vec(&(crc), v13);                       \
        CRC8_from_vec(&(crc), v14);                       \
        CRC8_from_vec(&(crc), v15);                       \
    } while(0)

AARU_EXPORT TARGET_WITH_NEON int AARU_CALL crc16_update_vmull(crc16_ctx *ctx, const uint8_t *data, uint32_t len)
{
    if(AARU_UNLIKELY(!ctx || !data)) return -1;

    uint16_t       crc = ctx->crc;
    const uint8_t *p   = data;

    // Head: align to 16B boundary for cheaper vld1q
    uintptr_t mis = (16u - ((uintptr_t)p & 15u)) & 15u;
    while(len && mis)
    {
        crc = (crc >> 8) ^ crc16_table[0][(crc & 0xFF) ^ *p++];
        len--;
        mis--;
    }

    // Hot path: 256 bytes per iteration (16 × 16B), reduces loop overhead
    while(AARU_LIKELY(len >= 256))
    {
        // Pull in two future lines to L1/L2
        __builtin_prefetch(p + 512, 0, 3);
        __builtin_prefetch(p + 1024, 0, 2);

        CRC8_FROM_16V(crc, p);

        p += 256;
        len -= 256;
    }

    // Secondary hot path: 128B chunks if any remain
    while(len >= 128)
    {
        __builtin_prefetch(p + 256, 0, 3);

        uint8x16_t v0 = vld1q_u8(p + 0);
        uint8x16_t v1 = vld1q_u8(p + 16);
        uint8x16_t v2 = vld1q_u8(p + 32);
        uint8x16_t v3 = vld1q_u8(p + 48);
        uint8x16_t v4 = vld1q_u8(p + 64);
        uint8x16_t v5 = vld1q_u8(p + 80);
        uint8x16_t v6 = vld1q_u8(p + 96);
        uint8x16_t v7 = vld1q_u8(p + 112);

        CRC8_from_vec(&crc, v0);
        CRC8_from_vec(&crc, v1);
        CRC8_from_vec(&crc, v2);
        CRC8_from_vec(&crc, v3);
        CRC8_from_vec(&crc, v4);
        CRC8_from_vec(&crc, v5);
        CRC8_from_vec(&crc, v6);
        CRC8_from_vec(&crc, v7);

        p += 128;
        len -= 128;
    }

    // Drain: 16B steps, then scalar tail
    while(len >= 16)
    {
        __builtin_prefetch(p + 128, 0, 3);
        CRC8_from_vec(&crc, vld1q_u8(p));
        p += 16;
        len -= 16;
    }

    while(len--) crc = (crc >> 8) ^ crc16_table[0][(crc & 0xFF) ^ *p++];

    ctx->crc = crc;
    return 0;
}
#endif
