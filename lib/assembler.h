#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdint.h>
#include <stddef.h>

/* Constants */
#define MAX_LINE_LEN 256
#define MAX_LINES 1024
#define MAX_CODE_SIZE 4096
#define MAX_DATA_SIZE 4096
#define MAX_SYMBOLS 100

/* ELF file related constants. */
#define ELF_HEADER_SIZE 64
#define PROGRAM_HEADER_SIZE 56
#define CODE_OFFSET (ELF_HEADER_SIZE + PROGRAM_HEADER_SIZE)
#define BASE_ADDR 0x400000

/* Data structures for code and data sections. */
typedef struct
{
    uint8_t bytes[MAX_CODE_SIZE];
    size_t size;
} CodeBuffer;

typedef struct
{
    uint8_t bytes[MAX_DATA_SIZE];
    size_t size;
} DataBuffer;

/* The assembler module provides a single function to assemble an input file
   into an ELF executable.
*/
int assemble(const char *input_filename, const char *output_filename);

#endif // ASSEMBLER_H