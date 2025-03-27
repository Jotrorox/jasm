# JASM Coding Style Guide

This document outlines the coding style guidelines for the JASM project.

## Code Formatting

The JASM project uses clang-format to automatically format C code. The specific rules are defined in the `.clang-format` file located in the project root directory.

### Key Style Rules

- Indentation: 4 spaces (no tabs)
- Column limit: 100 characters
- Braces: 
  - Opening brace on the same line for control statements
  - Opening brace on a new line for functions
  - No braces on single-line blocks
- Function parameters: One parameter per line when the function declaration doesn't fit on a single line
- Comments: 
  - Prefer `/* C-style comments */` for multi-line comments
  - Use `// C++-style comments` for single-line comments
  - Two spaces before trailing comments
- Pointer alignment: Right-aligned (e.g., `char* ptr`)
- Include blocks: Preserve original order
- Maximum empty lines: 1 between blocks
- Case labels: Indented with 4 spaces
- Binary operators: Break before non-assignment operators

## How to Format Code

### Automatic Formatting

To format code, you can use clang-format directly:

```bash
# Format a single file
clang-format -i path/to/file.c

# Format all C files in a directory recursively
clang-format -i **/*.c

# Format all C and header files in a directory recursively
clang-format -i **/*.{c,h}
```

The `-i` flag means "in-place" formatting. Without it, clang-format will print the formatted code to stdout instead of modifying the file.

### Editor Integration

For the best developer experience, configure your editor to use clang-format:

#### Visual Studio Code

1. Install the "C/C++" extension
2. Set `"C_Cpp.formatting": "clangFormat"` in your settings
3. The extension will use the project's `.clang-format` file automatically

#### Vim/NeoVim

Install a clang-format plugin or use:

```vim
:setlocal formatprg=clang-format
:setlocal formatexpr=
```

#### Emacs

Use the `clang-format.el` script.

## Code Organization

### File Structure

- One logical unit per file
- Header files should be self-contained and include-protected
- Implementation files should include their corresponding header first
- Group includes in the following order:
  1. System headers
  2. Third-party headers
  3. Project headers

### Naming Conventions

- Functions: Use verb-noun combinations (e.g., `calculate_sum`)
- Variables: Use descriptive names in snake_case
- Constants: Use UPPER_SNAKE_CASE
- Types: Use PascalCase for type names
- Macros: Use UPPER_SNAKE_CASE

## Documentation Style

### File Headers

Each source file should begin with a header comment containing:
- File name
- Brief description of the file's purpose
- Author(s)
- Copyright notice
- License information

### Function Documentation

Document all functions with a comment that explains:
- Function name and purpose
- Parameters and their meaning
- Return value and its meaning
- Side effects or error conditions
- Thread safety considerations (if applicable)

Example:
```c
/* Calculates the sum of two integers.
 *
 * @param a First integer to add
 * @param b Second integer to add
 * @return The sum of a and b
 * @throws None
 */
int add(int a, int b);
```

### Code Comments

- Use comments to explain "why" not "what"
- Keep comments up to date with code changes
- Remove commented-out code
- Use TODO comments for future improvements
- Document complex algorithms or non-obvious optimizations

## Manual Code Reviews

While automatic formatting handles most style issues, code reviews should also check for:

1. Meaningful variable and function names
2. Proper documentation of functions and complex logic
3. Consistent error handling patterns
4. Memory management best practices
5. Thread safety considerations
6. Performance implications
7. Security considerations
8. Code duplication
9. Test coverage
10. API design and consistency

## Error Handling

- Use consistent error return values across the codebase
- Document error conditions in function documentation
- Handle all error cases explicitly
- Use appropriate error logging levels
- Clean up resources in error cases
- Consider using error codes or result types for complex error scenarios

## Memory Management

- Always check malloc/calloc/realloc return values
- Use appropriate memory allocation strategies
- Document memory ownership and lifetime
- Avoid memory leaks by using RAII patterns where possible
- Consider using memory pools for frequently allocated objects
- Use valgrind or similar tools to check for memory issues
