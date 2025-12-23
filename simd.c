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

#include <stddef.h>
#include <stdint.h>

#include "library.h"
#include "simd.h"

#if defined(__x86_64__) || defined(__amd64) || defined(_M_AMD64) || defined(_M_X64) || defined(__I386__) || \
    defined(__i386__) || defined(__THW_INTEL) || defined(_M_IX86)

#ifdef _MSC_VER
#include <intrin.h>
#else
/*
 * Newer versions of GCC and clang come with cpuid.h
 * (ftr GCC 4.7 in Debian Wheezy has this)
 */
#include <cpuid.h>

#endif

/**
 * @brief Gets the CPUID information for the given info value.
 *
 * This function retrieves the CPUID information for the specified info argument
 * and stores the results in the provided pointers: eax, ebx, ecx, and edx.
 * Each register represents a 32-bit value returned by the CPUID instruction.
 *
 * @param info The CPUID info value specifying the desired information to retrieve.
 * @param eax Pointer to store the value of the EAX register.
 * @param ebx Pointer to store the value of the EBX register.
 * @param ecx Pointer to store the value of the ECX register.
 * @param edx Pointer to store the value of the EDX register.
 *
 * @note It is important to ensure that the provided pointers are valid and point
 * to a memory location that can be modified by this function.
 *
 * @see https://en.wikipedia.org/wiki/CPUID
 *
 * @return None.
 */
static void cpuid(int info, unsigned *eax, unsigned *ebx, unsigned *ecx, unsigned *edx)
{
#ifdef _MSC_VER
    unsigned int registers[4];
    __cpuid(registers, info);
    *eax = registers[0];
    *ebx = registers[1];
    *ecx = registers[2];
    *edx = registers[3];
#else
    /* GCC, clang */
    unsigned int _eax;
    unsigned int _ebx;
    unsigned int _ecx;
    unsigned int _edx;
    __cpuid(info, _eax, _ebx, _ecx, _edx);
    *eax = _eax;
    *ebx = _ebx;
    *ecx = _ecx;
    *edx = _edx;
#endif
}

/**
 * @brief Get the CPU extended information using CPUID instruction.
 *
 * This function retrieves the extended information from the CPU by using the CPUID instruction.
 * It reads the result into the output parameters eax, ebx, ecx, and edx based on the input parameters info and count.
 *
 * @param info The CPUID function number to be executed.
 * @param count The sub-leaf index for certain CPUID functions.
 * @param eax Pointer to store the value of the EAX register.
 * @param ebx Pointer to store the value of the EBX register.
 * @param ecx Pointer to store the value of the ECX register.
 * @param edx Pointer to store the value of the EDX register.
 *
 * @note It is important to ensure that the provided pointers are valid and point
 * to a memory location that can be modified by this function.
 *
 * @see https://en.wikipedia.org/wiki/CPUID
 *
 * @return None.
 */
static void cpuidex(int info, int count, unsigned *eax, unsigned *ebx, unsigned *ecx, unsigned *edx)
{
#ifdef _MSC_VER
    unsigned int registers[4];
    __cpuidex(registers, info, count);
    *eax = registers[0];
    *ebx = registers[1];
    *ecx = registers[2];
    *edx = registers[3];
#else
    /* GCC, clang */
    unsigned int _eax;
    unsigned int _ebx;
    unsigned int _ecx;
    unsigned int _edx;
    __cpuid_count(info, count, _eax, _ebx, _ecx, _edx);
    *eax = _eax;
    *ebx = _ebx;
    *ecx = _ecx;
    *edx = _edx;
#endif
}

/**
 * @brief Checks if the hardware supports the CLMUL instruction set.
 *
 * The function checks if the system's CPU supports the CLMUL (Carry-Less Multiplication) instruction set.
 * CLMUL is an extension to the x86 instruction set architecture and provides hardware acceleration for
 * carry-less multiplication operations.
 *
 * @return True if CLMUL instruction set is supported, False otherwise.
 *
 * @see https://software.intel.com/sites/landingpage/IntrinsicsGuide/#techs=CLMUL
 * @see https://en.wikipedia.org/wiki/Carry-less_multiplication
 */
