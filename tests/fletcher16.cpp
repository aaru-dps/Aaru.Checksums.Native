//
// Created by claunia on 5/10/21.
//

#include <climits>
#include <cstdint>
#include <cstring>

#include "../library.h"
#include "../fletcher16.h"
#include "gtest/gtest.h"

#define EXPECTED_FLETCHER16           0x3357
#define EXPECTED_FLETCHER16_1BYTE     0xAFAF
#define EXPECTED_FLETCHER16_5BYTES    0x1F16
#define EXPECTED_FLETCHER16_7BYTES    0x865B
#define EXPECTED_FLETCHER16_15BYTES   0x0282
#define EXPECTED_FLETCHER16_31BYTES   0xABB7
#define EXPECTED_FLETCHER16_63BYTES   0x1CA0
#define EXPECTED_FLETCHER16_2352BYTES 0x0AC5

static const uint8_t *buffer;
static const uint8_t *buffer_misaligned;

class fletcher16Fixture : public ::testing::Test
{
public:
    fletcher16Fixture()
    {
        // initialization;
        // can also be done in SetUp()
    }

protected:
    void SetUp()
    {
        char path[PATH_MAX];
        char filename[PATH_MAX];

        getcwd(path, PATH_MAX);
        snprintf(filename, PATH_MAX, "%s/data/random", path);

        FILE *file = fopen(filename, "rb");
        buffer     = (const uint8_t *)malloc(1048576);
        fread((void *)buffer, 1, 1048576, file);
        fclose(file);

        buffer_misaligned = (const uint8_t *)malloc(1048577);
        memcpy((void *)(buffer_misaligned + 1), buffer, 1048576);
    }

    void TearDown()
    {
        free((void *)buffer);
        free((void *)buffer_misaligned);
    }

    ~fletcher16Fixture()
    {
        // resources cleanup, no exceptions allowed
    }

    // shared user data
};

TEST_F(fletcher16Fixture, fletcher16_auto)
{
    fletcher16_ctx *ctx = fletcher16_init();
    uint16_t        fletcher;

    EXPECT_NE(ctx, nullptr);

    fletcher16_update(ctx, buffer, 1048576);
    fletcher16_final(ctx, &fletcher);

    EXPECT_EQ(fletcher, EXPECTED_FLETCHER16);
}

TEST_F(fletcher16Fixture, fletcher16_slicing)
{
    uint8_t sum1 = 0xFF;
    uint8_t sum2 = 0xFF;
    uint16_t fletcher;

    fletcher16_slicing(&sum1, &sum2, buffer, 1048576);

    fletcher = (uint16_t)((sum2 << 8) | sum1);

    EXPECT_EQ(fletcher, EXPECTED_FLETCHER16);
}

TEST_F(fletcher16Fixture, fletcher16_auto_misaligned)
{
    fletcher16_ctx *ctx = fletcher16_init();
    uint16_t        fletcher;

    EXPECT_NE(ctx, nullptr);

    fletcher16_update(ctx, buffer_misaligned + 1, 1048576);
    fletcher16_final(ctx, &fletcher);

    EXPECT_EQ(fletcher, EXPECTED_FLETCHER16);
}

TEST_F(fletcher16Fixture, fletcher16_slicing_misaligned)
{
    uint8_t sum1 = 0xFF;
    uint8_t sum2 = 0xFF;
    uint16_t fletcher;

    fletcher16_slicing(&sum1, &sum2, buffer_misaligned + 1, 1048576);

    fletcher = (uint16_t)((sum2 << 8) | sum1);

    EXPECT_EQ(fletcher, EXPECTED_FLETCHER16);
}

TEST_F(fletcher16Fixture, fletcher16_auto_1byte)
{
    fletcher16_ctx *ctx = fletcher16_init();
    uint16_t        fletcher;

    EXPECT_NE(ctx, nullptr);

    fletcher16_update(ctx, buffer, 1);
    fletcher16_final(ctx, &fletcher);

    EXPECT_EQ(fletcher, EXPECTED_FLETCHER16_1BYTE);
}

TEST_F(fletcher16Fixture, fletcher16_slicing_1byte)
{
    uint8_t sum1 = 0xFF;
    uint8_t sum2 = 0xFF;
    uint16_t fletcher;

    fletcher16_slicing(&sum1, &sum2, buffer, 1);

    fletcher = (uint16_t)((sum2 << 8) | sum1);

    EXPECT_EQ(fletcher, EXPECTED_FLETCHER16_1BYTE);
}

