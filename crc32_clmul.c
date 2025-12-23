/*
 * This file is part of the Aaru Data Preservation Suite.
 * Copyright © 2011-2026 Natalia Portillo
 * Copyright (c) 2016 Marian Beermann (add support for initial value, restructuring)
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 *   1. The origin of this software must not be misrepresented; you must not
 *      claim that you wrote the original software. If you use this software
 *      in a product, an acknowledgment in the product documentation would be
 *      appreciated but is not required.
 *
 *   2. Altered source versions must be plainly marked as such, and must not be
 *      misrepresented as being the original software.
 *
 *   3. This notice may not be removed or altered from any source distribution.
 */

#if defined(__x86_64__) || defined(__amd64) || defined(_M_AMD64) || defined(_M_X64) || defined(__I386__) || \
    defined(__i386__) || defined(__THW_INTEL) || defined(_M_IX86)

#include <inttypes.h>
#include <smmintrin.h>
#include <wmmintrin.h>

#include "library.h"
#include "crc32.h"
#include "crc32_simd.h"

TARGET_WITH_CLMUL static void fold_1(__m128i *xmm_crc0, __m128i *xmm_crc1, __m128i *xmm_crc2, __m128i *xmm_crc3)
{
    const __m128i xmm_fold4 = _mm_set_epi32(0x00000001, 0x54442bd4, 0x00000001, 0xc6e41596);

    __m128i x_tmp3;
    __m128  ps_crc0, ps_crc3, ps_res;

    x_tmp3 = *xmm_crc3;

    *xmm_crc3 = *xmm_crc0;
    *xmm_crc0 = _mm_clmulepi64_si128(*xmm_crc0, xmm_fold4, 0x01);
    *xmm_crc3 = _mm_clmulepi64_si128(*xmm_crc3, xmm_fold4, 0x10);
    ps_crc0   = _mm_castsi128_ps(*xmm_crc0);
    ps_crc3   = _mm_castsi128_ps(*xmm_crc3);
    ps_res    = _mm_xor_ps(ps_crc0, ps_crc3);

    *xmm_crc0 = *xmm_crc1;
    *xmm_crc1 = *xmm_crc2;
    *xmm_crc2 = x_tmp3;
    *xmm_crc3 = _mm_castps_si128(ps_res);
}

TARGET_WITH_CLMUL static void fold_2(__m128i *xmm_crc0, __m128i *xmm_crc1, __m128i *xmm_crc2, __m128i *xmm_crc3)
{
    const __m128i xmm_fold4 = _mm_set_epi32(0x00000001, 0x54442bd4, 0x00000001, 0xc6e41596);

    __m128i x_tmp3, x_tmp2;
    __m128  ps_crc0, ps_crc1, ps_crc2, ps_crc3, ps_res31, ps_res20;

    x_tmp3 = *xmm_crc3;
    x_tmp2 = *xmm_crc2;

    *xmm_crc3 = *xmm_crc1;
    *xmm_crc1 = _mm_clmulepi64_si128(*xmm_crc1, xmm_fold4, 0x01);
    *xmm_crc3 = _mm_clmulepi64_si128(*xmm_crc3, xmm_fold4, 0x10);
    ps_crc3   = _mm_castsi128_ps(*xmm_crc3);
    ps_crc1   = _mm_castsi128_ps(*xmm_crc1);
    ps_res31  = _mm_xor_ps(ps_crc3, ps_crc1);

    *xmm_crc2 = *xmm_crc0;
    *xmm_crc0 = _mm_clmulepi64_si128(*xmm_crc0, xmm_fold4, 0x01);
    *xmm_crc2 = _mm_clmulepi64_si128(*xmm_crc2, xmm_fold4, 0x10);
    ps_crc0   = _mm_castsi128_ps(*xmm_crc0);
    ps_crc2   = _mm_castsi128_ps(*xmm_crc2);
    ps_res20  = _mm_xor_ps(ps_crc0, ps_crc2);

    *xmm_crc0 = x_tmp2;
    *xmm_crc1 = x_tmp3;
    *xmm_crc2 = _mm_castps_si128(ps_res20);
    *xmm_crc3 = _mm_castps_si128(ps_res31);
}

