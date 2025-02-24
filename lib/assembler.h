#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdint.h>
#include <stddef.h>
#include "binary_writer.h" /* Include our new interface */

/* Constants for assembly processing */
#define MAX_LINE_LEN 256
#define MAX_LINES 1024
#define MAX_SYMBOLS 100

/* Base address for code (used to calculate entry point and symbol addresses) */
#define BASE_ADDR 0x400000
#define CODE_OFFSET 120  /* Reasonable offset for most formats */

/* Assembly options struct to control the assembler behavior */
typedef struct {
    const char* input_filename;    /* Source file to assemble */
    const char* output_filename;   /* Output binary file name */
    binary_writer_fn writer;       /* Function to write the output binary */
    int verbose;                   /* Enable verbose output */
} AssemblerOptions;

/* The assembler module provides functions to assemble an input file
   into different binary formats.
*/

/* Main assembly function - processes the input file and generates
   code and data buffers, then uses the specified writer to create output */
int assemble(const AssemblerOptions* options);

/* Predefined writer functions for supported formats */
int assemble_to_elf(const char* input_filename, const char* output_filename);

#endif // ASSEMBLER_H