TEST_F(fletcher16Fixture, fletcher16_auto_5bytes)
{
    fletcher16_ctx *ctx = fletcher16_init();
    uint16_t        fletcher;

    EXPECT_NE(ctx, nullptr);

    fletcher16_update(ctx, buffer, 5);
    fletcher16_final(ctx, &fletcher);

    EXPECT_EQ(fletcher, EXPECTED_FLETCHER16_5BYTES);
}

TEST_F(fletcher16Fixture, fletcher16_slicing_5bytes)
{
    uint8_t sum1 = 0xFF;
    uint8_t sum2 = 0xFF;
    uint16_t fletcher;

    fletcher16_slicing(&sum1, &sum2, buffer, 5);

    fletcher = (uint16_t)((sum2 << 8) | sum1);

    EXPECT_EQ(fletcher, EXPECTED_FLETCHER16_5BYTES);
}

TEST_F(fletcher16Fixture, fletcher16_auto_7bytes)
{
    fletcher16_ctx *ctx = fletcher16_init();
    uint16_t        fletcher;

    EXPECT_NE(ctx, nullptr);

    fletcher16_update(ctx, buffer, 7);
    fletcher16_final(ctx, &fletcher);

    EXPECT_EQ(fletcher, EXPECTED_FLETCHER16_7BYTES);
}

TEST_F(fletcher16Fixture, fletcher16_slicing_7bytes)
{
    uint8_t sum1 = 0xFF;
    uint8_t sum2 = 0xFF;
    uint16_t fletcher;

    fletcher16_slicing(&sum1, &sum2, buffer, 7);

    fletcher = (uint16_t)((sum2 << 8) | sum1);

    EXPECT_EQ(fletcher, EXPECTED_FLETCHER16_7BYTES);
}

TEST_F(fletcher16Fixture, fletcher16_auto_15bytes)
{
    fletcher16_ctx *ctx = fletcher16_init();
    uint16_t        fletcher;

    EXPECT_NE(ctx, nullptr);

    fletcher16_update(ctx, buffer, 15);
    fletcher16_final(ctx, &fletcher);

    EXPECT_EQ(fletcher, EXPECTED_FLETCHER16_15BYTES);
}

TEST_F(fletcher16Fixture, fletcher16_slicing_15bytes)
{
    uint8_t sum1 = 0xFF;
    uint8_t sum2 = 0xFF;
    uint16_t fletcher;

    fletcher16_slicing(&sum1, &sum2, buffer, 15);

    fletcher = (uint16_t)((sum2 << 8) | sum1);

    EXPECT_EQ(fletcher, EXPECTED_FLETCHER16_15BYTES);
}

TEST_F(fletcher16Fixture, fletcher16_auto_31bytes)
{
    fletcher16_ctx *ctx = fletcher16_init();
    uint16_t        fletcher;

    EXPECT_NE(ctx, nullptr);

    fletcher16_update(ctx, buffer, 31);
    fletcher16_final(ctx, &fletcher);

    EXPECT_EQ(fletcher, EXPECTED_FLETCHER16_31BYTES);
}

TEST_F(fletcher16Fixture, fletcher16_slicing_31bytes)
{
    uint8_t sum1 = 0xFF;
    uint8_t sum2 = 0xFF;
    uint16_t fletcher;

    fletcher16_slicing(&sum1, &sum2, buffer, 31);

    fletcher = (uint16_t)((sum2 << 8) | sum1);

    EXPECT_EQ(fletcher, EXPECTED_FLETCHER16_31BYTES);
}

TEST_F(fletcher16Fixture, fletcher16_auto_63bytes)
{
    fletcher16_ctx *ctx = fletcher16_init();
    uint16_t        fletcher;

    EXPECT_NE(ctx, nullptr);

    fletcher16_update(ctx, buffer, 63);
    fletcher16_final(ctx, &fletcher);

    EXPECT_EQ(fletcher, EXPECTED_FLETCHER16_63BYTES);
}

TEST_F(fletcher16Fixture, fletcher16_slicing_63bytes)
{
    uint8_t sum1 = 0xFF;
    uint8_t sum2 = 0xFF;
    uint16_t fletcher;

    fletcher16_slicing(&sum1, &sum2, buffer, 63);

    fletcher = (uint16_t)((sum2 << 8) | sum1);

    EXPECT_EQ(fletcher, EXPECTED_FLETCHER16_63BYTES);
}

