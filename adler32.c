/*
 * This file is part of the Aaru Data Preservation Suite.
 * Copyright (c) 2019-2026 Natalia Portillo.
 * Copyright (C) 1995-2011 Mark Adler
 * Copyright (C) Jean-loup Gailly
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 *  1. The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would be
 *     appreciated but is not required.
 *
 *  2. Altered source versions must be plainly marked as such, and must not be
 *     misrepresented as being the original software.
 *  3. This notice may not be removed or altered from any source distribution.
 */

#include <stdint.h>
#include <stdlib.h>

#include "library.h"
#include "adler32.h"
#include "simd.h"

/**
 * @brief Initializes the Adler-32 checksum algorithm.
 *
 * This function initializes the state variables required for the Adler-32
 * checksum algorithm. It prepares the algorithm to calculate the checksum
 * for a new data set.
 *
 * @return Pointer to a structure containing the checksum state.
 */
AARU_EXPORT adler32_ctx *AARU_CALL adler32_init()
{
    adler32_ctx *ctx;

    ctx = (adler32_ctx *)malloc(sizeof(adler32_ctx));

    if(!ctx) return NULL;

    ctx->sum1 = 1;
    ctx->sum2 = 0;

    return ctx;
}

/**
 * @brief Updates the Adler-32 checksum with new data.
 *
 * This function updates the Adler-32 checksum.
 * The checksum is updated for the given data by iterating through each byte and
 * applying the corresponding calculations to the rolling checksum values.
 *
 * @param ctx Pointer to the Adler-32 context structure.
 * @param data Pointer to the input data buffer.
 * @param len The length of the input data buffer.
 */
AARU_EXPORT int AARU_CALL adler32_update(adler32_ctx *ctx, const uint8_t *data, uint32_t len)
{
    if(!ctx || !data) return -1;

#if defined(__aarch64__) || defined(_M_ARM64) || ((defined(__arm__) || defined(_M_ARM)) && !defined(__MINGW32__))
    if(have_neon())
    {
        adler32_neon(&ctx->sum1, &ctx->sum2, data, len);

        return 0;
    }
#endif

#if defined(__x86_64__) || defined(__amd64) || defined(_M_AMD64) || defined(_M_X64) || defined(__I386__) || \
    defined(__i386__) || defined(__THW_INTEL) || defined(_M_IX86)
    if(have_avx2())
    {
        adler32_avx2(&ctx->sum1, &ctx->sum2, data, len);

        return 0;
    }

    if(have_ssse3())
    {
        adler32_ssse3(&ctx->sum1, &ctx->sum2, data, len);

        return 0;
    }
#endif

    adler32_slicing(&ctx->sum1, &ctx->sum2, data, len);

    return 0;
}

/**
 * @brief Calculates Adler-32 checksum for a given data using slicing algorithm.
 *
 * @param sum1 Pointer to a 16-bit unsigned integer to store the first sum value.
 * @param sum2 Pointer to a 16-bit unsigned integer to store the second sum value.
 * @param data Pointer to the data for which the checksum is to be calculated.
 * @param len The length of the data in bytes.
 */
