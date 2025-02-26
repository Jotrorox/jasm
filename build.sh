#!/bin/bash

# Check if CMake is installed
if ! command -v cmake &> /dev/null
then
    echo "CMake could not be found. Please install CMake to proceed."
    exit
fi

# build type from command line argument
if [ "$#" -gt 0 ]; then
    BUILD_TYPE=$1
else
    BUILD_TYPE="Debug"
fi

# Default build type is Debug
BUILD_TYPE=${1:-Debug}
BUILD_DIR="build"

# Create build directory if it doesn't exist
mkdir -p ${BUILD_DIR}

# Navigate to build directory and run CMake
cd ${BUILD_DIR}
cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} ..

# Build the project
cmake --build .

# Return to the original directory
cd ..

echo "Build completed in ${BUILD_DIR}/ directory"