TEST_F(fletcher16Fixture, fletcher16_auto_2352bytes)
{
    fletcher16_ctx *ctx = fletcher16_init();
    uint16_t        fletcher;

    EXPECT_NE(ctx, nullptr);

    fletcher16_update(ctx, buffer, 2352);
    fletcher16_final(ctx, &fletcher);

    EXPECT_EQ(fletcher, EXPECTED_FLETCHER16_2352BYTES);
}

TEST_F(fletcher16Fixture, fletcher16_slicing_2352bytes)
{
    uint8_t sum1 = 0xFF;
    uint8_t sum2 = 0xFF;
    uint16_t fletcher;

    fletcher16_slicing(&sum1, &sum2, buffer, 2352);

    fletcher = (uint16_t)((sum2 << 8) | sum1);

    EXPECT_EQ(fletcher, EXPECTED_FLETCHER16_2352BYTES);
}

#if defined(__aarch64__) || defined(_M_ARM64) || ((defined(__arm__) || defined(_M_ARM)) && !defined(_WIN32))
TEST_F(fletcher16Fixture, fletcher16_neon)
{
    if(!have_neon()) return;

    uint8_t  sum1;
    uint8_t  sum2;
    uint32_t fletcher16;

    sum1 = 0xFF;
    sum2 = 0xFF;

    fletcher16_neon(&sum1, &sum2, buffer, 1048576);

    fletcher16 = (sum2 << 8) | sum1;

    EXPECT_EQ(fletcher16, EXPECTED_FLETCHER16);
}

TEST_F(fletcher16Fixture, fletcher16_neon_misaligned)
{
    if(!have_neon()) return;

    uint8_t  sum1;
    uint8_t  sum2;
    uint32_t fletcher16;

    sum1 = 0xFF;
    sum2 = 0xFF;

    fletcher16_neon(&sum1, &sum2, buffer_misaligned + 1, 1048576);

    fletcher16 = (sum2 << 8) | sum1;

    EXPECT_EQ(fletcher16, EXPECTED_FLETCHER16);
}

TEST_F(fletcher16Fixture, fletcher16_neon_1byte)
{
    if(!have_neon()) return;

    uint8_t  sum1;
    uint8_t  sum2;
    uint32_t fletcher16;

    sum1 = 0xFF;
    sum2 = 0xFF;

    fletcher16_neon(&sum1, &sum2, buffer, 1);

    fletcher16 = (sum2 << 8) | sum1;

    EXPECT_EQ(fletcher16, EXPECTED_FLETCHER16_1BYTE);
}

TEST_F(fletcher16Fixture, fletcher16_neon_7bytes)
{
    if(!have_neon()) return;

    uint8_t  sum1;
    uint8_t  sum2;
    uint32_t fletcher16;

    sum1 = 0xFF;
    sum2 = 0xFF;

    fletcher16_neon(&sum1, &sum2, buffer, 7);

    fletcher16 = (sum2 << 8) | sum1;

    EXPECT_EQ(fletcher16, EXPECTED_FLETCHER16_7BYTES);
}

TEST_F(fletcher16Fixture, fletcher16_neon_15bytes)
{
    if(!have_neon()) return;

    uint8_t  sum1;
    uint8_t  sum2;
    uint32_t fletcher16;

    sum1 = 0xFF;
    sum2 = 0xFF;

    fletcher16_neon(&sum1, &sum2, buffer, 15);

    fletcher16 = (sum2 << 8) | sum1;

    EXPECT_EQ(fletcher16, EXPECTED_FLETCHER16_15BYTES);
}

TEST_F(fletcher16Fixture, fletcher16_neon_31bytes)
{
    if(!have_neon()) return;

    uint8_t  sum1;
    uint8_t  sum2;
    uint32_t fletcher16;

    sum1 = 0xFF;
    sum2 = 0xFF;

    fletcher16_neon(&sum1, &sum2, buffer, 31);

    fletcher16 = (sum2 << 8) | sum1;

    EXPECT_EQ(fletcher16, EXPECTED_FLETCHER16_31BYTES);
}

TEST_F(fletcher16Fixture, fletcher16_neon_63bytes)
{
    if(!have_neon()) return;

    uint8_t  sum1;
    uint8_t  sum2;
    uint32_t fletcher16;

    sum1 = 0xFF;
    sum2 = 0xFF;

    fletcher16_neon(&sum1, &sum2, buffer, 63);

    fletcher16 = (sum2 << 8) | sum1;

    EXPECT_EQ(fletcher16, EXPECTED_FLETCHER16_63BYTES);
}

