//
// Created by claunia on 5/10/21.
//

#include <climits>
#include <cstdint>
#include <cstring>

#include "../library.h"
#include "../crc16.h"
#include "../simd.h"
#include "gtest/gtest.h"

#define EXPECTED_CRC16           0x2d6d
#define EXPECTED_CRC16_15BYTES   0x72a6
#define EXPECTED_CRC16_31BYTES   0xF49E
#define EXPECTED_CRC16_63BYTES   0xFBD9
#define EXPECTED_CRC16_2352BYTES 0x23F4

static const uint8_t *buffer;
static const uint8_t *buffer_misaligned;

class crc16Fixture : public ::testing::Test
{
public:
    crc16Fixture()
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

    ~crc16Fixture()
    {
        // resources cleanup, no exceptions allowed
    }

    // shared user data
};

TEST_F(crc16Fixture, crc16_auto)
{
    crc16_ctx *ctx = crc16_init();
    uint16_t   crc;

    EXPECT_NE(ctx, nullptr);

    crc16_update(ctx, buffer, 1048576);
    crc16_final(ctx, &crc);

    EXPECT_EQ(crc, EXPECTED_CRC16);
}

TEST_F(crc16Fixture, crc16_auto_misaligned)
{
    crc16_ctx *ctx = crc16_init();
    uint16_t   crc;

    EXPECT_NE(ctx, nullptr);

    crc16_update(ctx, buffer_misaligned + 1, 1048576);
    crc16_final(ctx, &crc);

    EXPECT_EQ(crc, EXPECTED_CRC16);
}

TEST_F(crc16Fixture, crc16_auto_15bytes)
{
    crc16_ctx *ctx = crc16_init();
    uint16_t   crc;

    EXPECT_NE(ctx, nullptr);

    crc16_update(ctx, buffer, 15);
    crc16_final(ctx, &crc);

    EXPECT_EQ(crc, EXPECTED_CRC16_15BYTES);
}

TEST_F(crc16Fixture, crc16_auto_31bytes)
{
    crc16_ctx *ctx = crc16_init();
    uint16_t   crc;

    EXPECT_NE(ctx, nullptr);

    crc16_update(ctx, buffer, 31);
    crc16_final(ctx, &crc);

    EXPECT_EQ(crc, EXPECTED_CRC16_31BYTES);
}

TEST_F(crc16Fixture, crc16_auto_63bytes)
{
    crc16_ctx *ctx = crc16_init();
    uint16_t   crc;

    EXPECT_NE(ctx, nullptr);

    crc16_update(ctx, buffer, 63);
    crc16_final(ctx, &crc);

    EXPECT_EQ(crc, EXPECTED_CRC16_63BYTES);
}

TEST_F(crc16Fixture, crc16_auto_2352bytes)
{
    crc16_ctx *ctx = crc16_init();
    uint16_t   crc;

    EXPECT_NE(ctx, nullptr);

    crc16_update(ctx, buffer, 2352);
    crc16_final(ctx, &crc);

    EXPECT_EQ(crc, EXPECTED_CRC16_2352BYTES);
}

#if defined(__x86_64__) || defined(__amd64) || defined(_M_AMD64) || defined(_M_X64) || defined(__I386__) || \
defined(__i386__) || defined(__THW_INTEL) || defined(_M_IX86)

TEST_F(crc16Fixture, crc16_avx2)
{
    if(!have_avx2()) return;

    crc16_ctx *ctx = crc16_init();
    uint16_t   crc;

    EXPECT_NE(ctx, nullptr);

    crc16_update_avx2(ctx, buffer, 1048576);
    crc16_final(ctx, &crc);

    EXPECT_EQ(crc, EXPECTED_CRC16);
}TEST_F(crc16Fixture, crc16_avx2_misaligned)
{
    if(!have_avx2()) return;

    crc16_ctx *ctx = crc16_init();
    uint16_t   crc;

    EXPECT_NE(ctx, nullptr);

    crc16_update_avx2(ctx, buffer_misaligned + 1, 1048576);
    crc16_final(ctx, &crc);

    EXPECT_EQ(crc, EXPECTED_CRC16);
}TEST_F(crc16Fixture, crc16_avx2_15bytes)
{
    if(!have_avx2()) return;

    crc16_ctx *ctx = crc16_init();
    uint16_t   crc;

    EXPECT_NE(ctx, nullptr);

    crc16_update_avx2(ctx, buffer, 15);
    crc16_final(ctx, &crc);

    EXPECT_EQ(crc, EXPECTED_CRC16_15BYTES);
}TEST_F(crc16Fixture, crc16_avx2_31bytes)
{
    if(!have_avx2()) return;

    crc16_ctx *ctx = crc16_init();
    uint16_t   crc;

    EXPECT_NE(ctx, nullptr);

    crc16_update_avx2(ctx, buffer, 31);
    crc16_final(ctx, &crc);

    EXPECT_EQ(crc, EXPECTED_CRC16_31BYTES);
}TEST_F(crc16Fixture, crc16_avx2_63bytes)
{
    if(!have_avx2()) return;

    crc16_ctx *ctx = crc16_init();
    uint16_t   crc;

    EXPECT_NE(ctx, nullptr);

    crc16_update_avx2(ctx, buffer, 63);
    crc16_final(ctx, &crc);

    EXPECT_EQ(crc, EXPECTED_CRC16_63BYTES);
}TEST_F(crc16Fixture, crc16_avx2_2352bytes)
{
    if(!have_avx2()) return;

    crc16_ctx *ctx = crc16_init();
    uint16_t   crc;

    EXPECT_NE(ctx, nullptr);

    crc16_update_avx2(ctx, buffer, 2352);
    crc16_final(ctx, &crc);

    EXPECT_EQ(crc, EXPECTED_CRC16_2352BYTES);
}

#endif