#include <benchmark/benchmark.h>
#include "library.h"
#include "spamsum.h"
#include <cstdlib>
#include <cstring>
#include <random>

#define TEST_DATA_SIZE (1024 * 1024)

static void BM_spamsum_update(benchmark::State& state) {
    spamsum_ctx *ctx = spamsum_init();
    uint8_t *data = (uint8_t*)malloc(TEST_DATA_SIZE);
    // Fill buffer with random data
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, 255);
    for (size_t i = 0; i < TEST_DATA_SIZE; ++i) {
        data[i] = static_cast<uint8_t>(dis(gen));
    }
    for (auto _ : state) {
        spamsum_update(ctx, data, TEST_DATA_SIZE);
    }
    free(data);
    spamsum_free(ctx);
}

BENCHMARK(BM_spamsum_update)->MinTime(5.0)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