TEST_F(fletcher16Fixture, fletcher16_neon_2352bytes)
{
    if(!have_neon()) return;

    uint8_t  sum1;
    uint8_t  sum2;
    uint32_t fletcher16;

    sum1 = 0xFF;
    sum2 = 0xFF;

    fletcher16_neon(&sum1, &sum2, buffer, 2352);

    fletcher16 = (sum2 << 8) | sum1;

    EXPECT_EQ(fletcher16, EXPECTED_FLETCHER16_2352BYTES);
}
#endif

#if defined(__x86_64__) || defined(__amd64) || defined(_M_AMD64) || defined(_M_X64) || defined(__I386__) || \
    defined(__i386__) || defined(__THW_INTEL) || defined(_M_IX86)

TEST_F(fletcher16Fixture, fletcher16_avx2)
{
    if(!have_avx2()) return;

    uint8_t  sum1;
    uint8_t  sum2;
    uint32_t fletcher16;

    sum1 = 0xFF;
    sum2 = 0xFF;

    fletcher16_avx2(&sum1, &sum2, buffer, 1048576);

    fletcher16 = (sum2 << 8) | sum1;

    EXPECT_EQ(fletcher16, EXPECTED_FLETCHER16);
}

TEST_F(fletcher16Fixture, fletcher16_ssse3)
{
    if(!have_ssse3()) return;

    uint8_t  sum1;
    uint8_t  sum2;
    uint32_t fletcher16;

    sum1 = 0xFF;
    sum2 = 0xFF;

    fletcher16_ssse3(&sum1, &sum2, buffer, 1048576);

    fletcher16 = (sum2 << 8) | sum1;

    EXPECT_EQ(fletcher16, EXPECTED_FLETCHER16);
}

TEST_F(fletcher16Fixture, fletcher16_avx2_misaligned)
{
    if(!have_avx2()) return;

    uint8_t  sum1;
    uint8_t  sum2;
    uint32_t fletcher16;

    sum1 = 0xFF;
    sum2 = 0xFF;

    fletcher16_avx2(&sum1, &sum2, buffer_misaligned + 1, 1048576);

    fletcher16 = (sum2 << 8) | sum1;

    EXPECT_EQ(fletcher16, EXPECTED_FLETCHER16);
}

TEST_F(fletcher16Fixture, fletcher16_ssse3_misaligned)
{
    if(!have_ssse3()) return;

    uint8_t  sum1;
    uint8_t  sum2;
    uint32_t fletcher16;

    sum1 = 0xFF;
    sum2 = 0xFF;

    fletcher16_ssse3(&sum1, &sum2, buffer_misaligned + 1, 1048576);

    fletcher16 = (sum2 << 8) | sum1;

    EXPECT_EQ(fletcher16, EXPECTED_FLETCHER16);
}

TEST_F(fletcher16Fixture, fletcher16_avx2_1byte)
{
    if(!have_avx2()) return;

    uint8_t  sum1;
    uint8_t  sum2;
    uint32_t fletcher16;

    sum1 = 0xFF;
    sum2 = 0xFF;

    fletcher16_avx2(&sum1, &sum2, buffer, 1);

    fletcher16 = (sum2 << 8) | sum1;

    EXPECT_EQ(fletcher16, EXPECTED_FLETCHER16_1BYTE);
}

TEST_F(fletcher16Fixture, fletcher16_ssse3_1byte)
{
    if(!have_ssse3()) return;

    uint8_t  sum1;
    uint8_t  sum2;
    uint32_t fletcher16;

    sum1 = 0xFF;
    sum2 = 0xFF;

    fletcher16_ssse3(&sum1, &sum2, buffer, 1);

    fletcher16 = (sum2 << 8) | sum1;

    EXPECT_EQ(fletcher16, EXPECTED_FLETCHER16_1BYTE);
}

TEST_F(fletcher16Fixture, fletcher16_avx2_7bytes)
{
    if(!have_avx2()) return;

    uint8_t  sum1;
    uint8_t  sum2;
    uint32_t fletcher16;

    sum1 = 0xFF;
    sum2 = 0xFF;

    fletcher16_avx2(&sum1, &sum2, buffer, 7);

    fletcher16 = (sum2 << 8) | sum1;

    EXPECT_EQ(fletcher16, EXPECTED_FLETCHER16_7BYTES);
}

