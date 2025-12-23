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
#include "crc32.h"

/**
 * @brief Initializes the CRC-32 checksum algorithm with the ISO polynomial.
 *
 * This function initializes the state variables required for the CRC-32
 * checksum algorithm using the ISO polynomial. It prepares the algorithm
 * to calculate the checksum for a new data set.
 *
 * @return Pointer to a structure containing the checksum state.
 */
AARU_EXPORT crc32_ctx *AARU_CALL crc32_init(void)
{
    crc32_ctx *ctx = (crc32_ctx *)malloc(sizeof(crc32_ctx));

    if(!ctx) return NULL;

    ctx->crc = CRC32_ISO_SEED;

    return ctx;
}

/**
 * @brief Updates the CRC-32 checksum with new data.
 *
 * This function updates the CRC-32 checksum.
 * The checksum is updated for the given data by using the ISO polynomial.
 * The algorithm continues the checksum calculation from the previous state,
 * so it can be used to update the checksum with new data as it is read.
 *
 * @param ctx Pointer to the CRC-32 context structure.
 * @param data Pointer to the input data buffer.
 * @param len The length of the input data buffer.
 *
 * @returns 0 on success, -1 on error.
 */
AARU_EXPORT int AARU_CALL crc32_update(crc32_ctx *ctx, const uint8_t *data, uint32_t len)
{
    if(!ctx || !data) return -1;

#if defined(__x86_64__) || defined(__amd64) || defined(_M_AMD64) || defined(_M_X64) || defined(__I386__) || \
    defined(__i386__) || defined(__THW_INTEL) || defined(_M_IX86)
    if(have_clmul())
    {
        ctx->crc = ~crc32_clmul(~ctx->crc, data, (long)len);

        return 0;
    }
#endif

#if defined(__aarch64__) || defined(_M_ARM64) || defined(__arm__) || defined(_M_ARM)
#if __ARM_ARCH >= 7
    if(have_arm_crc32())
    {
        ctx->crc = armv8_crc32_little(ctx->crc, data, len);

        return 0;
    }
#endif
    if(have_neon())
    {
        ctx->crc = ~crc32_vmull(~ctx->crc, data, len);
        return 0;
    }
#endif

    crc32_slicing(&ctx->crc, data, len);
    return 0;
}

/**
 * @brief Computes the CRC-32 checksum using slicing-by-8 algorithm.
 *
 * This function calculates the CRC-32 value for the given data using slicing-by-8 algorithm.
 *
 * @param previous_crc A pointer to the previous CRC-32 value, and where the updated value gets stored.
 * @param data The pointer to the data buffer.
 * @param len The length of the data in bytes.
 *
 * @note This function assumes little-endian byte order.
 */
AARU_EXPORT void AARU_CALL crc32_slicing(uint32_t *previous_crc, const uint8_t *data, long len)
{
    // Unroll according to Intel slicing by uint8_t
    // http://www.intel.com/technology/comms/perfnet/download/CRC_generators.pdf
    // http://sourceforge.net/projects/slicing-by-8/
    uint32_t        c;
    const uint32_t *current;
    const uint8_t  *current_char     = data;
    const size_t    unroll           = 4;
    const size_t    bytes_at_once    = 8 * unroll;
    uintptr_t       unaligned_length = (4 - (((uintptr_t)current_char) & 3)) & 3;

    c = *previous_crc;

    while((len != 0) && (unaligned_length != 0))
    {
        c = (c >> 8) ^ crc32_table[0][(c & 0xFF) ^ *current_char++];
        len--;
        unaligned_length--;
    }

    current = (const uint32_t *)current_char;

    while(len >= bytes_at_once)
    {
        size_t unrolling;
        for(unrolling = 0; unrolling < unroll; unrolling++)
        {
            uint32_t one = *current++ ^ c;
            uint32_t two = *current++;
            // TODO: Big endian!
            c            = crc32_table[0][(two >> 24) & 0xFF] ^ crc32_table[1][(two >> 16) & 0xFF] ^
                crc32_table[2][(two >> 8) & 0xFF] ^ crc32_table[3][two & 0xFF] ^ crc32_table[4][(one >> 24) & 0xFF] ^
                crc32_table[5][(one >> 16) & 0xFF] ^ crc32_table[6][(one >> 8) & 0xFF] ^ crc32_table[7][one & 0xFF];
        }

        len -= bytes_at_once;
    }

    current_char = (const uint8_t *)current;

    while(len-- != 0) c = (c >> 8) ^ crc32_table[0][(c & 0xFF) ^ *current_char++];

    *previous_crc = c;
}

/**
 * @brief Finalizes the calculation of the CRC-32 checksum.
 *
 * This function finalizes the calculation of the CRC-32 checksum and returns
 * its value.
 *
 * @param[in] ctx Pointer to the CRC-32 context structure.
 * @param[out] checksum Pointer to a 32-bit unsigned integer to store the checksum value.
 *
 * @returns 0 on success, -1 on error.
 */
AARU_EXPORT int AARU_CALL crc32_final(crc32_ctx *ctx, uint32_t *crc)
{
    if(!ctx) return -1;

    *crc = ctx->crc ^ CRC32_ISO_SEED;

    return 0;
}

/**
 * @brief Frees the resources allocated for the CRC-32 checksum context.
 *
 * This function should be called to release the memory used by the CRC-32 checksum
 * context structure after it is no longer needed.
 *
 * @param ctx The CRC-32 checksum context structure, to be freed.
 */
AARU_EXPORT void AARU_CALL crc32_free(crc32_ctx *ctx)
{
    if(ctx) free(ctx);
}