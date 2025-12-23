/*
 * This file is part of the Aaru Data Preservation Suite.
 * Copyright (c) 2019-2026 Natalia Portillo.
 * Copyright (C) 2002 Andrew Tridgell <tridge@samba.org>
 * Copyright (C) 2006 ManTech International Corporation
 * Copyright (C) 2013 Helmut Grohne <helmut@subdivi.de>
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

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "library.h"
#include "spamsum.h"

#if defined(__x86_64__) || defined(__amd64) || defined(_M_AMD64) || defined(_M_X64) || defined(__I386__) || \
defined(__i386__) || defined(__THW_INTEL) || defined(_M_IX86)
    #include <immintrin.h>
#endif

#if defined(_MSC_VER)
#define ALWAYS_INLINE __forceinline
#define LIKELY(x)     (x)
#define UNLIKELY(x)   (x)
#else
#define ALWAYS_INLINE __attribute__((always_inline)) inline
#define LIKELY(x)     __builtin_expect(!!(x), 1)
#define UNLIKELY(x)   __builtin_expect(!!(x), 0)
#endif

static ALWAYS_INLINE uint32_t fastmod3_u32(uint32_t x)
{
    // q = floor(x/3) via multiplicative inverse (2^33 / 3)
    uint32_t q = (uint32_t)(((uint64_t)x * 0xAAAAAAABu) >> 33);
    return x - 3u * q;
}

static uint8_t b64[] = {0x41,
                        0x42,
                        0x43,
                        0x44,
                        0x45,
                        0x46,
                        0x47,
                        0x48,
                        0x49,
                        0x4A,
                        0x4B,
                        0x4C,
                        0x4D,
                        0x4E,
                        0x4F,
                        0x50,
                        0x51,
                        0x52,
                        0x53,
                        0x54,
                        0x55,
                        0x56,
                        0x57,
                        0x58,
                        0x59,
                        0x5A,
                        0x61,
                        0x62,
                        0x63,
                        0x64,
                        0x65,
                        0x66,
                        0x67,
                        0x68,
                        0x69,
                        0x6A,
                        0x6B,
                        0x6C,
                        0x6D,
                        0x6E,
                        0x6F,
                        0x70,
                        0x71,
                        0x72,
                        0x73,
                        0x74,
                        0x75,
                        0x76,
                        0x77,
                        0x78,
                        0x79,
                        0x7A,
                        0x30,
                        0x31,
                        0x32,
                        0x33,
                        0x34,
                        0x35,
                        0x36,
                        0x37,
                        0x38,
                        0x39,
                        0x2B,
                        0x2F};

/**
 * @brief Initializes the SpamSum checksum algorithm.
 *
 * This function initializes the state variables required for the SpamSum
 * checksum algorithm. It prepares the algorithm to calculate the checksum
 * for a new data set.
 *
 * @return Pointer to a structure containing the checksum state.
 */
AARU_EXPORT spamsum_ctx *AARU_CALL spamsum_init(void)
{
    spamsum_ctx *ctx = (spamsum_ctx *)malloc(sizeof(spamsum_ctx));
    if(!ctx) return NULL;

    memset(ctx, 0, sizeof(spamsum_ctx));

    ctx->bh_end       = 1;
    ctx->bh[0].h      = HASH_INIT;
    ctx->bh[0].half_h = HASH_INIT;

    return ctx;
}

/**
 * @brief Updates the SpamSum checksum with new data.
 *
 * This function updates the SpamSum checksum.
 *
 * @param ctx Pointer to the SpamSum context structure.
 * @param data Pointer to the input data buffer.
 * @param len The length of the input data buffer.
 *
 * @returns 0 on success, -1 on error.
 */
AARU_EXPORT int AARU_CALL spamsum_update(spamsum_ctx *ctx, const uint8_t *data, uint32_t len)
{
    if(!ctx || !data) return -1;

    const uint8_t *p = data;
    const uint8_t *e = data + len;

    // 4x unroll; falls through to remainder
    for(; p + 4 <= e; p += 4)
    {
        fuzzy_engine_step(ctx, p[0]);
        fuzzy_engine_step(ctx, p[1]);
        fuzzy_engine_step(ctx, p[2]);
        fuzzy_engine_step(ctx, p[3]);
    }
    while(p < e) { fuzzy_engine_step(ctx, *p++); }

    ctx->total_size += len;
    return 0;
}

