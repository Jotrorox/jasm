cmake_minimum_required(VERSION 3.10)

# Project name and version from original Makefile
project(jasm VERSION 0.1
    DESCRIPTION "JASM Assembler"
    LANGUAGES C)

# Author information
set(AUTHOR "Johannes (Jotrorox) Müller")
set(COPYRIGHT "2025 ${AUTHOR}")

# Directory structure
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")

# Find all source files
file(GLOB SOURCES "${SRC_DIR}/*.c")

# Create the executable target
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE ${LIB_DIR})

# Basic compiler flags for all build types
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)

# Configure build types
set(CMAKE_C_FLAGS_DEBUG "-g -fsanitize=address -fno-omit-frame-pointer -fno-inline -fno-strict-aliasing -fno-lto -O0")
set(CMAKE_C_FLAGS_RELEASE "-DNDEBUG -O3 -fomit-frame-pointer -finline-functions -fstrict-aliasing -flto -march=native -funroll-loops")

# Custom build type for verbose output
set(CMAKE_C_FLAGS_VERBOSE "-DVERBOSE -O2 -Wall -Wextra")

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build (Debug, Release, Verbose)" FORCE)
endif()

# Print build configuration information
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "Copyright (c) ${COPYRIGHT}")
