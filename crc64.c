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

#include <stdint.h>
#include <stdlib.h>

#include "library.h"
#include "crc64.h"
#include "simd.h"

/**
 * @brief Initializes the CRC-64 checksum algorithm with the ECMA polynomial.
 *
 * This function initializes the state variables required for the CRC-ECMA
 * checksum algorithm using the IBM polynomial. It prepares the algorithm
 * to calculate the checksum for a new data set.
 *
 * @return Pointer to a structure containing the checksum state.
 */
AARU_EXPORT crc64_ctx *AARU_CALL crc64_init(void)
{
    crc64_ctx *ctx = (crc64_ctx *)malloc(sizeof(crc64_ctx));

    if(!ctx) return NULL;

    ctx->crc = CRC64_ECMA_SEED;

    return ctx;
}

/**
 * @brief Updates the CRC-64 checksum with new data.
 *
 * This function updates the CRC-64 checksum.
 * The checksum is updated for the given data by using the ECMA polynomial.
 * The algorithm continues the checksum calculation from the previous state,
 * so it can be used to update the checksum with new data as it is read.
 *
 * @param ctx Pointer to the CRC-64 context structure.
 * @param data Pointer to the input data buffer.
 * @param len The length of the input data buffer.
 *
 * @returns 0 on success, -1 on error.
 */
AARU_EXPORT int AARU_CALL crc64_update(crc64_ctx *ctx, const uint8_t *data, uint32_t len)
{
    if(!ctx || !data) return -1;

#if defined(__x86_64__) || defined(__amd64) || defined(_M_AMD64) || defined(_M_X64) || defined(__I386__) || \
    defined(__i386__) || defined(__THW_INTEL) || defined(_M_IX86)
    if(have_clmul())
    {
        ctx->crc = ~crc64_clmul(~ctx->crc, data, len);
        return 0;
    }
#endif

#if defined(__aarch64__) || defined(_M_ARM64) || defined(__arm__) || defined(_M_ARM)
    if(have_neon())
    {
        ctx->crc = ~crc64_vmull(~ctx->crc, data, len);
        return 0;
    }
#endif

    // Unroll according to Intel slicing by uint8_t
    // http://www.intel.com/technology/comms/perfnet/download/CRC_generators.pdf
    // http://sourceforge.net/projects/slicing-by-8/

    crc64_slicing(&ctx->crc, data, len);

    return 0;
}

AARU_EXPORT void AARU_CALL crc64_slicing(uint64_t *previous_crc, const uint8_t *data, uint32_t len)
{
    uint64_t c = *previous_crc;

    if(len > 4)
    {
        const uint8_t *limit;

        while((uintptr_t)(data) & 3)
        {
            c = crc64_table[0][*data++ ^ (c & 0xFF)] ^ (c >> 8);
            --len;
        }

        limit = data + (len & ~(uint32_t)(3));
        len &= (uint32_t)(3);

        while(data < limit)
        {
            const uint32_t tmp = c ^ *(const uint32_t *)(data);
            data += 4;

            c = crc64_table[3][tmp & 0xFF] ^ crc64_table[2][(tmp >> 8) & 0xFF] ^ (c >> 32) ^
                crc64_table[1][tmp >> 16 & 0xFF] ^ crc64_table[0][tmp >> 24];
        }
    }

    while(len-- != 0) c = crc64_table[0][*data++ ^ (c & 0xFF)] ^ (c >> 8);

    *previous_crc = c;
}

/**
 * @brief Finalizes the calculation of the CRC-64 checksum.
 *
 * This function finalizes the calculation of the CRC-64 checksum and returns
 * its value.
 *
 * @param[in] ctx Pointer to the CRC-64 context structure.
 * @param[out] checksum Pointer to a 64-bit unsigned integer to store the checksum value.
 *
 * @returns 0 on success, -1 on error.
 */
AARU_EXPORT int AARU_CALL crc64_final(crc64_ctx *ctx, uint64_t *crc)
{
    if(!ctx) return -1;

    *crc = ctx->crc ^ CRC64_ECMA_SEED;

    return 0;
}

/**
 * @brief Frees the resources allocated for the CRC-64 checksum context.
 *
 * This function should be called to release the memory used by the CRC-64 checksum
 * context structure after it is no longer needed.
 *
 * @param ctx The CRC-64 checksum context structure, to be freed.
 */
AARU_EXPORT void AARU_CALL crc64_free(crc64_ctx *ctx)
{
    if(ctx) free(ctx);
}