TARGET_WITH_CLMUL static void fold_3(__m128i *xmm_crc0, __m128i *xmm_crc1, __m128i *xmm_crc2, __m128i *xmm_crc3)
{
    const __m128i xmm_fold4 = _mm_set_epi32(0x00000001, 0x54442bd4, 0x00000001, 0xc6e41596);

    __m128i x_tmp3;
    __m128  ps_crc0, ps_crc1, ps_crc2, ps_crc3, ps_res32, ps_res21, ps_res10;

    x_tmp3 = *xmm_crc3;

    *xmm_crc3 = *xmm_crc2;
    *xmm_crc2 = _mm_clmulepi64_si128(*xmm_crc2, xmm_fold4, 0x01);
    *xmm_crc3 = _mm_clmulepi64_si128(*xmm_crc3, xmm_fold4, 0x10);
    ps_crc2   = _mm_castsi128_ps(*xmm_crc2);
    ps_crc3   = _mm_castsi128_ps(*xmm_crc3);
    ps_res32  = _mm_xor_ps(ps_crc2, ps_crc3);

    *xmm_crc2 = *xmm_crc1;
    *xmm_crc1 = _mm_clmulepi64_si128(*xmm_crc1, xmm_fold4, 0x01);
    *xmm_crc2 = _mm_clmulepi64_si128(*xmm_crc2, xmm_fold4, 0x10);
    ps_crc1   = _mm_castsi128_ps(*xmm_crc1);
    ps_crc2   = _mm_castsi128_ps(*xmm_crc2);
    ps_res21  = _mm_xor_ps(ps_crc1, ps_crc2);

    *xmm_crc1 = *xmm_crc0;
    *xmm_crc0 = _mm_clmulepi64_si128(*xmm_crc0, xmm_fold4, 0x01);
    *xmm_crc1 = _mm_clmulepi64_si128(*xmm_crc1, xmm_fold4, 0x10);
    ps_crc0   = _mm_castsi128_ps(*xmm_crc0);
    ps_crc1   = _mm_castsi128_ps(*xmm_crc1);
    ps_res10  = _mm_xor_ps(ps_crc0, ps_crc1);

    *xmm_crc0 = x_tmp3;
    *xmm_crc1 = _mm_castps_si128(ps_res10);
    *xmm_crc2 = _mm_castps_si128(ps_res21);
    *xmm_crc3 = _mm_castps_si128(ps_res32);
}

TARGET_WITH_CLMUL static void fold_4(__m128i *xmm_crc0, __m128i *xmm_crc1, __m128i *xmm_crc2, __m128i *xmm_crc3)
{
    const __m128i xmm_fold4 = _mm_set_epi32(0x00000001, 0x54442bd4, 0x00000001, 0xc6e41596);

    __m128i x_tmp0, x_tmp1, x_tmp2, x_tmp3;
    __m128  ps_crc0, ps_crc1, ps_crc2, ps_crc3;
    __m128  ps_t0, ps_t1, ps_t2, ps_t3;
    __m128  ps_res0, ps_res1, ps_res2, ps_res3;

    x_tmp0 = *xmm_crc0;
    x_tmp1 = *xmm_crc1;
    x_tmp2 = *xmm_crc2;
    x_tmp3 = *xmm_crc3;

    *xmm_crc0 = _mm_clmulepi64_si128(*xmm_crc0, xmm_fold4, 0x01);
    x_tmp0    = _mm_clmulepi64_si128(x_tmp0, xmm_fold4, 0x10);
    ps_crc0   = _mm_castsi128_ps(*xmm_crc0);
    ps_t0     = _mm_castsi128_ps(x_tmp0);
    ps_res0   = _mm_xor_ps(ps_crc0, ps_t0);

    *xmm_crc1 = _mm_clmulepi64_si128(*xmm_crc1, xmm_fold4, 0x01);
    x_tmp1    = _mm_clmulepi64_si128(x_tmp1, xmm_fold4, 0x10);
    ps_crc1   = _mm_castsi128_ps(*xmm_crc1);
    ps_t1     = _mm_castsi128_ps(x_tmp1);
    ps_res1   = _mm_xor_ps(ps_crc1, ps_t1);

    *xmm_crc2 = _mm_clmulepi64_si128(*xmm_crc2, xmm_fold4, 0x01);
    x_tmp2    = _mm_clmulepi64_si128(x_tmp2, xmm_fold4, 0x10);
    ps_crc2   = _mm_castsi128_ps(*xmm_crc2);
    ps_t2     = _mm_castsi128_ps(x_tmp2);
    ps_res2   = _mm_xor_ps(ps_crc2, ps_t2);

    *xmm_crc3 = _mm_clmulepi64_si128(*xmm_crc3, xmm_fold4, 0x01);
    x_tmp3    = _mm_clmulepi64_si128(x_tmp3, xmm_fold4, 0x10);
    ps_crc3   = _mm_castsi128_ps(*xmm_crc3);
    ps_t3     = _mm_castsi128_ps(x_tmp3);
    ps_res3   = _mm_xor_ps(ps_crc3, ps_t3);

    *xmm_crc0 = _mm_castps_si128(ps_res0);
    *xmm_crc1 = _mm_castps_si128(ps_res1);
    *xmm_crc2 = _mm_castps_si128(ps_res2);
    *xmm_crc3 = _mm_castps_si128(ps_res3);
}

