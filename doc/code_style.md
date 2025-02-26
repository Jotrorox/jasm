# JASM Coding Style Guide

This document outlines the coding style guidelines for the JASM project.

## Code Formatting

The JASM project uses clang-format to automatically format C code. The specific rules are defined in the `.clang-format` file located in the project root directory.

### Key Style Rules

- Indentation: 4 spaces (no tabs)
- Column limit: 100 characters
- Braces: Opening brace on the same line for control statements, new line for functions
- Function parameters: One parameter per line when the function declaration doesn't fit on a single line
- Comments: Prefer `/* C-style comments */` for multi-line comments and `// C++-style comments` for single-line comments

## How to Format Code

### Automatic Formatting

To automatically format all code in the repository:

```bash
make format
```

This will run clang-format on all C source files in the project.

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

## Manual Code Reviews

While automatic formatting handles most style issues, code reviews should also check for:

1. Meaningful variable and function names
2. Proper documentation of functions and complex logic
3. Consistent error handling patterns
4. Memory management best practices

## Documentation Style

- Use full sentences with proper punctuation in comments
- Begin each file with a comment describing its purpose
- Document all functions with a comment that explains:
  - What the function does
  - Parameters and their meaning
  - Return value
  - Side effects or error conditions
