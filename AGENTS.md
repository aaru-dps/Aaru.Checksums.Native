# Agents Guide for Aaru.Checksums.Native

## Project Overview

This is a native C library providing checksum and hashing algorithms for the [Aaru Data Preservation Suite](https://www.aaru.app). The library is designed to be cross-compiled for multiple platforms and architectures using Docker and CMake.

## Repository Structure

```
├── *.c, *.h          # Core algorithm implementations
├── CMakeLists.txt    # CMake build configuration
├── build.sh          # Cross-compilation build script
├── tests/            # Google Test based unit tests
├── benchmarks/       # Performance benchmarks
├── docker/           # Dockcross Docker images
├── runtimes/         # Built native libraries per platform
└── lib/              # Third-party libraries
```

## Implemented Algorithms

- **Adler-32** (`adler32.c`, `adler32.h`) - with SSSE3, AVX2, NEON optimizations
- **CRC-16** (`crc16.c`, `crc16.h`) - IBM polynomial, with AVX2 and NEON optimizations
- **CRC-16 CCITT** (`crc16_ccitt.c`, `crc16_ccitt.h`) - CCITT polynomial, with CLMUL, PMULL, VMULL optimizations
- **CRC-32** (`crc32.c`, `crc32.h`) - ISO polynomial, with CLMUL and VMULL optimizations
- **CRC-64** (`crc64.c`, `crc64.h`) - ECMA polynomial, with CLMUL and VMULL optimizations
- **Fletcher-16** (`fletcher16.c`, `fletcher16.h`) - with SSSE3, AVX2, NEON optimizations
- **Fletcher-32** (`fletcher32.c`, `fletcher32.h`) - with SSSE3, AVX2, NEON optimizations
- **SpamSum** (`spamsum.c`, `spamsum.h`) - fuzzy hashing

## SIMD Support

SIMD implementations are conditionally compiled based on target architecture:
- **x86/x64**: SSE4.2, SSSE3, AVX2, CLMUL (PCLMULQDQ)
- **ARM/ARM64**: NEON, CRC32 instructions, PMULL/VMULL (crypto extensions)

Runtime detection is handled in `simd.c`/`simd.h`.

## Build System

### CMake Configuration

- Minimum version: 3.15
- C standard: C90 (C11 for MSVC ARM)
- Key options:
  - `AARU_BUILD_PACKAGE=1` - Package build mode (skips tests)
  - `CMAKE_BUILD_TYPE=Release` - Enables optimizations

### Building Locally

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

### Cross-Compilation (Full Build)

Run `build.sh` with Docker installed. This builds for all supported platforms:
- Android: ARM, ARM64, x86, x64
- Linux: ARM, ARM64, x64, x86, MIPS64, s390x, ppc64le, musl variants
- Windows: ARM, ARM64, x86, x64
- macOS: x64, ARM64

## Testing

Tests use Google Test framework. Run tests:

```bash
mkdir build && cd build
cmake ..
make
./tests/tests_run
```

Or using CTest:

```bash
ctest --test-dir build
```

## Code Style Guidelines

- C90 standard (except ARM MSVC which uses C11)
- Use `AARU_EXPORT` macro for public API functions
- Use `AARU_CALL` for calling convention (`__stdcall` on Windows)
- Use `FORCE_INLINE` for performance-critical inline functions
- SIMD functions should use appropriate `TARGET_WITH_*` attributes

## Public API Pattern

All exported functions follow this pattern:

```c
AARU_EXPORT return_type AARU_CALL function_name(parameters);
```

## Adding New Algorithms

1. Create implementation files (`algorithm.c`, `algorithm.h`)
2. Add SIMD-optimized variants if applicable (`algorithm_avx2.c`, etc.)
3. Add header guards following pattern: `AARU_CHECKSUMS_NATIVE_ALGORITHM_H`
4. Add source files to `CMakeLists.txt` library sources
5. Add unit tests in `tests/algorithm.cpp`
6. Ensure license compatibility with LGPL 2.1

## License

LGPL 2.1 - All new code must be compatible with this license.

## Version

Version is defined in `library.h` as `AARU_CHECKUMS_NATIVE_VERSION` and exposed via `get_acn_version()`.

