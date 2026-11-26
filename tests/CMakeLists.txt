if ("${AARU_BUILD_PACKAGE}" MATCHES 1)
  return()
endif ()

# 'Google_test' is the subproject name
project(tests)

# 'lib' is the folder with Google Test sources
add_subdirectory(lib)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data/random
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/data/)

# 'Google_Tests_run' is the target name
# 'test1.cpp tests2.cpp' are source files with tests
add_executable(tests_run adler32.cpp crc16.cpp crc16_ccitt.cpp crc32.cpp crc64.cpp fletcher16.cpp fletcher32.cpp spamsum.cpp)
target_link_libraries(tests_run gtest gtest_main "Aaru.Checksums.Native")

# Copy DLL to test output directory after build (Windows)
if (WIN32)
  add_custom_command(TARGET tests_run POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "$<TARGET_FILE_DIR:Aaru.Checksums.Native>/$<TARGET_FILE_NAME:Aaru.Checksums.Native>"
      $<TARGET_FILE_DIR:tests_run>
  )
endif()
