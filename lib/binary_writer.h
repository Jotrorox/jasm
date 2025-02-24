#ifndef BINARY_WRITER_H
#define BINARY_WRITER_H

#include <stdint.h>
#include <stddef.h>

/* Binary writer interface for different target formats.
 * This interface allows the assembler to generate different
 * executable formats without changing the assembly processing code.
 */

/* Data structures for code and data sections. */
typedef struct {
    uint8_t* bytes;
    size_t size;
    size_t capacity;
} CodeBuffer;

typedef struct {
    uint8_t* bytes;
    size_t size;
    size_t capacity;
} DataBuffer;

/* Function pointer type for writing binary output */
typedef int (*binary_writer_fn)(const char* output_filename, 
                              CodeBuffer* codeBuf, 
                              DataBuffer* dataBuf,
                              uint64_t entry_point);

/* Initialize code and data buffers */
void init_code_buffer(CodeBuffer* buffer, size_t initial_capacity);
void init_data_buffer(DataBuffer* buffer, size_t initial_capacity);

/* Free allocated memory in buffers */
void free_code_buffer(CodeBuffer* buffer);
void free_data_buffer(DataBuffer* buffer);

/* Write a binary file in ELF format (implementation in elf_writer.c) */
int write_elf_file(const char* output_filename, 
                  CodeBuffer* codeBuf, 
                  DataBuffer* dataBuf,
                  uint64_t entry_point);

#endif /* BINARY_WRITER_H */ 