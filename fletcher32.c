/*
 * This file is part of the Aaru Data Preservation Suite.
 * Copyright (c) 2019-2021 Natalia Portillo.
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
#include "fletcher32.h"

AARU_EXPORT fletcher32_ctx* AARU_CALL fletcher32_init()
{
    fletcher32_ctx* ctx;

    ctx = (fletcher32_ctx*)malloc(sizeof(fletcher32_ctx));

    if(!ctx) return NULL;

    ctx->sum1 = 0xFFFF;
    ctx->sum2 = 0xFFFF;

    return ctx;
}

AARU_EXPORT int AARU_CALL fletcher32_update(fletcher32_ctx* ctx, const uint8_t* data, uint32_t len)
{
    if(!ctx || !data) return -1;

    uint32_t sum1 = ctx->sum1;
    uint32_t sum2 = ctx->sum2;
    unsigned n;

    /* in case user likes doing a byte at a time, keep it fast */
    if(len == 1)
    {
        sum1 += data[0];
        if(sum1 >= FLETCHER32_MODULE) sum1 -= FLETCHER32_MODULE;
        sum2 += sum1;
        if(sum2 >= FLETCHER32_MODULE) sum2 -= FLETCHER32_MODULE;

        ctx->sum1 = sum1 & 0xFFFF;
        ctx->sum2 = sum2 & 0xFFFF;
        return 0;
    }

    /* in case short lengths are provided, keep it somewhat fast */
    if(len < 16)
    {
        while(len--)
        {
            sum1 += *data++;
            sum2 += sum1;
        }
        if(sum1 >= FLETCHER32_MODULE) sum1 -= FLETCHER32_MODULE;
        sum2 %= FLETCHER32_MODULE; /* only added so many FLETCHER32_MODULE's */
        ctx->sum1 = sum1 & 0xFFFF;
        ctx->sum2 = sum2 & 0xFFFF;
        return 0;
    }

    /* do length NMAX blocks -- requires just one modulo operation */
    while(len >= NMAX)
    {
        len -= NMAX;
        n = NMAX / 16; /* NMAX is divisible by 16 */
        do {
            sum1 += (data)[0];
            sum2 += sum1;
            sum1 += (data)[0 + 1];
            sum2 += sum1;
            sum1 += (data)[0 + 2];
            sum2 += sum1;
            sum1 += (data)[0 + 2 + 1];
            sum2 += sum1;
            sum1 += (data)[0 + 4];
            sum2 += sum1;
            sum1 += (data)[0 + 4 + 1];
            sum2 += sum1;
            sum1 += (data)[0 + 4 + 2];
            sum2 += sum1;
            sum1 += (data)[0 + 4 + 2 + 1];
            sum2 += sum1;
            sum1 += (data)[8];
            sum2 += sum1;
            sum1 += (data)[8 + 1];
            sum2 += sum1;
            sum1 += (data)[8 + 2];
            sum2 += sum1;
            sum1 += (data)[8 + 2 + 1];
            sum2 += sum1;
            sum1 += (data)[8 + 4];
            sum2 += sum1;
            sum1 += (data)[8 + 4 + 1];
            sum2 += sum1;
            sum1 += (data)[8 + 4 + 2];
            sum2 += sum1;
            sum1 += (data)[8 + 4 + 2 + 1];
            sum2 += sum1;

            /* 16 sums unrolled */
            data += 16;
        } while(--n);
        sum1 %= FLETCHER32_MODULE;
        sum2 %= FLETCHER32_MODULE;
    }

    /* do remaining bytes (less than NMAX, still just one modulo) */
    if(len)
    { /* avoid modulos if none remaining */
        while(len >= 16)
        {
            len -= 16;
            sum1 += (data)[0];
            sum2 += sum1;
            sum1 += (data)[0 + 1];
            sum2 += sum1;
            sum1 += (data)[0 + 2];
            sum2 += sum1;
            sum1 += (data)[0 + 2 + 1];
            sum2 += sum1;
            sum1 += (data)[0 + 4];
            sum2 += sum1;
            sum1 += (data)[0 + 4 + 1];
            sum2 += sum1;
            sum1 += (data)[0 + 4 + 2];
            sum2 += sum1;
            sum1 += (data)[0 + 4 + 2 + 1];
            sum2 += sum1;
            sum1 += (data)[8];
            sum2 += sum1;
            sum1 += (data)[8 + 1];
            sum2 += sum1;
            sum1 += (data)[8 + 2];
            sum2 += sum1;
            sum1 += (data)[8 + 2 + 1];
            sum2 += sum1;
            sum1 += (data)[8 + 4];
            sum2 += sum1;
            sum1 += (data)[8 + 4 + 1];
            sum2 += sum1;
            sum1 += (data)[8 + 4 + 2];
            sum2 += sum1;
            sum1 += (data)[8 + 4 + 2 + 1];
            sum2 += sum1;

            data += 16;
        }
        while(len--)
        {
            sum1 += *data++;
            sum2 += sum1;
        }
        sum1 %= FLETCHER32_MODULE;
        sum2 %= FLETCHER32_MODULE;
    }

    ctx->sum1 = sum1 & 0xFFFF;
    ctx->sum2 = sum2 & 0xFFFF;
    return 0;
}

AARU_EXPORT int AARU_CALL fletcher32_final(fletcher32_ctx* ctx, uint32_t* checksum)
{
    if(!ctx) return -1;

    *checksum = (ctx->sum2 << 16) | ctx->sum1;
    return 0;
}

AARU_EXPORT void AARU_CALL fletcher32_free(fletcher32_ctx* ctx)
{
    if(!ctx) return;

    free(ctx);
}