TARGET_WITH_CLMUL static void partial_fold(const size_t len, __m128i *xmm_crc0, __m128i *xmm_crc1, __m128i *xmm_crc2,
                                           __m128i *xmm_crc3, __m128i *xmm_crc_part)
{
    const __m128i xmm_fold4 = _mm_set_epi32(0x00000001, 0x54442bd4, 0x00000001, 0xc6e41596);
    const __m128i xmm_mask3 = _mm_set1_epi32(0x80808080);

    __m128i xmm_shl, xmm_shr, xmm_tmp1, xmm_tmp2, xmm_tmp3;
    __m128i xmm_a0_0, xmm_a0_1;
    __m128  ps_crc3, psa0_0, psa0_1, ps_res;

    xmm_shl = _mm_load_si128((__m128i *)pshufb_shf_table + (len - 1));
    xmm_shr = xmm_shl;
    xmm_shr = _mm_xor_si128(xmm_shr, xmm_mask3);

    xmm_a0_0 = _mm_shuffle_epi8(*xmm_crc0, xmm_shl);

    *xmm_crc0 = _mm_shuffle_epi8(*xmm_crc0, xmm_shr);
    xmm_tmp1  = _mm_shuffle_epi8(*xmm_crc1, xmm_shl);
    *xmm_crc0 = _mm_or_si128(*xmm_crc0, xmm_tmp1);

    *xmm_crc1 = _mm_shuffle_epi8(*xmm_crc1, xmm_shr);
    xmm_tmp2  = _mm_shuffle_epi8(*xmm_crc2, xmm_shl);
    *xmm_crc1 = _mm_or_si128(*xmm_crc1, xmm_tmp2);

    *xmm_crc2 = _mm_shuffle_epi8(*xmm_crc2, xmm_shr);
    xmm_tmp3  = _mm_shuffle_epi8(*xmm_crc3, xmm_shl);
    *xmm_crc2 = _mm_or_si128(*xmm_crc2, xmm_tmp3);

    *xmm_crc3     = _mm_shuffle_epi8(*xmm_crc3, xmm_shr);
    *xmm_crc_part = _mm_shuffle_epi8(*xmm_crc_part, xmm_shl);
    *xmm_crc3     = _mm_or_si128(*xmm_crc3, *xmm_crc_part);

    xmm_a0_1 = _mm_clmulepi64_si128(xmm_a0_0, xmm_fold4, 0x10);
    xmm_a0_0 = _mm_clmulepi64_si128(xmm_a0_0, xmm_fold4, 0x01);

    ps_crc3 = _mm_castsi128_ps(*xmm_crc3);
    psa0_0  = _mm_castsi128_ps(xmm_a0_0);
    psa0_1  = _mm_castsi128_ps(xmm_a0_1);

    ps_res = _mm_xor_ps(ps_crc3, psa0_0);
    ps_res = _mm_xor_ps(ps_res, psa0_1);

    *xmm_crc3 = _mm_castps_si128(ps_res);
}

/*
 * somewhat surprisingly the "naive" way of doing this, ie. with a flag and a cond. branch,
 * is consistently ~5 % faster on average than the implied-recommended branchless way (always xor,
 * always zero xmm_initial). Guess speculative execution and branch prediction got the better of
 * yet another "optimization tip".
 */
#define XOR_INITIAL(where) ONCE(where = _mm_xor_si128(where, xmm_initial))

/**
 * @brief Calculate the CRC32 checksum using CLMUL instruction extension.
 *
 * @param previous_crc The previously calculated CRC32 checksum.
 * @param data Pointer to the input data buffer.
 * @param len Length of the input data in bytes.
 *
 * @return The calculated CRC32 checksum.
 */
