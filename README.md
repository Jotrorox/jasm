# 🚀 JASM Assembler

<div align="center">

![Version](https://img.shields.io/badge/version-0.1-blue)
![License](https://img.shields.io/badge/license-MIT-green)
![Platform](https://img.shields.io/badge/platform-Linux-lightgrey)

**A modern, user-friendly x86_64 assembler for Linux.**

[Installation](#installation) •
[Quick Start](#quick-start) •
[Examples](#examples) •
[Documentation](#documentation) •
[CLI Reference](#command-line-interface)

</div>

## ✨ Features

- 🔥 **Lightning Fast**: Optimized assembler with quick compilation times
- 💡 **Easy to Use**: Simple syntax and helpful error messages
- 🔧 **Modern**: Support for latest x86_64 instructions
- 📦 **Flexible Output**: Generate ELF executables or raw binary files

## 📋 Installation

```bash
# Clone the repository
git clone https://github.com/jotrorox/jasm.git

# Enter the directory
cd jasm

# Build the project
make
```

## 🚀 Quick Start

1. Create a new file named `hello.jasm`
2. Write your assembly code following the [syntax guide](#syntax-reference)
3. Compile your code:
   ```bash
   jasm hello.jasm
   ```
4. Run the executable:
   ```bash
   ./a.out
   ```

## 📝 Examples

### Hello World

```assembly
# Example: Print "Hello, world!" to stdout
# Syscall details:
#   sys_write: rax=1, rdi=stdout, rsi=buffer, rdx=length

data msg "Hello, World!\n"

# sys_write(stdout, msg, 14)
mov rax, 1       # sys_write
mov rdi, 1       # stdout
mov rsi, msg     # message
mov rdx, 14      # length
call

# sys_exit(0)
mov rax, 60      # sys_exit
mov rdi, 0       # status
call
```

## 💻 Command Line Interface

```
jasm [options] <input.jasm> [output]

Options:
  -h, --help            Display help message
  -v, --verbose         Enable verbose output
  -V, --version         Display version information
  -f, --format <format> Specify output format (elf, bin)
```

### Example Usage

- **Basic Compilation**:
  ```bash
  jasm program.jasm
  ```
  Creates an ELF executable named `a.out`

- **Binary Output**:
  ```bash
  jasm -f bin program.jasm prog.bin
  ```
  Creates a raw binary file

- **Verbose Mode**:
  ```bash
  jasm -v program.jasm prog
  ```
  Shows detailed assembly information

## 📚 Documentation

For complete documentation, visit the [JASM Documentation](https://github.com/jotrorox/jasm).

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## 📄 License

Copyright © 2025 Johannes (Jotrorox) Müller

This project is licensed under the MIT License.