int have_clmul(void)
{
    unsigned eax, ebx, ecx, edx;
    int      has_pclmulqdq;
    int      has_sse41;
    cpuid(1 /* feature bits */, &eax, &ebx, &ecx, &edx);

    has_pclmulqdq = ecx & 0x2;     /* bit 1 */
    has_sse41     = ecx & 0x80000; /* bit 19 */

    return has_pclmulqdq && has_sse41;
}

/**
 * @brief Checks if the current processor supports SSSE3 instructions.
 *
 * The function detects whether the current processor supports SSSE3 instructions by
 * checking the CPU feature flags. SSSE3 (Supplemental Streaming SIMD Extensions 3)
 * is an extension to the x86 instruction set architecture that introduces
 * additional SIMD instructions useful for multimedia and signal processing tasks.
 *
 * @return true if the current processor supports SSSE3 instructions, false otherwise.
 *
 * @see https://software.intel.com/sites/landingpage/IntrinsicsGuide/#techs=SSSE3
 * @see https://en.wikipedia.org/wiki/SSSE3
 */
int have_ssse3(void)
{
    unsigned eax, ebx, ecx, edx;
    cpuid(1 /* feature bits */, &eax, &ebx, &ecx, &edx);

    return ecx & 0x200;
}

/**
 * @brief Checks if the current processor supports AVX2 instructions.
 *
 * The function detects whether the current processor supports AVX2 instructions by
 * checking the CPU feature flags. AVX2 (Advanced Vector Extensions 2) is an extension
 * to the x86 instruction set architecture that introduces additional SIMD instructions
 * useful for multimedia and signal processing tasks.
 *
 * @return true if the current processor supports AVX2 instructions, false otherwise.
 *
 * @see https://software.intel.com/sites/landingpage/IntrinsicsGuide/#techs=AVX2
 * @see https://en.wikipedia.org/wiki/Advanced_Vector_Extensions
 */

int have_avx2(void)
{
    unsigned eax, ebx, ecx, edx;
    cpuidex(7 /* extended feature bits */, 0, &eax, &ebx, &ecx, &edx);

    return ebx & 0x20;
}
#endif

#if defined(__aarch64__) || defined(_M_ARM64) || defined(__arm__) || defined(_M_ARM)
#if defined(_WIN32)

#include <windows.h>

#include <processthreadsapi.h>

#elif defined(__APPLE__)

#include <sys/sysctl.h>

#else

#include <sys/auxv.h>

#endif
#endif

#if(defined(__aarch64__) || defined(_M_ARM64) || defined(__arm__) || defined(_M_ARM)) && defined(__APPLE__)

/**
 * @brief Checks if the current processor supports NEON instructions.
 *
 * The function detects whether the current processor supports NEON instructions by
 * checking the CPU feature flags. NEON is an extension to the ARM instruction set
 * architecture that introduces additional SIMD instructions useful for multimedia
 * and signal processing tasks.
 *
 * @return true if the current processor supports NEON instructions, false otherwise.
 *
 * @see https://developer.arm.com/architectures/instruction-sets/simd-isas/neon
 * @see https://en.wikipedia.org/wiki/ARM_architecture#Advanced_SIMD_(NEON)
 */
int have_neon_apple()
{
    int    value;
    size_t len = sizeof(int);
    int    ret = sysctlbyname("hw.optional.neon", &value, &len, NULL, 0);

    if(ret != 0) return 0;

    return value == 1;
}

/**
 * @brief Checks if the current processor supports CRC32 instructions.
 *
 * The function detects whether the current processor supports CRC32 instructions by
 * checking the CPU feature flags. CRC32 is an extension to the ARM instruction set
 * architecture that introduces additional instructions for calculating CRC32 checksums.
 *
 * @return true if the current processor supports CRC32 instructions, false otherwise.
 */
int have_crc32_apple()
{
    int    value;
    size_t len = sizeof(int);
    int    ret = sysctlbyname("hw.optional.armv8_crc32", &value, &len, NULL, 0);

    if(ret != 0) return 0;

    return value == 1;
}

/**
 * @brief Checks if the current processor supports cryptographic instructions.
 *
 * The function detects whether the current processor supports cryptographic instructions by
 * checking the CPU feature flags. Cryptographic instructions are an extension to the ARM instruction set
 * architecture that introduces additional instructions for cryptographic operations.
 *
 * @return true if the current processor supports cryptographic instructions, false otherwise.
 */
