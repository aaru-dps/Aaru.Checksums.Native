/*
 * This file is part of the Aaru Data Preservation Suite.
 * Copyright (c) 2019-2026 Natalia Portillo.
 * Copyright 2017 The Chromium Authors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *    * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if defined(__aarch64__) || defined(_M_ARM64) || ((defined(__arm__) || defined(_M_ARM)))

#include <arm_neon.h>

#include "library.h"
#include "adler32.h"
#include "simd.h"

/**
 * @brief Calculate Adler-32 checksum for a given data using NEON instructions.
 *
 * This function calculates the Adler-32 checksum for a block of data using NEON vector instructions.
 *
 * @param sum1 Pointer to the variable where the first 16-bit checksum value is stored.
 * @param sum2 Pointer to the variable where the second 16-bit checksum value is stored.
 * @param data Pointer to the data buffer.
 * @param len Length of the data buffer in bytes.
 */
TARGET_WITH_NEON void adler32_neon(uint16_t *sum1, uint16_t *sum2, const uint8_t *data, uint32_t len)
{
    /*
     * Split Adler-32 into component sums.
     */
    uint32_t s1 = *sum1;
    uint32_t s2 = *sum2;

    /*
     * Process the data in blocks.
     */
    const unsigned BLOCK_SIZE = 1 << 5;
    if(len >= BLOCK_SIZE)
    {
        /*
         * Serially compute s1 & s2, until the data is 16-byte aligned.
         */
        if((uintptr_t)data & 15)
        {
            while((uintptr_t)data & 15)
            {
                s2 += (s1 += *data++);
                --len;
            }
            if(s1 >= ADLER_MODULE) s1 -= ADLER_MODULE;
            s2 %= ADLER_MODULE;
        }

        uint32_t blocks = len / BLOCK_SIZE;
        len -= blocks * BLOCK_SIZE;
        while(blocks)
        {
            unsigned n = NMAX / BLOCK_SIZE; /* The NMAX constraint. */
            if(n > blocks) n = (unsigned)blocks;
            blocks -= n;
            /*
             * Process n blocks of data. At most NMAX data bytes can be
             * processed before s2 must be reduced modulo ADLER_MODULE.
             */
#ifdef _MSC_VER
            uint32x4_t v_s2 = {
                .n128_u32 = {0, 0, 0, s1 * n}
            };
            uint32x4_t v_s1 = {
                .n128_u32 = {0, 0, 0, 0}
            };
#else
            uint32x4_t v_s2 = (uint32x4_t){0, 0, 0, s1 * n};
            uint32x4_t v_s1 = (uint32x4_t){0, 0, 0, 0};
#endif
            uint16x8_t v_column_sum_1 = vdupq_n_u16(0);
            uint16x8_t v_column_sum_2 = vdupq_n_u16(0);
            uint16x8_t v_column_sum_3 = vdupq_n_u16(0);
            uint16x8_t v_column_sum_4 = vdupq_n_u16(0);
            do {
                /*
                 * Load 32 input bytes.
                 */
                const uint8x16_t bytes1 = vld1q_u8((uint8_t *)(data));
                const uint8x16_t bytes2 = vld1q_u8((uint8_t *)(data + 16));
                /*
                 * Add previous block byte sum to v_s2.
                 */
                v_s2                    = vaddq_u32(v_s2, v_s1);
                /*
                 * Horizontally add the bytes for s1.
                 */
                v_s1                    = vpadalq_u16(v_s1, vpadalq_u8(vpaddlq_u8(bytes1), bytes2));
                /*
                 * Vertically add the bytes for s2.
                 */
                v_column_sum_1          = vaddw_u8(v_column_sum_1, vget_low_u8(bytes1));
                v_column_sum_2          = vaddw_u8(v_column_sum_2, vget_high_u8(bytes1));
                v_column_sum_3          = vaddw_u8(v_column_sum_3, vget_low_u8(bytes2));
                v_column_sum_4          = vaddw_u8(v_column_sum_4, vget_high_u8(bytes2));
                data += BLOCK_SIZE;
            } while(--n);
            v_s2 = vshlq_n_u32(v_s2, 5);
            /*
             * Multiply-add bytes by [ 32, 31, 30, ... ] for s2.
             */
#ifdef _MSC_VER
#ifdef _M_ARM64
            v_s2 = vmlal_u16(v_s2, vget_low_u16(v_column_sum_1), neon_ld1m_16((uint16_t[]){32, 31, 30, 29}));
            v_s2 = vmlal_u16(v_s2, vget_high_u16(v_column_sum_1), neon_ld1m_16((uint16_t[]){28, 27, 26, 25}));
            v_s2 = vmlal_u16(v_s2, vget_low_u16(v_column_sum_2), neon_ld1m_16((uint16_t[]){24, 23, 22, 21}));
            v_s2 = vmlal_u16(v_s2, vget_high_u16(v_column_sum_2), neon_ld1m_16((uint16_t[]){20, 19, 18, 17}));
            v_s2 = vmlal_u16(v_s2, vget_low_u16(v_column_sum_3), neon_ld1m_16((uint16_t[]){16, 15, 14, 13}));
            v_s2 = vmlal_u16(v_s2, vget_high_u16(v_column_sum_3), neon_ld1m_16((uint16_t[]){12, 11, 10, 9}));
            v_s2 = vmlal_u16(v_s2, vget_low_u16(v_column_sum_4), neon_ld1m_16((uint16_t[]){8, 7, 6, 5}));
            v_s2 = vmlal_u16(v_s2, vget_high_u16(v_column_sum_4), neon_ld1m_16((uint16_t[]){4, 3, 2, 1}));
#else
            v_s2 = vmlal_u16(v_s2, vget_low_u16(v_column_sum_1), vld1_u16(((uint16_t[]){32, 31, 30, 29})));
            v_s2 = vmlal_u16(v_s2, vget_high_u16(v_column_sum_1), vld1_u16(((uint16_t[]){28, 27, 26, 25})));
            v_s2 = vmlal_u16(v_s2, vget_low_u16(v_column_sum_2), vld1_u16(((uint16_t[]){24, 23, 22, 21})));
            v_s2 = vmlal_u16(v_s2, vget_high_u16(v_column_sum_2), vld1_u16(((uint16_t[]){20, 19, 18, 17})));
            v_s2 = vmlal_u16(v_s2, vget_low_u16(v_column_sum_3), vld1_u16(((uint16_t[]){16, 15, 14, 13})));
            v_s2 = vmlal_u16(v_s2, vget_high_u16(v_column_sum_3), vld1_u16(((uint16_t[]){12, 11, 10, 9})));
            v_s2 = vmlal_u16(v_s2, vget_low_u16(v_column_sum_4), vld1_u16(((uint16_t[]){8, 7, 6, 5})));
            v_s2 = vmlal_u16(v_s2, vget_high_u16(v_column_sum_4), vld1_u16(((uint16_t[]){4, 3, 2, 1})));
#endif
#else
            v_s2 = vmlal_u16(v_s2, vget_low_u16(v_column_sum_1), (uint16x4_t){32, 31, 30, 29});
            v_s2 = vmlal_u16(v_s2, vget_high_u16(v_column_sum_1), (uint16x4_t){28, 27, 26, 25});
            v_s2 = vmlal_u16(v_s2, vget_low_u16(v_column_sum_2), (uint16x4_t){24, 23, 22, 21});
            v_s2 = vmlal_u16(v_s2, vget_high_u16(v_column_sum_2), (uint16x4_t){20, 19, 18, 17});
            v_s2 = vmlal_u16(v_s2, vget_low_u16(v_column_sum_3), (uint16x4_t){16, 15, 14, 13});
            v_s2 = vmlal_u16(v_s2, vget_high_u16(v_column_sum_3), (uint16x4_t){12, 11, 10, 9});
            v_s2 = vmlal_u16(v_s2, vget_low_u16(v_column_sum_4), (uint16x4_t){8, 7, 6, 5});
            v_s2 = vmlal_u16(v_s2, vget_high_u16(v_column_sum_4), (uint16x4_t){4, 3, 2, 1});
#endif
            /*
             * Sum epi32 ints v_s1(s2) and accumulate in s1(s2).
             */
            uint32x2_t t_s1 = vpadd_u32(vget_low_u32(v_s1), vget_high_u32(v_s1));
            uint32x2_t t_s2 = vpadd_u32(vget_low_u32(v_s2), vget_high_u32(v_s2));
            uint32x2_t s1s2 = vpadd_u32(t_s1, t_s2);
            s1 += vget_lane_u32(s1s2, 0);
            s2 += vget_lane_u32(s1s2, 1);
            /*
             * Reduce.
             */
            s1 %= ADLER_MODULE;
            s2 %= ADLER_MODULE;
        }
    }

    /*
     * Handle leftover data.
     */
    if(len)
    {
        if(len >= 16)
        {
            s2 += (s1 += *data++);
            s2 += (s1 += *data++);
            s2 += (s1 += *data++);
            s2 += (s1 += *data++);
            s2 += (s1 += *data++);
            s2 += (s1 += *data++);
            s2 += (s1 += *data++);
            s2 += (s1 += *data++);
            s2 += (s1 += *data++);
            s2 += (s1 += *data++);
            s2 += (s1 += *data++);
            s2 += (s1 += *data++);
            s2 += (s1 += *data++);
            s2 += (s1 += *data++);
            s2 += (s1 += *data++);
            s2 += (s1 += *data++);
            len -= 16;
        }
        while(len--) { s2 += (s1 += *data++); }
        if(s1 >= ADLER_MODULE) s1 -= ADLER_MODULE;
        s2 %= ADLER_MODULE;
    }

    /*
     * Return the recombined sums.
     */
    *sum1 = s1 & 0xFFFF;
    *sum2 = s2 & 0xFFFF;
}

#endif