AARU_EXPORT TARGET_WITH_CLMUL uint32_t AARU_CALL crc32_clmul(uint32_t previous_crc, const uint8_t *data, long len)
{
    unsigned long algn_diff;
    __m128i       xmm_t0, xmm_t1, xmm_t2, xmm_t3;
    __m128i       xmm_initial = _mm_cvtsi32_si128(previous_crc);
    __m128i       xmm_crc0    = _mm_cvtsi32_si128(0x9db42487);
    __m128i       xmm_crc1    = _mm_setzero_si128();
    __m128i       xmm_crc2    = _mm_setzero_si128();
    __m128i       xmm_crc3    = _mm_setzero_si128();
    __m128i       xmm_crc_part;

    int first = 1;

    /* fold 512 to 32 step variable declarations for ISO-C90 compat. */
    const __m128i xmm_mask  = _mm_load_si128((__m128i *)crc_mask);
    const __m128i xmm_mask2 = _mm_load_si128((__m128i *)crc_mask2);

    uint32_t crc;
    __m128i  x_tmp0, x_tmp1, x_tmp2, crc_fold;

    if(len < 16)
    {
        if(len == 0) return previous_crc;
        if(len < 4)
        {
            /*
             * no idea how to do this for <4 bytes, delegate to classic impl.
             */
            uint32_t crc = ~previous_crc;
            switch(len)
            {
                case 3:
                    crc = (crc >> 8) ^ crc32_table[0][(crc & 0xFF) ^ *data++];
                case 2:
                    crc = (crc >> 8) ^ crc32_table[0][(crc & 0xFF) ^ *data++];
                case 1:
                    crc = (crc >> 8) ^ crc32_table[0][(crc & 0xFF) ^ *data++];
            }
            return ~crc;
        }
        xmm_crc_part = _mm_loadu_si128((__m128i *)data);
        XOR_INITIAL(xmm_crc_part);
        goto partial;
    }

    /* this alignment computation would be wrong for len<16 handled above */
    algn_diff = (0 - (uintptr_t)data) & 0xF;
    if(algn_diff)
    {
        xmm_crc_part = _mm_loadu_si128((__m128i *)data);
        XOR_INITIAL(xmm_crc_part);

        data += algn_diff;
        len -= algn_diff;

        partial_fold(algn_diff, &xmm_crc0, &xmm_crc1, &xmm_crc2, &xmm_crc3, &xmm_crc_part);
    }

    while((len -= 64) >= 0)
    {
        xmm_t0 = _mm_load_si128((__m128i *)data);
        xmm_t1 = _mm_load_si128((__m128i *)data + 1);
        xmm_t2 = _mm_load_si128((__m128i *)data + 2);
        xmm_t3 = _mm_load_si128((__m128i *)data + 3);

        XOR_INITIAL(xmm_t0);

        fold_4(&xmm_crc0, &xmm_crc1, &xmm_crc2, &xmm_crc3);

        xmm_crc0 = _mm_xor_si128(xmm_crc0, xmm_t0);
        xmm_crc1 = _mm_xor_si128(xmm_crc1, xmm_t1);
        xmm_crc2 = _mm_xor_si128(xmm_crc2, xmm_t2);
        xmm_crc3 = _mm_xor_si128(xmm_crc3, xmm_t3);

        data += 64;
    }

    /*
     * len = num bytes left - 64
     */
    if(len + 16 >= 0)
    {
        len += 16;

        xmm_t0 = _mm_load_si128((__m128i *)data);
        xmm_t1 = _mm_load_si128((__m128i *)data + 1);
        xmm_t2 = _mm_load_si128((__m128i *)data + 2);

        XOR_INITIAL(xmm_t0);

        fold_3(&xmm_crc0, &xmm_crc1, &xmm_crc2, &xmm_crc3);

        xmm_crc1 = _mm_xor_si128(xmm_crc1, xmm_t0);
        xmm_crc2 = _mm_xor_si128(xmm_crc2, xmm_t1);
        xmm_crc3 = _mm_xor_si128(xmm_crc3, xmm_t2);

        if(len == 0) goto done;

        xmm_crc_part = _mm_load_si128((__m128i *)data + 3);
    }
    else if(len + 32 >= 0)
    {
        len += 32;

        xmm_t0 = _mm_load_si128((__m128i *)data);
        xmm_t1 = _mm_load_si128((__m128i *)data + 1);

        XOR_INITIAL(xmm_t0);

        fold_2(&xmm_crc0, &xmm_crc1, &xmm_crc2, &xmm_crc3);

        xmm_crc2 = _mm_xor_si128(xmm_crc2, xmm_t0);
        xmm_crc3 = _mm_xor_si128(xmm_crc3, xmm_t1);

        if(len == 0) goto done;

        xmm_crc_part = _mm_load_si128((__m128i *)data + 2);
    }
    else if(len + 48 >= 0)
    {
        len += 48;

        xmm_t0 = _mm_load_si128((__m128i *)data);

        XOR_INITIAL(xmm_t0);

        fold_1(&xmm_crc0, &xmm_crc1, &xmm_crc2, &xmm_crc3);

        xmm_crc3 = _mm_xor_si128(xmm_crc3, xmm_t0);

        if(len == 0) goto done;

        xmm_crc_part = _mm_load_si128((__m128i *)data + 1);
    }
    else
    {
        len += 64;
        if(len == 0) goto done;
        xmm_crc_part = _mm_load_si128((__m128i *)data);
        XOR_INITIAL(xmm_crc_part);
    }

partial:
    partial_fold(len, &xmm_crc0, &xmm_crc1, &xmm_crc2, &xmm_crc3, &xmm_crc_part);

done:
    (void)0;

    /* fold 512 to 32 */

    /*
     * k1
     */
    crc_fold = _mm_load_si128((__m128i *)crc_k);

    x_tmp0   = _mm_clmulepi64_si128(xmm_crc0, crc_fold, 0x10);
    xmm_crc0 = _mm_clmulepi64_si128(xmm_crc0, crc_fold, 0x01);
    xmm_crc1 = _mm_xor_si128(xmm_crc1, x_tmp0);
    xmm_crc1 = _mm_xor_si128(xmm_crc1, xmm_crc0);

    x_tmp1   = _mm_clmulepi64_si128(xmm_crc1, crc_fold, 0x10);
    xmm_crc1 = _mm_clmulepi64_si128(xmm_crc1, crc_fold, 0x01);
    xmm_crc2 = _mm_xor_si128(xmm_crc2, x_tmp1);
    xmm_crc2 = _mm_xor_si128(xmm_crc2, xmm_crc1);

    x_tmp2   = _mm_clmulepi64_si128(xmm_crc2, crc_fold, 0x10);
    xmm_crc2 = _mm_clmulepi64_si128(xmm_crc2, crc_fold, 0x01);
    xmm_crc3 = _mm_xor_si128(xmm_crc3, x_tmp2);
    xmm_crc3 = _mm_xor_si128(xmm_crc3, xmm_crc2);

    /*
     * k5
     */
    crc_fold = _mm_load_si128((__m128i *)crc_k + 1);

    xmm_crc0 = xmm_crc3;
    xmm_crc3 = _mm_clmulepi64_si128(xmm_crc3, crc_fold, 0);
    xmm_crc0 = _mm_srli_si128(xmm_crc0, 8);
    xmm_crc3 = _mm_xor_si128(xmm_crc3, xmm_crc0);

    xmm_crc0 = xmm_crc3;
    xmm_crc3 = _mm_slli_si128(xmm_crc3, 4);
    xmm_crc3 = _mm_clmulepi64_si128(xmm_crc3, crc_fold, 0x10);
    xmm_crc3 = _mm_xor_si128(xmm_crc3, xmm_crc0);
    xmm_crc3 = _mm_and_si128(xmm_crc3, xmm_mask2);

    /*
     * k7
     */
    xmm_crc1 = xmm_crc3;
    xmm_crc2 = xmm_crc3;
    crc_fold = _mm_load_si128((__m128i *)crc_k + 2);

    xmm_crc3 = _mm_clmulepi64_si128(xmm_crc3, crc_fold, 0);
    xmm_crc3 = _mm_xor_si128(xmm_crc3, xmm_crc2);
    xmm_crc3 = _mm_and_si128(xmm_crc3, xmm_mask);

    xmm_crc2 = xmm_crc3;
    xmm_crc3 = _mm_clmulepi64_si128(xmm_crc3, crc_fold, 0x10);
    xmm_crc3 = _mm_xor_si128(xmm_crc3, xmm_crc2);
    xmm_crc3 = _mm_xor_si128(xmm_crc3, xmm_crc1);

    /*
     * could just as well write xmm_crc3[2], doing a movaps and truncating, but
     * no real advantage - it's a tiny bit slower per call, while no additional CPUs
     * would be supported by only requiring SSSE3 and CLMUL instead of SSE4.1 + CLMUL
     */
    crc = _mm_extract_epi32(xmm_crc3, 2);
    return ~crc;
}

#endif