int have_crypto_apple() { return 0; }

#endif

#if defined(__aarch64__) || defined(_M_ARM64)

int have_neon(void)
{
    return 1;  // ARMv8-A made it mandatory
}

/**
 * @brief Checks if the current processor supports CRC32 instructions.
 *
 * The function detects whether the current processor supports CRC32 instructions by
 * checking the CPU feature flags. CRC32 is an extension to the ARM instruction set
 * architecture that introduces additional instructions for calculating CRC32 checksums.
 *
 * @return true if the current processor supports CRC32 instructions, false otherwise.
 */
int have_arm_crc32(void)
{
#if defined(_WIN32)
    return IsProcessorFeaturePresent(PF_ARM_V8_CRC32_INSTRUCTIONS_AVAILABLE) != 0;
#elif defined(__APPLE__)
    return have_crc32_apple();
#else
    return getauxval(AT_HWCAP) & HWCAP_CRC32;
#endif
}

/**
 * @brief Checks if the current processor supports cryptographic instructions.
 *
 * The function detects whether the current processor supports cryptographic instructions by
 * checking the CPU feature flags. Cryptographic instructions are an extension to the ARM instruction set
 * architecture that introduces additional instructions for cryptographic operations.
 *
 * @return true if the current processor supports cryptographic instructions, false otherwise.
 */
int have_arm_crypto(void)
{
#if defined(_WIN32)
    return IsProcessorFeaturePresent(PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE) != 0;
#elif defined(__APPLE__)
    return have_crypto_apple();
#else
    return getauxval(AT_HWCAP) & HWCAP_AES;
#endif
}

#endif

#if defined(__arm__) || defined(_M_ARM)

/**
 * @brief Checks if the current processor supports NEON instructions.
 *
 * The function detects whether the current processor supports NEON instructions by
 * checking the CPU feature flags. NEON is an extension to the ARM instruction set
 * architecture that introduces additional SIMD instructions useful for multimedia
 * and signal processing tasks.
 *
 * @return true if the current processor supports NEON instructions, false otherwise.
 *
 * @see https://developer.arm.com/architectures/instruction-sets/simd-isas/neon
 * @see https://en.wikipedia.org/wiki/ARM_architecture#Advanced_SIMD_(NEON)
 */
int have_neon(void)
{
#if defined(_WIN32)
    return IsProcessorFeaturePresent(PF_ARM_VFP_32_REGISTERS_AVAILABLE) != 0;
#elif defined(__APPLE__)
    return have_neon_apple();
#else
    return getauxval(AT_HWCAP) & HWCAP_NEON;
#endif
}

/**
 * @brief Checks if the current processor supports CRC32 instructions.
 *
 * The function detects whether the current processor supports CRC32 instructions by
 * checking the CPU feature flags. CRC32 is an extension to the ARM instruction set
 * architecture that introduces additional instructions for calculating CRC32 checksums.
 *
 * @return true if the current processor supports CRC32 instructions, false otherwise.
 */
int have_arm_crc32(void)
{
#if defined(_WIN32)
    return IsProcessorFeaturePresent(PF_ARM_V8_CRC32_INSTRUCTIONS_AVAILABLE) != 0;
#elif defined(__APPLE__)
    return have_crc32_apple();
#else

// Not defined in ARMv7 compilers, even if the CPU has the capability
#ifndef HWCAP2_CRC32
#define HWCAP2_CRC32 (1 << 4)
#endif

    return getauxval(AT_HWCAP2) & HWCAP2_CRC32;
#endif
}

/**
 * @brief Checks if the current processor supports cryptographic instructions.
 *
 * The function detects whether the current processor supports cryptographic instructions by
 * checking the CPU feature flags. Cryptographic instructions are an extension to the ARM instruction set
 * architecture that introduces additional instructions for cryptographic operations.
 *
 * @return true if the current processor supports cryptographic instructions, false otherwise.
 */
int have_arm_crypto(void)
{
#if defined(_WIN32)
    return IsProcessorFeaturePresent(PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE) != 0;
#elif defined(__APPLE__)
    return have_crypto_apple();
#else
    return getauxval(AT_HWCAP2) & HWCAP2_AES;
#endif
}

#endif