TEST_F(fletcher16Fixture, fletcher16_ssse3_7bytes)
{
    if(!have_ssse3()) return;

    uint8_t  sum1;
    uint8_t  sum2;
    uint32_t fletcher16;

    sum1 = 0xFF;
    sum2 = 0xFF;

    fletcher16_ssse3(&sum1, &sum2, buffer, 7);

    fletcher16 = (sum2 << 8) | sum1;

    EXPECT_EQ(fletcher16, EXPECTED_FLETCHER16_7BYTES);
}

TEST_F(fletcher16Fixture, fletcher16_avx2_15bytes)
{
    if(!have_avx2()) return;

    uint8_t  sum1;
    uint8_t  sum2;
    uint32_t fletcher16;

    sum1 = 0xFF;
    sum2 = 0xFF;

    fletcher16_avx2(&sum1, &sum2, buffer, 15);

    fletcher16 = (sum2 << 8) | sum1;

    EXPECT_EQ(fletcher16, EXPECTED_FLETCHER16_15BYTES);
}

TEST_F(fletcher16Fixture, fletcher16_ssse3_15bytes)
{
    if(!have_ssse3()) return;

    uint8_t  sum1;
    uint8_t  sum2;
    uint32_t fletcher16;

    sum1 = 0xFF;
    sum2 = 0xFF;

    fletcher16_ssse3(&sum1, &sum2, buffer, 15);

    fletcher16 = (sum2 << 8) | sum1;

    EXPECT_EQ(fletcher16, EXPECTED_FLETCHER16_15BYTES);
}

TEST_F(fletcher16Fixture, fletcher16_avx2_31bytes)
{
    if(!have_avx2()) return;

    uint8_t  sum1;
    uint8_t  sum2;
    uint32_t fletcher16;

    sum1 = 0xFF;
    sum2 = 0xFF;

    fletcher16_avx2(&sum1, &sum2, buffer, 31);

    fletcher16 = (sum2 << 8) | sum1;

    EXPECT_EQ(fletcher16, EXPECTED_FLETCHER16_31BYTES);
}

TEST_F(fletcher16Fixture, fletcher16_ssse3_31bytes)
{
    if(!have_ssse3()) return;

    uint8_t  sum1;
    uint8_t  sum2;
    uint32_t fletcher16;

    sum1 = 0xFF;
    sum2 = 0xFF;

    fletcher16_ssse3(&sum1, &sum2, buffer, 31);

    fletcher16 = (sum2 << 8) | sum1;

    EXPECT_EQ(fletcher16, EXPECTED_FLETCHER16_31BYTES);
}

TEST_F(fletcher16Fixture, fletcher16_avx2_63bytes)
{
    if(!have_avx2()) return;

    uint8_t  sum1;
    uint8_t  sum2;
    uint32_t fletcher16;

    sum1 = 0xFF;
    sum2 = 0xFF;

    fletcher16_avx2(&sum1, &sum2, buffer, 63);

    fletcher16 = (sum2 << 8) | sum1;

    EXPECT_EQ(fletcher16, EXPECTED_FLETCHER16_63BYTES);
}

TEST_F(fletcher16Fixture, fletcher16_ssse3_63bytes)
{
    if(!have_ssse3()) return;

    uint8_t  sum1;
    uint8_t  sum2;
    uint32_t fletcher16;

    sum1 = 0xFF;
    sum2 = 0xFF;

    fletcher16_ssse3(&sum1, &sum2, buffer, 63);

    fletcher16 = (sum2 << 8) | sum1;

    EXPECT_EQ(fletcher16, EXPECTED_FLETCHER16_63BYTES);
}

TEST_F(fletcher16Fixture, fletcher16_avx2_2352bytes)
{
    if(!have_avx2()) return;

    uint8_t  sum1;
    uint8_t  sum2;
    uint32_t fletcher16;

    sum1 = 0xFF;
    sum2 = 0xFF;

    fletcher16_avx2(&sum1, &sum2, buffer, 2352);

    fletcher16 = (sum2 << 8) | sum1;

    EXPECT_EQ(fletcher16, EXPECTED_FLETCHER16_2352BYTES);
}

TEST_F(fletcher16Fixture, fletcher16_ssse3_2352bytes)
{
    if(!have_ssse3()) return;

    uint8_t  sum1;
    uint8_t  sum2;
    uint32_t fletcher16;

    sum1 = 0xFF;
    sum2 = 0xFF;

    fletcher16_ssse3(&sum1, &sum2, buffer, 2352);

    fletcher16 = (sum2 << 8) | sum1;

    EXPECT_EQ(fletcher16, EXPECTED_FLETCHER16_2352BYTES);
}
#endif