/**
 * @brief Frees the resources allocated for the SpamSum checksum context.
 *
 * This function should be called to release the memory used by the SpamSum checksum
 * context structure after it is no longer needed.
 *
 * @param ctx The SpamSum checksum context structure, to be freed.
 */
AARU_EXPORT void AARU_CALL spamsum_free(spamsum_ctx *ctx) { if(ctx) free(ctx); }

#define ROLL_SUM(ctx)    ((ctx)->roll.h1 + (ctx)->roll.h2 + (ctx)->roll.h3)
#define SUM_HASH(c, h)   (((h) * HASH_PRIME) ^ (c));
#define SSDEEP_BS(index) (MIN_BLOCKSIZE << (index))

static inline void fuzzy_engine_step(spamsum_ctx *ctx, uint8_t c) {
    uint32_t i;

    // 1. Update rolling hash (scalar, unchanged)
    roll_hash(ctx, c);
    uint32_t h = ROLL_SUM(ctx);

    for ( i = ctx->bh_start; i < ctx->bh_end; ++i) {
        ctx->bh[i].h      = SUM_HASH(c, ctx->bh[i].h);
        ctx->bh[i].half_h = SUM_HASH(c, ctx->bh[i].half_h);
    }

    if (LIKELY(fastmod3_u32(h) != 2u)) return;

    i = ctx->bh_start;
    uint64_t mask = (i == 0) ? 0 : (((uint64_t)1u << i) - 1u);

    for (; i < ctx->bh_end; ++i) {
        if (UNLIKELY(((uint64_t)h & mask) != mask)) break;

        if (ctx->bh[i].d_len == 0)
            fuzzy_try_fork_blockhash(ctx);

        uint8_t ch = b64[ctx->bh[i].h % 64];
        ctx->bh[i].digest[ctx->bh[i].d_len] = ch;
        ctx->bh[i].half_digest = b64[ctx->bh[i].half_h % 64];

        if (ctx->bh[i].d_len < SPAMSUM_LENGTH - 1) {
            ctx->bh[i].digest[++ctx->bh[i].d_len] = 0;
            ctx->bh[i].h = HASH_INIT;

            if (ctx->bh[i].d_len < SPAMSUM_LENGTH / 2) {
                ctx->bh[i].half_h = HASH_INIT;
                ctx->bh[i].half_digest = 0;
            }
        } else {
            fuzzy_try_reduce_blockhash(ctx);
        }
        mask = (mask << 1) | 1u;
    }
}

ALWAYS_INLINE void roll_hash(spamsum_ctx *ctx, uint8_t c)
{
    // compute window index once
    uint32_t n   = ctx->roll.n;
    uint32_t idx = n % ROLLING_WINDOW;
    uint8_t  old = ctx->roll.window[idx];

    ctx->roll.h2 -= ctx->roll.h1;
    ctx->roll.h2 += (uint32_t)ROLLING_WINDOW * c;

    ctx->roll.h1 += c;
    ctx->roll.h1 -= old;

    ctx->roll.window[idx] = c;
    ctx->roll.n           = n + 1;

    ctx->roll.h3 = (ctx->roll.h3 << 5) ^ c;
}

FORCE_INLINE void fuzzy_try_reduce_blockhash(spamsum_ctx *ctx)
{
    // assert(ctx->bh_start < ctx->bh_end);

    if(ctx->bh_end - ctx->bh_start < 2) /* Need at least two working hashes. */
        return;

    if((uint64_t)SSDEEP_BS(ctx->bh_start) * SPAMSUM_LENGTH >= ctx->total_size)
        /* Initial blocksize estimate would select this or a smaller
         * blocksize. */
        return;

    if(ctx->bh[ctx->bh_start + 1].d_len < SPAMSUM_LENGTH / 2) /* Estimate adjustment would select this blocksize. */
        return;

    /* At this point we are clearly no longer interested in the
     * start_blocksize. Get rid of it. */
    ++ctx->bh_start;
}

FORCE_INLINE void fuzzy_try_fork_blockhash(spamsum_ctx *ctx)
{
    if(ctx->bh_end >= NUM_BLOCKHASHES) return;

    // assert(ctx->bh_end != 0);

    uint32_t obh             = ctx->bh_end - 1;
    uint32_t nbh             = ctx->bh_end;
    ctx->bh[nbh].h           = ctx->bh[obh].h;
    ctx->bh[nbh].half_h      = ctx->bh[obh].half_h;
    ctx->bh[nbh].digest[0]   = 0;
    ctx->bh[nbh].half_digest = 0;
    ctx->bh[nbh].d_len       = 0;
    ++ctx->bh_end;
}

