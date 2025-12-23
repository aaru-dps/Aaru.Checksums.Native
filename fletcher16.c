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
#include "fletcher16.h"

/**
 * @brief Initializes the Fletcher-16 checksum algorithm.
 *
 * This function initializes the state variables required for the Fletcher-16
 * checksum algorithm. It prepares the algorithm to calculate the checksum
 * for a new data set.
 *
 * @return Pointer to a structure containing the checksum state.
 */
AARU_EXPORT fletcher16_ctx *AARU_CALL fletcher16_init()
{
    fletcher16_ctx *ctx;

    ctx = (fletcher16_ctx *)malloc(sizeof(fletcher16_ctx));

    if(!ctx) return NULL;

    ctx->sum1 = 0xFF;
    ctx->sum2 = 0xFF;

    return ctx;
}

/**
 * @brief Updates the Fletcher-16 checksum with new data.
 *
 * This function updates the Fletcher-16 checksum.
 * The checksum is updated for the given data by iterating through each byte and
 * applying the corresponding calculations to the rolling checksum values.
 *
 * @param ctx Pointer to the Fletcher-16 context structure.
 * @param data Pointer to the input data buffer.
 * @param len The length of the input data buffer.
 */
AARU_EXPORT int AARU_CALL fletcher16_update(fletcher16_ctx *ctx, const uint8_t *data, uint32_t len)
{
    if(!ctx || !data) return -1;

#if defined(__aarch64__) || defined(_M_ARM64) || ((defined(__arm__) || defined(_M_ARM)) && !defined(__MINGW32__))
    if(have_neon())
    {
        fletcher16_neon(&ctx->sum1, &ctx->sum2, data, len);

        return 0;
    }
#endif

#if defined(__x86_64__) || defined(__amd64) || defined(_M_AMD64) || defined(_M_X64) || defined(__I386__) || \
    defined(__i386__) || defined(__THW_INTEL) || defined(_M_IX86)
    if(have_avx2())
    {
        fletcher16_avx2(&ctx->sum1, &ctx->sum2, data, len);

        return 0;
    }

    if(have_ssse3())
    {
        fletcher16_ssse3(&ctx->sum1, &ctx->sum2, data, len);

        return 0;
    }
#endif

    uint32_t sum1 = ctx->sum1;
    uint32_t sum2 = ctx->sum2;
    unsigned n;

    /* in case user likes doing a byte at a time, keep it fast */
    if(len == 1)
    {
        sum1 += data[0];
        if(sum1 >= FLETCHER16_MODULE) sum1 -= FLETCHER16_MODULE;
        sum2 += sum1;
        if(sum2 >= FLETCHER16_MODULE) sum2 -= FLETCHER16_MODULE;

        ctx->sum1 = sum1 & 0xFF;
        ctx->sum2 = sum2 & 0xFF;
        return 0;
    }

    /* in case short lengths are provided, keep it somewhat fast */
    if(len < 6)
    {
        while(len--)
        {
            sum1 += *data++;
            sum2 += sum1;
        }
        sum1 %= FLETCHER16_MODULE;
        sum2 %= FLETCHER16_MODULE; /* only added so many FLETCHER16_MODULE's */
        ctx->sum1 = sum1 & 0xFF;
        ctx->sum2 = sum2 & 0xFF;
        return 0;
    }

    /* do length NMAX blocks -- requires just one modulo operation */
    while(len >= NMAX)
    {
        len -= NMAX;
        n = NMAX / 6; /* NMAX is divisible by 6 */
        do {
            sum1 += data[0];
            sum2 += sum1;
            sum1 += data[0 + 1];
            sum2 += sum1;
            sum1 += data[0 + 2];
            sum2 += sum1;
            sum1 += data[0 + 2 + 1];
            sum2 += sum1;
            sum1 += data[0 + 4];
            sum2 += sum1;
            sum1 += data[0 + 4 + 1];
            sum2 += sum1;

            /* 6 sums unrolled */
            data += 6;
        } while(--n);
        sum1 %= FLETCHER16_MODULE;
        sum2 %= FLETCHER16_MODULE;
    }

    /* do remaining bytes (less than NMAX, still just one modulo) */
    if(len)
    { /* avoid modulos if none remaining */
        while(len >= 6)
        {
            len -= 6;
            sum1 += data[0];
            sum2 += sum1;
            sum1 += data[0 + 1];
            sum2 += sum1;
            sum1 += data[0 + 2];
            sum2 += sum1;
            sum1 += data[0 + 2 + 1];
            sum2 += sum1;
            sum1 += data[0 + 4];
            sum2 += sum1;
            sum1 += data[0 + 4 + 1];
            sum2 += sum1;

            data += 6;
        }
        while(len--)
        {
            sum1 += *data++;
            sum2 += sum1;
        }
        sum1 %= FLETCHER16_MODULE;
        sum2 %= FLETCHER16_MODULE;
    }

    ctx->sum1 = sum1 & 0xFF;
    ctx->sum2 = sum2 & 0xFF;
    return 0;
}

/**
 * @brief Finalizes the calculation of the Fletcher-16 checksum.
 *
 * This function finalizes the calculation of the Fletcher-16 checksum and returns
 * its value.
 *
 * @param[in] ctx Pointer to the Fletcher-32 context structure.
 * @param[out] checksum Pointer to a 16-bit unsigned integer to store the checksum value.
 *
 * @returns 0 on success, -1 on error.
 */
AARU_EXPORT int AARU_CALL fletcher16_final(fletcher16_ctx *ctx, uint16_t *checksum)
{
    if(!ctx) return -1;

    *checksum = (ctx->sum2 << 8) | ctx->sum1;
    return 0;
}

/**
 * @brief Frees the resources allocated for the Fletcher-16 checksum context.
 *
 * This function should be called to release the memory used by the Fletcher-16 checksum
 * context structure after it is no longer needed.
 *
 * @param ctx The Fletcher-16 checksum context structure, to be freed.
 */
AARU_EXPORT void AARU_CALL fletcher16_free(fletcher16_ctx *ctx)
{
    if(!ctx) return;

    free(ctx);
}