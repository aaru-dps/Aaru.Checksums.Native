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

#if(defined(__aarch64__) || defined(_M_ARM64) || defined(__arm__) || defined(_M_ARM)) && __ARM_ARCH >= 7

#if !defined(__ARM_FEATURE_CRC32)
#define __ARM_FEATURE_CRC32 1
#endif

#include <arm_acle.h>

#include "library.h"
#include "crc32.h"

/**
 * @brief Calculates the CRC-32 using the ARMv8 instruction set in little endian mode.
 *
 * This function takes the previous CRC value, data and length as inputs and calculates
 * the new CRC-32 using the ARMv8 instruction set in little endian mode.
 *
 * @param previous_crc The previous CRC value.
 * @param data The input data to calculate the CRC over.
 * @param len The length of the input data.
 * @return The new CRC-32 value.
 */
TARGET_ARMV8_WITH_CRC uint32_t armv8_crc32_little(uint32_t previous_crc, const uint8_t *data, uint32_t len)
{
    uint32_t c = previous_crc;

#if defined(__aarch64__) || defined(_M_ARM64)
    while(len && ((uintptr_t)data & 7))
    {
        c = __crc32b(c, *data++);
        --len;
    }
    const uint64_t *buf8 = (const uint64_t *)data;
    while(len >= 64)
    {
        c = __crc32d(c, *buf8++);
        c = __crc32d(c, *buf8++);
        c = __crc32d(c, *buf8++);
        c = __crc32d(c, *buf8++);
        c = __crc32d(c, *buf8++);
        c = __crc32d(c, *buf8++);
        c = __crc32d(c, *buf8++);
        c = __crc32d(c, *buf8++);
        len -= 64;
    }
    while(len >= 8)
    {
        c = __crc32d(c, *buf8++);
        len -= 8;
    }

    data = (const uint8_t *)buf8;
#else  // AARCH64
    while(len && ((uintptr_t)data & 3))
    {
        c = __crc32b(c, *data++);
        --len;
    }
    const uint32_t *buf4 = (const uint32_t *)data;
    while(len >= 32)
    {
        c = __crc32w(c, *buf4++);
        c = __crc32w(c, *buf4++);
        c = __crc32w(c, *buf4++);
        c = __crc32w(c, *buf4++);
        c = __crc32w(c, *buf4++);
        c = __crc32w(c, *buf4++);
        c = __crc32w(c, *buf4++);
        c = __crc32w(c, *buf4++);
        len -= 32;
    }
    while(len >= 4)
    {
        c = __crc32w(c, *buf4++);
        len -= 4;
    }

    data = (const uint8_t *)buf4;
#endif

    while(len--) { c = __crc32b(c, *data++); }
    return c;
}

#endif
