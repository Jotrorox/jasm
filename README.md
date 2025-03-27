# JASM Assembler

A modern, user-friendly x86_64 assembler for Linux.

## Features

- Simple, intuitive syntax
- Support for all x86_64 instructions
- ELF and raw binary output formats
- Built-in debugging support
- Fast compilation times
- Comprehensive error messages

## Installation

### Building from Source

1. Clone the repository:
```bash
git clone https://github.com/jotrorox/jasm.git
cd jasm
```

2. Build the project:
```bash
cc -o nob nob.c
./nob --type Release
```

The build system supports different build types:
- `Debug`: Includes debugging symbols and ASAN (Address Sanitizer)
- `Release`: Optimized build with maximum performance
- `Verbose`: Includes additional warnings and debug information

Additional build options:
- `--verbose`: Show detailed build information
- `--output <dir>`: Specify output directory (default: build)
- `--clean`: Clean the build directory
- `--install`: Install the executable
- `--uninstall`: Uninstall the executable
- `--prefix <path>`: Specify installation prefix (default: /usr/local/bin/jasm)

### System Requirements

- Linux x86_64
- A C compiler (`cc` in your path)

## Usage

Basic usage:
```bash
jasm input.jasm [output]
```

Options:
- `-h, --help`: Display help message
- `-v, --verbose`: Enable verbose output
- `-V, --version`: Display version information
- `-f, --format <format>`: Specify output format (elf, bin)

## Examples

### Hello World
```jasm
# Example: Print "Hello, world!" to stdout
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

## Documentation

For detailed documentation, visit our [documentation website](https://jotrorox.github.io/jasm/).

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## Author

Johannes (Jotrorox) Müller