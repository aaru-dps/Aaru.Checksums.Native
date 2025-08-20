//
// Created by claunia on 5/10/21.
//

#include <climits>
#include <cstdint>
#include <cstring>

#include "../library.h"
#include "../crc16_ccitt.h"
#include "gtest/gtest.h"

#define EXPECTED_CRC16_CCITT           0x3640
#define EXPECTED_CRC16_CCITT_15BYTES   0x166e
#define EXPECTED_CRC16_CCITT_31BYTES   0xd016
#define EXPECTED_CRC16_CCITT_63BYTES   0x73c4
#define EXPECTED_CRC16_CCITT_2352BYTES 0x1946

static const uint8_t *buffer;
static const uint8_t *buffer_misaligned;

class crc16_ccittFixture : public ::testing::Test
{
public:
    crc16_ccittFixture()
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

    ~crc16_ccittFixture()
    {
        // resources cleanup, no exceptions allowed
    }

    // shared user data
};

TEST_F(crc16_ccittFixture, crc16_ccitt_auto)
{
    crc16_ccitt_ctx *ctx = crc16_ccitt_init();
    uint16_t         crc;

    EXPECT_NE(ctx, nullptr);

    crc16_ccitt_update(ctx, buffer, 1048576);
    crc16_ccitt_final(ctx, &crc);

    EXPECT_EQ(crc, EXPECTED_CRC16_CCITT);
}

TEST_F(crc16_ccittFixture, crc16_ccitt_auto_misaligned)
{
    crc16_ccitt_ctx *ctx = crc16_ccitt_init();
    uint16_t         crc;

    EXPECT_NE(ctx, nullptr);

    crc16_ccitt_update(ctx, buffer_misaligned + 1, 1048576);
    crc16_ccitt_final(ctx, &crc);

    EXPECT_EQ(crc, EXPECTED_CRC16_CCITT);
}

TEST_F(crc16_ccittFixture, crc16_ccitt_auto_15bytes)
{
    crc16_ccitt_ctx *ctx = crc16_ccitt_init();
    uint16_t         crc;

    EXPECT_NE(ctx, nullptr);

    crc16_ccitt_update(ctx, buffer, 15);
    crc16_ccitt_final(ctx, &crc);

    EXPECT_EQ(crc, EXPECTED_CRC16_CCITT_15BYTES);
}

TEST_F(crc16_ccittFixture, crc16_ccitt_auto_31bytes)
{
    crc16_ccitt_ctx *ctx = crc16_ccitt_init();
    uint16_t         crc;

    EXPECT_NE(ctx, nullptr);

    crc16_ccitt_update(ctx, buffer, 31);
    crc16_ccitt_final(ctx, &crc);

    EXPECT_EQ(crc, EXPECTED_CRC16_CCITT_31BYTES);
}

TEST_F(crc16_ccittFixture, crc16_ccitt_auto_63bytes)
{
    crc16_ccitt_ctx *ctx = crc16_ccitt_init();
    uint16_t         crc;

    EXPECT_NE(ctx, nullptr);

    crc16_ccitt_update(ctx, buffer, 63);
    crc16_ccitt_final(ctx, &crc);

    EXPECT_EQ(crc, EXPECTED_CRC16_CCITT_63BYTES);
}

TEST_F(crc16_ccittFixture, crc16_ccitt_auto_2352bytes)
{
    crc16_ccitt_ctx *ctx = crc16_ccitt_init();
    uint16_t         crc;

    EXPECT_NE(ctx, nullptr);

    crc16_ccitt_update(ctx, buffer, 2352);
    crc16_ccitt_final(ctx, &crc);

    EXPECT_EQ(crc, EXPECTED_CRC16_CCITT_2352BYTES);
}

#if defined(__x86_64__) || defined(__amd64) || defined(_M_AMD64) || defined(_M_X64) || defined(__I386__) || \
defined(__i386__) || defined(__THW_INTEL) || defined(_M_IX86)

TEST_F(crc16_ccittFixture, crc16_ccitt_clmul)
{
    if(!have_clmul()) return;

    crc16_ccitt_ctx *ctx = crc16_ccitt_init();
    uint16_t         crc;

    EXPECT_NE(ctx, nullptr);

    crc16_ccitt_update_clmul(ctx, buffer, 1048576);
    crc16_ccitt_final(ctx, &crc);

    EXPECT_EQ(crc, EXPECTED_CRC16_CCITT);
}

TEST_F(crc16_ccittFixture, crc16_ccitt_clmul_misaligned)
{
    if(!have_clmul()) return;

    crc16_ccitt_ctx *ctx = crc16_ccitt_init();
    uint16_t         crc;

    EXPECT_NE(ctx, nullptr);

    crc16_ccitt_update_clmul(ctx, buffer_misaligned + 1, 1048576);
    crc16_ccitt_final(ctx, &crc);

    EXPECT_EQ(crc, EXPECTED_CRC16_CCITT);
}

TEST_F(crc16_ccittFixture, crc16_ccitt_clmul_15bytes)
{
    if(!have_clmul()) return;

    crc16_ccitt_ctx *ctx = crc16_ccitt_init();
    uint16_t         crc;

    EXPECT_NE(ctx, nullptr);

    crc16_ccitt_update_clmul(ctx, buffer, 15);
    crc16_ccitt_final(ctx, &crc);

    EXPECT_EQ(crc, EXPECTED_CRC16_CCITT_15BYTES);
}

TEST_F(crc16_ccittFixture, crc16_ccitt_clmul_31bytes)
{
    if(!have_clmul()) return;

    crc16_ccitt_ctx *ctx = crc16_ccitt_init();
    uint16_t         crc;

    EXPECT_NE(ctx, nullptr);

    crc16_ccitt_update_clmul(ctx, buffer, 31);
    crc16_ccitt_final(ctx, &crc);

    EXPECT_EQ(crc, EXPECTED_CRC16_CCITT_31BYTES);
}

TEST_F(crc16_ccittFixture, crc16_ccitt_clmul_63bytes)
{
    if(!have_clmul()) return;

    crc16_ccitt_ctx *ctx = crc16_ccitt_init();
    uint16_t         crc;

    EXPECT_NE(ctx, nullptr);

    crc16_ccitt_update_clmul(ctx, buffer, 63);
    crc16_ccitt_final(ctx, &crc);

    EXPECT_EQ(crc, EXPECTED_CRC16_CCITT_63BYTES);
}

TEST_F(crc16_ccittFixture, crc16_ccitt_clmul_2352bytes)
{
    if(!have_clmul()) return;

    crc16_ccitt_ctx *ctx = crc16_ccitt_init();
    uint16_t         crc;

    EXPECT_NE(ctx, nullptr);

    crc16_ccitt_update_clmul(ctx, buffer, 2352);
    crc16_ccitt_final(ctx, &crc);

    EXPECT_EQ(crc, EXPECTED_CRC16_CCITT_2352BYTES);
}

#endif