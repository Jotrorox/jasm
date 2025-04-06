cmake_minimum_required(VERSION 3.10)

project(jasm VERSION 0.1 LANGUAGES C)

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")

file(GLOB SOURCES "${SRC_DIR}/*.c")

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${LIB_DIR})

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)