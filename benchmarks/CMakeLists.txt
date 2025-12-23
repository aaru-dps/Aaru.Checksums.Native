cmake_minimum_required(VERSION 3.15)
project(benchmarks CXX)

include(FetchContent)
FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.8.4
)
FetchContent_MakeAvailable(benchmark)

add_executable(bench_spamsum bench_spamsum.cpp)
target_link_libraries(bench_spamsum PRIVATE benchmark::benchmark "Aaru.Checksums.Native")
target_include_directories(bench_spamsum PRIVATE ${CMAKE_SOURCE_DIR})

# Copy DLL to benchmark output directory after build (Windows)
if (WIN32)
  add_custom_command(TARGET bench_spamsum POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "$<TARGET_FILE_DIR:Aaru.Checksums.Native>/$<TARGET_FILE_NAME:Aaru.Checksums.Native>"
      $<TARGET_FILE_DIR:bench_spamsum>
  )
endif()
