cmake_minimum_required (VERSION 2.6)
project (Tail)

include (CheckFunctionExists)
check_function_exists (mmap HAVE_MMAP)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

aux_source_directory(tail tail_sources)
include_directories(tail)

add_executable(tail ${tail_sources})
add_executable(gen_text tests/gen_text.c)

add_custom_command(TARGET tail PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_SOURCE_DIR}/tests ${CMAKE_CURRENT_BINARY_DIR}/tests)

if (HAVE_MMAP)
  target_compile_definitions(tail PRIVATE HAVE_MMAP=1)
endif ()

set(TAIL_DESTINATION_BIN bin)
install (TARGETS tail DESTINATION ${TAIL_DESTINATION_BIN})
install (TARGETS gen_text DESTINATION ${TAIL_DESTINATION_BIN})

################################################################################
include(CTest)
set (CTEST_PROJECT_NAME "Tail")

add_test (GetTextRuns ${TAIL_DESTINATION_BIN}/gen_text)

add_test (GetText3x5 ${TAIL_DESTINATION_BIN}/gen_text string:3:a:5)
set_tests_properties (GetText3x5 PROPERTIES
    PASS_REGULAR_EXPRESSION "^aaaaa\naaaaa\naaaaa\n$")

add_test (TailRuns bash "-c"
  "${TAIL_DESTINATION_BIN}/gen_text string:5:a:5 | ${TAIL_DESTINATION_BIN}/tail -n 2")
set_tests_properties (TailRuns PROPERTIES PASS_REGULAR_EXPRESSION "^aaaaa\naaaaa\n$")

add_test (BehaveTests bash "-c" "cd tests/behave; behave")
set_tests_properties (BehaveTests PROPERTIES FAIL_REGULAR_EXPRESSION "^Failing scenarios:")


# The 'check' target is used to perform a build and then run tests.  This
# approach is done in lieu of a way to make the CMake generated 'test' target
# depend on 'all'.
add_custom_target(
  check
  COMMAND ${CMAKE_COMMAND}
    --build ${CMAKE_BINARY_DIR}
    --target all
  COMMAND ${CMAKE_CTEST_COMMAND})