static ALWAYS_INLINE char *u32toa(uint32_t v, char *buf_end)
{
    // write digits backwards, return new head
    do
    {
        *--buf_end = (char)('0' + (v % 10));
        v /= 10;
    } while(v);
    return buf_end;
}

/**
 * @brief Finalizes the calculation of the SpamSum checksum.
 *
 * This function finalizes the calculation of the SpamSum checksum and returns
 * its value.
 *
 * @param[in] ctx Pointer to the SpamSum context structure.
 * @param[out] result Pointer to a buffer to store the checksum value.
 *
 * @returns 0 on success, -1 on error.
 */
AARU_EXPORT int AARU_CALL spamsum_final(spamsum_ctx *ctx, uint8_t *result)
{
    uint32_t bi     = ctx->bh_start;
    uint32_t h      = ROLL_SUM(ctx);
    int      remain = (int)(FUZZY_MAX_RESULT - 1); /* Exclude terminating '\0'. */

    if(!result) return -1;

    /* Verify that our elimination was not overeager. */
    // assert(bi == 0 || (uint64_t)SSDEEP_BS(bi) / 2 * SPAMSUM_LENGTH < ctx->total_size);

    /* Initial blocksize guess. */
    while((uint64_t)SSDEEP_BS(bi) * SPAMSUM_LENGTH < ctx->total_size)
    {
        ++bi;

        if(bi >= NUM_BLOCKHASHES)
        {
            errno = EOVERFLOW;
            return -1;
        }
    }

    /* Adapt blocksize guess to actual digest length. */
    while(bi >= ctx->bh_end) --bi;

    while(bi > ctx->bh_start && ctx->bh[bi].d_len < SPAMSUM_LENGTH / 2) --bi;

    // assert(!(bi > 0 && ctx->bh[bi].d_len < SPAMSUM_LENGTH / 2));

    {
        uint32_t bs    = SSDEEP_BS(bi);
        char *   p     = (char *)result;
        char *   end   = p + remain;
        char *   start = u32toa(bs, end);
        size_t   n     = (size_t)(end - start);
        memmove(p, start, n);
        p[n] = ':'; // add colon
        p += n + 1;
        remain -= (int)(n + 1);
        result = (uint8_t *)p;
    }

    int i = (int)ctx->bh[bi].d_len;

    // assert(i <= remain);

    memcpy(result, ctx->bh[bi].digest, (size_t)i);
    result += i;
    remain -= i;

    if(h != 0)
    {
        // assert(remain > 0);

        *result = b64[ctx->bh[bi].h % 64];

        if(i < 3 || *result != result[-1] || *result != result[-2] || *result != result[-3])
        {
            ++result;
            --remain;
        }
    }
    else if(ctx->bh[bi].digest[i] != 0)
    {
        // assert(remain > 0);

        *result = ctx->bh[bi].digest[i];

        if(i < 3 || *result != result[-1] || *result != result[-2] || *result != result[-3])
        {
            ++result;
            --remain;
        }
    }

    // assert(remain > 0);

    *result++ = ':';
    --remain;

    if(bi < ctx->bh_end - 1)
    {
        ++bi;
        i = (int)ctx->bh[bi].d_len;

        memcpy(result, ctx->bh[bi].digest, (size_t)i);
        result += i;
        remain -= i;

        if(h != 0)
        {
            // assert(remain > 0);

            h       = ctx->bh[bi].half_h;
            *result = b64[h % 64];

            if(i < 3 || *result != result[-1] || *result != result[-2] || *result != result[-3])
            {
                ++result;
                --remain;
            }
        }
        else
        {
            i = ctx->bh[bi].half_digest;

            if(i != 0)
            {
                // assert(remain > 0);

                *result = (uint8_t)i;

                if(i < 3 || *result != result[-1] || *result != result[-2] || *result != result[-3])
                {
                    ++result;
                    --remain;
                }
            }
        }
    }
    else if(h != 0)
    {
        // assert(ctx->bh[bi].d_len == 0);

        // assert(remain > 0);

        *result++ = b64[ctx->bh[bi].h % 64];
        /* No need to bother with FUZZY_FLAG_ELIMSEQ, because this
         * digest has length 1. */
        --remain;
    }

    *result = 0;

    return 0;
}
