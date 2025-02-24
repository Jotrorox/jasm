#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "binary_writer.h"

/* ELF file related constants. */
#define ELF_HEADER_SIZE 64
#define PROGRAM_HEADER_SIZE 56
#define CODE_OFFSET (ELF_HEADER_SIZE + PROGRAM_HEADER_SIZE)
#define BASE_ADDR 0x400000

/* Write the assembled code and data as an ELF executable file */
int write_elf_file(const char* outfile, CodeBuffer* codeBuf, DataBuffer* dataBuf, uint64_t entry_point) {
    size_t file_size = CODE_OFFSET + codeBuf->size + dataBuf->size;
    uint8_t* file_buf = calloc(1, file_size);
    if (!file_buf) {
        perror("calloc");
        return 1;
    }
    uint8_t* p = file_buf;
    
    /* Construct ELF header */
    typedef struct {
        uint8_t e_ident[16];
        uint16_t e_type;
        uint16_t e_machine;
        uint32_t e_version;
        uint64_t e_entry;
        uint64_t e_phoff;
        uint64_t e_shoff;
        uint32_t e_flags;
        uint16_t e_ehsize;
        uint16_t e_phentsize;
        uint16_t e_phnum;
        uint16_t e_shentsize;
        uint16_t e_shnum;
        uint16_t e_shstrndx;
    } Elf64_Ehdr;
    
    Elf64_Ehdr eh = {0};
    memcpy(eh.e_ident, "\177ELF\2\1\1\0", 8);
    eh.e_type = 2;       /* EXEC */
    eh.e_machine = 0x3E; /* AMD x86-64 */
    eh.e_version = 1;
    eh.e_entry = entry_point;
    eh.e_phoff = ELF_HEADER_SIZE;
    eh.e_shoff = 0;
    eh.e_flags = 0;
    eh.e_ehsize = ELF_HEADER_SIZE;
    eh.e_phentsize = PROGRAM_HEADER_SIZE;
    eh.e_phnum = 1;
    memcpy(p, &eh, ELF_HEADER_SIZE);

    p = file_buf + ELF_HEADER_SIZE;
    
    /* Construct Program Header */
    typedef struct {
        uint32_t p_type;
        uint32_t p_flags;
        uint64_t p_offset;
        uint64_t p_vaddr;
        uint64_t p_paddr;
        uint64_t p_filesz;
        uint64_t p_memsz;
        uint64_t p_align;
    } Elf64_Phdr;
    
    Elf64_Phdr ph = {0};
    ph.p_type = 1;                        /* PT_LOAD */
    ph.p_flags = 7; /* PF_R | PF_W | PF_X */ /* Make segment readable, writable and executable */
    ph.p_offset = 0;
    ph.p_vaddr = BASE_ADDR;
    ph.p_paddr = BASE_ADDR;
    ph.p_filesz = file_size;
    ph.p_memsz = file_size;
    ph.p_align = 0x1000;
    memcpy(p, &ph, PROGRAM_HEADER_SIZE);

    /* Copy code section */
    memcpy(file_buf + CODE_OFFSET, codeBuf->bytes, codeBuf->size);
    /* Append data section */
    memcpy(file_buf + CODE_OFFSET + codeBuf->size, dataBuf->bytes, dataBuf->size);

    FILE* out = fopen(outfile, "wb");
    if (!out) {
        perror("fopen");
        free(file_buf);
        return 1;
    }
    
    if (fwrite(file_buf, 1, file_size, out) != file_size) {
        perror("fwrite");
        fclose(out);
        free(file_buf);
        return 1;
    }
    
    fclose(out);
    free(file_buf);
    
    printf("Assembled %zu bytes of machine code and %zu bytes of data into ELF file: %s\n",
           codeBuf->size, dataBuf->size, outfile);
    
    return 0;
} 