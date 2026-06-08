cmake_minimum_required(VERSION 3.22)

set(EXTRA_COMPONENT_DIRS "../components")

set(TEST_COMPONENTS
    "board_i2c"
    "environment_measurements"
    "nvs"
    CACHE STRING
    "Project components whose Unity tests are included"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

idf_build_set_property(MINIMAL_BUILD ON)

# Unity prints the compiler-provided __FILE__ value for each test result.
# Replace the absolute repository path with an empty prefix so test output
# contains paths such as components/nvs/test/test_rm_nvs.c instead of a
# developer-specific path outside the repository.
get_filename_component(REPOSITORY_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
idf_build_set_property(
    COMPILE_OPTIONS
    "-ffile-prefix-map=${REPOSITORY_ROOT}/="
    APPEND
)

project(redmole_tests)
