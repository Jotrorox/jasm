# JASM Assembler

A simple easy-to-use assembler for the JASM language written in Zig.
It is meant to be fast, easy to use, and portable. For now it's only 
for educational and recreational purposes. And it's not even close to 
being complete. So don't expect it to be stable or feature-complete. 
But it's a start. And it's fun to write.

## Table of Contents

- [Features](#features)
- [Installation](#installation)
  - [Building from Source](#building-from-source)
  - [System Requirements](#system-requirements)
- [Usage](#usage)
- [Examples](#examples)
  - [Hello World](#hello-world)
- [Documentation](#documentation)
- [License](#license)
- [Contributing](#contributing)
- [Author](#author)

## Features

- Fast
- Easy to use
- Portable
- Simple
- Fun
- Working ( not yet :) )

## Installation

### Prerequisites

- [Zig](https://ziglang.org/) (master branch or 1.15.0-dev.369+1a2ceb36c)
- That's it, this is all you need to build the assembler from scratch.

### Building from Source

Just run `zig build` in the project directory. This will build the assembler
and place it in the `zig-out/bin` directory.

```bash
# Clone the repository
$ git clone https://github.com/jotrorox/jasm.git
$ cd jasm

# Build the assembler
$ zig build

# Run the assembler
$ ./zig-out/bin/jasm
```

Just run `zig build run` to build and run the assembler in one step.

```bash
# Clone the repository
$ git clone https://github.com/jotrorox/jasm.git
$ cd jasm

# Build and run the assembler
$ zig build run -- <arguments>
```

Or for a optimized build, run `zig build -Doptimize=ReleaseSafe`.

## Usage

```bash
jasm [options] [file]

Options:
  -h, --help      Display this help message
  -V, --version   Display version information
```

## Examples

### Hello World

ToDo

## Documentation

For detailed documentation, visit our [documentation website](https://jotrorox.github.io/jasm/).

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contributing

Contributions are welcome! Please feel free to submit a [Pull Request](https://github.com/jotrorox/jasm/pulls) or an [Issue](https://github.com/jotrorox/jasm/issues).

## Author

- **Johannes (jotrorox) Müller** - [jotrorox](https://github.com/jotrorox)

See also the list of [contributors](https://github.com/jotrorox/jasm/contributors) who participated in this project.