AARU_EXPORT void AARU_CALL adler32_slicing(uint16_t *sum1, uint16_t *sum2, const uint8_t *data, long len)
{
    uint32_t s1 = *sum1;
    uint32_t s2 = *sum2;
    unsigned n;

    /* in case user likes doing a byte at a time, keep it fast */
    if(len == 1)
    {
        s1 += data[0];
        if(s1 >= ADLER_MODULE) s1 -= ADLER_MODULE;
        s2 += s1;
        if(s2 >= ADLER_MODULE) s2 -= ADLER_MODULE;

        *sum1 = s1 & 0xFFFF;
        *sum2 = s2 & 0xFFFF;

        return;
    }

    /* in case short lengths are provided, keep it somewhat fast */
    if(len < 16)
    {
        while(len--)
        {
            s1 += *data++;
            s2 += s1;
        }
        if(s1 >= ADLER_MODULE) s1 -= ADLER_MODULE;
        s2 %= ADLER_MODULE; /* only added so many ADLER_MODULE's */
        *sum1 = s1 & 0xFFFF;
        *sum2 = s2 & 0xFFFF;

        return;
    }

    /* do length NMAX blocks -- requires just one modulo operation */
    while(len >= NMAX)
    {
        len -= NMAX;
        n = NMAX / 16; /* NMAX is divisible by 16 */
        do {
            s1 += data[0];
            s2 += s1;
            s1 += data[0 + 1];
            s2 += s1;
            s1 += data[0 + 2];
            s2 += s1;
            s1 += data[0 + 2 + 1];
            s2 += s1;
            s1 += data[0 + 4];
            s2 += s1;
            s1 += data[0 + 4 + 1];
            s2 += s1;
            s1 += data[0 + 4 + 2];
            s2 += s1;
            s1 += data[0 + 4 + 2 + 1];
            s2 += s1;
            s1 += data[8];
            s2 += s1;
            s1 += data[8 + 1];
            s2 += s1;
            s1 += data[8 + 2];
            s2 += s1;
            s1 += data[8 + 2 + 1];
            s2 += s1;
            s1 += data[8 + 4];
            s2 += s1;
            s1 += data[8 + 4 + 1];
            s2 += s1;
            s1 += data[8 + 4 + 2];
            s2 += s1;
            s1 += data[8 + 4 + 2 + 1];
            s2 += s1;

            /* 16 sums unrolled */
            data += 16;
        } while(--n);
        s1 %= ADLER_MODULE;
        s2 %= ADLER_MODULE;
    }

    /* do remaining bytes (less than NMAX, still just one modulo) */
    if(len)
    { /* avoid modulos if none remaining */
        while(len >= 16)
        {
            len -= 16;
            s1 += data[0];
            s2 += s1;
            s1 += data[0 + 1];
            s2 += s1;
            s1 += data[0 + 2];
            s2 += s1;
            s1 += data[0 + 2 + 1];
            s2 += s1;
            s1 += data[0 + 4];
            s2 += s1;
            s1 += data[0 + 4 + 1];
            s2 += s1;
            s1 += data[0 + 4 + 2];
            s2 += s1;
            s1 += data[0 + 4 + 2 + 1];
            s2 += s1;
            s1 += data[8];
            s2 += s1;
            s1 += data[8 + 1];
            s2 += s1;
            s1 += data[8 + 2];
            s2 += s1;
            s1 += data[8 + 2 + 1];
            s2 += s1;
            s1 += data[8 + 4];
            s2 += s1;
            s1 += data[8 + 4 + 1];
            s2 += s1;
            s1 += data[8 + 4 + 2];
            s2 += s1;
            s1 += data[8 + 4 + 2 + 1];
            s2 += s1;

            data += 16;
        }
        while(len--)
        {
            s1 += *data++;
            s2 += s1;
        }
        s1 %= ADLER_MODULE;
        s2 %= ADLER_MODULE;
    }

    *sum1 = s1 & 0xFFFF;
    *sum2 = s2 & 0xFFFF;
}

/**
 * @brief Finalizes the calculation of the Adler-32 checksum.
 *
 * This function finalizes the calculation of the Adler-32 checksum and returns
 * its value.
 *
 * @param[in] ctx Pointer to the Adler-32 context structure.
 * @param[out] checksum Pointer to a 32-bit unsigned integer to store the checksum value.
 *
 * @returns 0 on success, -1 on error.
 */
AARU_EXPORT int AARU_CALL adler32_final(adler32_ctx *ctx, uint32_t *checksum)
{
    if(!ctx) return -1;

    *checksum = (ctx->sum2 << 16) | ctx->sum1;
    return 0;
}

/**
 * @brief Frees the resources allocated for the Adler-32 checksum context.
 *
 * This function should be called to release the memory used by the Adler-32 checksum
 * context structure after it is no longer needed.
 *
 * @param ctx The Adler-32 checksum context structure, to be freed.
 */
AARU_EXPORT void AARU_CALL adler32_free(adler32_ctx *ctx)
{
    if(!ctx) return;

    free(ctx);
}