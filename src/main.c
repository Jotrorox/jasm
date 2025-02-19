#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <errno.h>

#define MAX_LINE_LEN 256
#define MAX_LINES 1024
#define MAX_CODE_SIZE 4096
#define MAX_DATA_SIZE 4096
#define MAX_SYMBOLS 100

// ELF header & program header sizes.
#define ELF_HEADER_SIZE 64
#define PROGRAM_HEADER_SIZE 56
// The code section always starts at offset = ELF_HEADER_SIZE + PROGRAM_HEADER_SIZE.
#define CODE_OFFSET (ELF_HEADER_SIZE + PROGRAM_HEADER_SIZE)

// Base runtime address of our ELF mapping.
#define BASE_ADDR 0x400000

// Structures for code and data sections.
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

// Symbol structure for data labels.
typedef struct
{
    char name[32];
    uint64_t value;
} Symbol;

Symbol symbols[MAX_SYMBOLS];
size_t symbolCount = 0;

// Data directive structure to store parsed data info from the source.
typedef struct
{
    char label[32];
    char literal[MAX_LINE_LEN];
} DataDirective;

DataDirective dataDirectives[MAX_SYMBOLS];
size_t dataDirCount = 0;

// Supported registers for move instructions.
typedef struct
{
    const char *name;
    uint8_t opcode; // to be added to 0xB8; for 32-bit encoding, no REX prefix.
} RegisterMapping;

RegisterMapping regmap[] = {
    {"rax", 0x00},
    {"rcx", 0x01},
    {"rdx", 0x02},
    {"rbx", 0x03},
    {"rsi", 0x06},
    {"rdi", 0x07},
    {NULL, 0}};

uint8_t get_register_opcode(const char *reg)
{
    for (int i = 0; regmap[i].name != NULL; i++)
    {
        if (strcmp(reg, regmap[i].name) == 0)
        {
            return 0xB8 + regmap[i].opcode;
        }
    }
    fprintf(stderr, "Error: unknown register '%s'\n", reg);
    exit(1);
}

