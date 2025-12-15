# NBox Testing Suite
# This is a SEPARATE build system for testing only.
# Users building NBox do NOT need this.

cmake_minimum_required(VERSION 3.16)
project(NBox_Tests)

# Set C++ standard to match main project
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find dependencies (same as main project)
find_package(Geant4 REQUIRED ui_all vis_all)
include(${Geant4_USE_FILE})

find_package(ROOT REQUIRED COMPONENTS Core Hist RIO)
include(${ROOT_USE_FILE})

# Google Test setup using FetchContent (no system installation needed)
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)
# For Windows: Prevent overriding parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Include NBox source directories
include_directories(
    ${PROJECT_SOURCE_DIR}/../include
    ${PROJECT_SOURCE_DIR}/helpers
)

# Gather NBox source files (reuse from main project)
file(GLOB NBOX_SOURCES ${PROJECT_SOURCE_DIR}/../src/*.cc)
file(GLOB NBOX_HEADERS ${PROJECT_SOURCE_DIR}/../include/*.hh)

# Test helper sources
file(GLOB TEST_HELPERS ${PROJECT_SOURCE_DIR}/helpers/*.cc)

# Unit test sources
file(GLOB UNIT_TESTS ${PROJECT_SOURCE_DIR}/unit/*.cc)

# Integration test sources
file(GLOB INTEGRATION_TESTS ${PROJECT_SOURCE_DIR}/integration/*.cc)

# Create test executable
add_executable(nbox_tests
    ${UNIT_TESTS}
    ${INTEGRATION_TESTS}
    ${TEST_HELPERS}
    ${NBOX_SOURCES}
    ${NBOX_HEADERS}
)

# Link libraries
target_link_libraries(nbox_tests
    GTest::gtest_main
    ${Geant4_LIBRARIES}
    ${ROOT_LIBRARIES}
)

# Enable testing
enable_testing()
include(GoogleTest)
gtest_discover_tests(nbox_tests)

# Copy test fixtures to build directory
file(COPY ${PROJECT_SOURCE_DIR}/fixtures
     DESTINATION ${PROJECT_BINARY_DIR})

# Print build information
message(STATUS "===========================================")
message(STATUS "NBox Testing Suite Configuration")
message(STATUS "===========================================")
message(STATUS "Google Test: FetchContent (automatic)")
message(STATUS "Geant4:      ${Geant4_VERSION}")
message(STATUS "ROOT:        ${ROOT_VERSION}")
message(STATUS "Unit tests:  ${UNIT_TESTS}")
message(STATUS "===========================================")
