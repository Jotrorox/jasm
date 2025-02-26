#!/bin/bash

# Script to format all C/C++ files in the JASM project
# using the project's .clang-format configuration

# Exit on error
set -e

# Get the project root directory (parent of the directory containing this script)
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$( cd "$SCRIPT_DIR/.." && pwd )"

# Print some information
echo "JASM Code Formatter"
echo "=================="
echo "Project root: $PROJECT_ROOT"
echo "Using clang-format version: $(clang-format --version)"
echo

# Find and format all C/C++ files
echo "Formatting C/C++ files..."
find "$PROJECT_ROOT" \
    -type f \( -name "*.c" -o -name "*.h" -o -name "*.cpp" -o -name "*.hpp" \) \
    -not -path "*/build/*" \
    -not -path "*/\.*" \
    | while read -r file; do
        echo "  Processing: $file"
        clang-format -i -style=file "$file"
    done

echo
echo "Formatting complete!"