// Trim whitespace in-place.
char *trim(char *str)
{
    while (*str == ' ' || *str == '\t')
        str++;
    char *end = str + strlen(str) - 1;
    while (end >= str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r'))
    {
        *end = '\0';
        end--;
    }
    return str;
}

void add_symbol(const char *name, uint64_t value)
{
    if (symbolCount >= MAX_SYMBOLS)
    {
        fprintf(stderr, "Error: symbol table overflow\n");
        exit(1);
    }
    strncpy(symbols[symbolCount].name, name, sizeof(symbols[symbolCount].name) - 1);
    symbols[symbolCount].name[sizeof(symbols[symbolCount].name) - 1] = '\0';
    symbols[symbolCount].value = value;
    symbolCount++;
}

uint64_t lookup_symbol(const char *name)
{
    for (size_t i = 0; i < symbolCount; i++)
    {
        if (strcmp(symbols[i].name, name) == 0)
            return symbols[i].value;
    }
    fprintf(stderr, "Error: unknown symbol '%s'\n", name);
    exit(1);
}

// Check if a string represents a numeric constant.
int is_numeric(const char *str)
{
    if (!str || !*str)
        return 0;
    if (isdigit((unsigned char)str[0]) || str[0] == '-' ||
        (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')))
        return 1;
    return 0;
}

// For simulation purposes, determine the size (in bytes) that an instruction will emit.
// Supported instructions:
//    move <reg>, <immediate> -> 5 bytes if immediate fits in 32 bits, else 10 bytes.
//    call -> 2 bytes.
size_t simulate_instruction(const char *line)
{
    char buf[MAX_LINE_LEN];
    strncpy(buf, line, MAX_LINE_LEN - 1);
    buf[MAX_LINE_LEN - 1] = '\0';
    char *trimmed = trim(buf);
    if (strncmp(trimmed, "move", 4) == 0)
    {
        char *token = strtok(trimmed, " ,\t"); // "move"
        token = strtok(NULL, " ,\t");          // register
        if (!token)
            return 0;
        token = strtok(NULL, " ,\t"); // immediate (or symbol)
        if (!token)
            return 0;
        if (is_numeric(token))
        {
            uint64_t val = strtoull(token, NULL, 0);
            if (val <= 0xffffffffULL)
                return 5;
            else
                return 10;
        }
        else
        {
            // For symbols, assume 32-bit constant.
            return 5;
        }
    }
    else if (strncmp(trimmed, "call", 4) == 0)
    {
        return 2;
    }
    return 0;
}

// Read all lines from the input file.
size_t read_lines(const char *filename, char lines[][MAX_LINE_LEN])
{
    FILE *fp = fopen(filename, "r");
    if (!fp)
    {
        perror("fopen");
        exit(1);
    }
    size_t count = 0;
    while (fgets(lines[count], MAX_LINE_LEN, fp) != NULL && count < MAX_LINES)
        count++;
    fclose(fp);
    return count;
}

// Process escape sequences in a string literal.
// This function converts sequences such as "\n", "\t", "\\", "\"" into their actual byte values.
void process_escape_sequences(const char *input, char *output)
{
    while (*input)
    {
        if (*input == '\\')
        {
            input++;
            if (*input == 'n')
            {
                *output = '\n';
            }
            else if (*input == 't')
            {
                *output = '\t';
            }
            else if (*input == 'r')
            {
                *output = '\r';
            }
            else if (*input == '\\')
            {
                *output = '\\';
            }
            else if (*input == '"')
            {
                *output = '"';
            }
            else
            {
                // Unknown escape, copy as-is.
                *output = *input;
            }
        }
        else
        {
            *output = *input;
        }
        input++;
        output++;
    }
    *output = '\0';
}

// First pass: simulate instruction emission and collect data directives.
// Returns total simulated code size.
size_t first_pass(char lines[][MAX_LINE_LEN], size_t lineCount)
{
    size_t codeSize = 0;
    for (size_t i = 0; i < lineCount; i++)
    {
        char *trimmed = trim(lines[i]);
        if (trimmed[0] == '\0' || trimmed[0] == '#')
            continue;
        if (strncmp(trimmed, "data", 4) == 0)
        {
            // Process data directive.
            char *p = trimmed + 4;
            p = trim(p);
            char *label = strtok(p, " \t");
            if (!label)
            {
                fprintf(stderr, "Error: data directive requires a label\n");
                exit(1);
            }
            char *strLiteral = strtok(NULL, "\n");
            if (!strLiteral)
            {
                fprintf(stderr, "Error: data directive missing string literal\n");
                exit(1);
            }
            strLiteral = trim(strLiteral);
            if (strLiteral[0] != '"')
            {
                fprintf(stderr, "Error: data directive requires a string literal in quotes.\n");
                exit(1);
            }
            strLiteral++; // skip opening quote
            char *endQuote = strchr(strLiteral, '"');
            if (!endQuote)
            {
                fprintf(stderr, "Error: missing closing quote in data directive.\n");
                exit(1);
            }
            *endQuote = '\0';
            // Save the raw literal (escape sequences will be processed later).
            strncpy(dataDirectives[dataDirCount].label, label, sizeof(dataDirectives[dataDirCount].label) - 1);
            dataDirectives[dataDirCount].label[sizeof(dataDirectives[dataDirCount].label) - 1] = '\0';
            strncpy(dataDirectives[dataDirCount].literal, strLiteral, sizeof(dataDirectives[dataDirCount].literal) - 1);
            dataDirectives[dataDirCount].literal[sizeof(dataDirectives[dataDirCount].literal) - 1] = '\0';
            dataDirCount++;
        }
        else
        {
            // For instruction lines, simulate emission.
            size_t sz = simulate_instruction(trimmed);
            codeSize += sz;
        }
    }
    return codeSize;
}

// Emit machine code into the CodeBuffer for an instruction line.
void emit_instruction_line(CodeBuffer *codeBuf, const char *line)
{
    char buf[MAX_LINE_LEN];
    strncpy(buf, line, MAX_LINE_LEN - 1);
    buf[MAX_LINE_LEN - 1] = '\0';
    char *trimmed = trim(buf);
    if (trimmed[0] == '\0' || trimmed[0] == '#')
        return;
    if (strncmp(trimmed, "data", 4) == 0)
        return; // skip data directives.
    if (strncmp(trimmed, "move", 4) == 0)
    {
        // Syntax: move <register>, <immediate>
        char *token = strtok(trimmed, " ,\t"); // "move"
        token = strtok(NULL, " ,\t");          // register
        if (!token)
        {
            fprintf(stderr, "Error: expected register after 'move'\n");
            exit(1);
        }
        char reg[16];
        strncpy(reg, token, sizeof(reg) - 1);
        reg[sizeof(reg) - 1] = '\0';
        token = strtok(NULL, " ,\t"); // immediate or symbol
        if (!token)
        {
            fprintf(stderr, "Error: expected immediate after register '%s'\n", reg);
            exit(1);
        }
        if (is_numeric(token))
        {
            uint64_t val = strtoull(token, NULL, 0);
            if (val <= 0xffffffffULL)
            {
                uint8_t opcode = get_register_opcode(reg);
                codeBuf->bytes[codeBuf->size++] = opcode;
                for (int i = 0; i < 4; i++)
                {
                    codeBuf->bytes[codeBuf->size++] = (uint8_t)((val >> (8 * i)) & 0xff);
                }
            }
            else
            {
                codeBuf->bytes[codeBuf->size++] = 0x48;
                codeBuf->bytes[codeBuf->size++] = get_register_opcode(reg);
                for (int i = 0; i < 8; i++)
                {
                    codeBuf->bytes[codeBuf->size++] = (uint8_t)((val >> (8 * i)) & 0xff);
                }
            }
        }
        else
        {
            // Immediate is a symbol; lookup its value.
            uint64_t symVal = lookup_symbol(token);
            if (symVal <= 0xffffffffULL)
            {
                uint8_t opcode = get_register_opcode(reg);
                codeBuf->bytes[codeBuf->size++] = opcode;
                for (int i = 0; i < 4; i++)
                {
                    codeBuf->bytes[codeBuf->size++] = (uint8_t)((symVal >> (8 * i)) & 0xff);
                }
            }
            else
            {
                codeBuf->bytes[codeBuf->size++] = 0x48;
                codeBuf->bytes[codeBuf->size++] = get_register_opcode(reg);
                for (int i = 0; i < 8; i++)
                {
                    codeBuf->bytes[codeBuf->size++] = (uint8_t)((symVal >> (8 * i)) & 0xff);
                }
            }
        }
    }
    else if (strncmp(trimmed, "call", 4) == 0)
    {
        codeBuf->bytes[codeBuf->size++] = 0x0F;
        codeBuf->bytes[codeBuf->size++] = 0x05;
    }
    else
    {
        fprintf(stderr, "Error: unknown instruction '%s'\n", trimmed);
        exit(1);
    }
}

#pragma pack(push, 1)
typedef struct
{
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

typedef struct
{
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} Elf64_Phdr;
#pragma pack(pop)

// Write the ELF file with layout: ELF header, program header, code section, then data section.
void write_elf(const char *outfile, CodeBuffer *codeBuf, DataBuffer *dataBuf)
{
    size_t file_size = CODE_OFFSET + codeBuf->size + dataBuf->size;
    uint8_t *file_buf = calloc(1, file_size);
    if (!file_buf)
    {
        perror("calloc");
        exit(1);
    }

    uint8_t *p = file_buf;
    // Construct ELF header.
    Elf64_Ehdr eh = {0};
    memcpy(eh.e_ident, "\177ELF\2\1\1\0", 8);
    eh.e_type = 2;       // EXEC
    eh.e_machine = 0x3E; // AMD x86-64
    eh.e_version = 1;
    // Entry point is start of code.
    eh.e_entry = BASE_ADDR + CODE_OFFSET;
    eh.e_phoff = ELF_HEADER_SIZE;
    eh.e_shoff = 0;
    eh.e_flags = 0;
    eh.e_ehsize = ELF_HEADER_SIZE;
    eh.e_phentsize = PROGRAM_HEADER_SIZE;
    eh.e_phnum = 1;
    memcpy(p, &eh, ELF_HEADER_SIZE);

    p = file_buf + ELF_HEADER_SIZE;
    // Construct Program Header.
    Elf64_Phdr phdr = {0};
    phdr.p_type = 1;  // PT_LOAD
    phdr.p_flags = 5; // PF_R | PF_X
    phdr.p_offset = 0;
    phdr.p_vaddr = BASE_ADDR;
    phdr.p_paddr = BASE_ADDR;
    phdr.p_filesz = file_size;
    phdr.p_memsz = file_size;
    phdr.p_align = 0x1000;
    memcpy(p, &phdr, PROGRAM_HEADER_SIZE);

    // Copy code section.
    memcpy(file_buf + CODE_OFFSET, codeBuf->bytes, codeBuf->size);
    // Append data section immediately after code.
    memcpy(file_buf + CODE_OFFSET + codeBuf->size, dataBuf->bytes, dataBuf->size);

    FILE *out = fopen(outfile, "wb");
    if (!out)
    {
        perror("fopen");
        exit(1);
    }
    if (fwrite(file_buf, 1, file_size, out) != file_size)
    {
        perror("fwrite");
        exit(1);
    }
    fclose(out);
    free(file_buf);
    printf("Assembled %zu bytes of machine code and %zu bytes of data into ELF file: %s\n",
           codeBuf->size, dataBuf->size, outfile);
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <input.asm> <output binary>\n", argv[0]);
        return 1;
    }

    char lines[MAX_LINES][MAX_LINE_LEN];
    size_t lineCount = read_lines(argv[1], lines);

    // First pass: simulate instruction emission and collect data directives.
    size_t simulatedCodeSize = first_pass(lines, lineCount);

    // Now that we know simulatedCodeSize, the data section will begin at:
    // DATA_BASE = BASE_ADDR + CODE_OFFSET + simulatedCodeSize.
    uint64_t dataBase = BASE_ADDR + CODE_OFFSET + simulatedCodeSize;

    // Process collected data directives: assign their symbol addresses and emit processed data.
    DataBuffer dataBuf = {.size = 0};
    for (size_t i = 0; i < dataDirCount; i++)
    {
        uint64_t addr = dataBase + dataBuf.size;
        add_symbol(dataDirectives[i].label, addr);
        // Process escape sequences in the literal.
        char processed[MAX_LINE_LEN];
        process_escape_sequences(dataDirectives[i].literal, processed);
        size_t len = strlen(processed);
        if (dataBuf.size + len > MAX_DATA_SIZE)
        {
            fprintf(stderr, "Error: data buffer overflow\n");
            exit(1);
        }
        memcpy(dataBuf.bytes + dataBuf.size, processed, len);
        dataBuf.size += len;
    }

    // Second pass: emit instructions.
    CodeBuffer codeBuf = {.size = 0};
    for (size_t i = 0; i < lineCount; i++)
    {
        char *trimmed = trim(lines[i]);
        if (trimmed[0] == '\0' || trimmed[0] == '#')
            continue;
        if (strncmp(trimmed, "data", 4) == 0)
            continue;
        emit_instruction_line(&codeBuf, trimmed);
    }

    write_elf(argv[2], &codeBuf, &dataBuf);
    return 0;
}