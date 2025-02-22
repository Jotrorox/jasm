#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include "assembler.h"

/* ---- Internal Constants and Structures ---- */

/* Data directive structure for storing parsed data info. */
typedef struct
{
    char label[32];
    enum
    {
        DATA_STRING, /* String literal */
        DATA_BUFFER, /* Raw buffer of given size */
    } type;
    union
    {
        char literal[MAX_LINE_LEN];
        size_t size;
    } data;
} DataDirective;

/* Internal arrays for symbol and data directive storage. */
static char lines[MAX_LINES][MAX_LINE_LEN];
static DataDirective dataDirectives[MAX_SYMBOLS];
static size_t dataDirCount = 0;

/* Symbol structure. */
typedef struct
{
    char name[32];
    uint64_t value;
} Symbol;

/* Internal symbol table. */
static Symbol symbols[MAX_SYMBOLS];
static size_t symbolCount = 0;

/* ---- Utility Functions ---- */

/* Trim leading and trailing whitespace; modifies string in place. */
static char *trim(char *str)
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

/* Add a symbol to the symbol table. */
static void add_symbol(const char *name, uint64_t value)
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

/* Lookup symbol value by name. Exits if symbol not found. */
static uint64_t lookup_symbol(const char *name)
{
    for (size_t i = 0; i < symbolCount; i++)
    {
        if (strcmp(symbols[i].name, name) == 0)
            return symbols[i].value;
    }
    fprintf(stderr, "Error: unknown symbol '%s'\n", name);
    exit(1);
}

/* Check if string is a numeric constant. */
static int is_numeric(const char *str)
{
    if (!str || !*str)
        return 0;
    if (isdigit((unsigned char)str[0]) || str[0] == '-' ||
        (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')))
        return 1;
    return 0;
}

/* Process escape sequences in string literal.
   Converts: \n, \t, \r, \\ and \".
*/
void process_escape_sequences(const char *input, char *output)
{
    while (*input)
    {
        if (*input == '\\')
        {
            input++;
            if (*input == 'n')
                *output = '\n';
            else if (*input == 't')
                *output = '\t';
            else if (*input == 'r')
                *output = '\r';
            else if (*input == '\\')
                *output = '\\';
            else if (*input == '"')
                *output = '"';
            else
                *output = *input;
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

/* Check if string represents a memory reference [symbol] */
static int is_memory_ref(const char *str)
{
    return str[0] == '[' && str[strlen(str) - 1] == ']';
}

/* Extract symbol name from memory reference [symbol] */
static char *extract_memory_ref(const char *str)
{
    static char buf[MAX_LINE_LEN];
    size_t len = strlen(str) - 2; /* exclude [] */
    strncpy(buf, str + 1, len);
    buf[len] = '\0';
    return buf;
}

/* Process data directive.
   Formats supported:
   - data <label> "string literal"
   - data <label> size <number>
*/
static void process_data_directive(char *trimmed)
{
    char *p = trimmed + 4;
    p = trim(p);
    char *label = strtok(p, " \t");
    if (!label)
    {
        fprintf(stderr, "Error: data directive requires a label\n");
        exit(1);
    }
    char *value = strtok(NULL, "\n");
    if (!value)
    {
        fprintf(stderr, "Error: data directive missing value\n");
        exit(1);
    }
    value = trim(value);

    /* Save the directive for later processing */
    strncpy(dataDirectives[dataDirCount].label, label, sizeof(dataDirectives[dataDirCount].label) - 1);
    dataDirectives[dataDirCount].label[sizeof(dataDirectives[dataDirCount].label) - 1] = '\0';

    if (value[0] == '"')
    {
        /* String literal */
        dataDirectives[dataDirCount].type = DATA_STRING;
        value++; /* skip opening quote */
        char *endQuote = strchr(value, '"');
        if (!endQuote)
        {
            fprintf(stderr, "Error: missing closing quote in data directive\n");
            exit(1);
        }
        *endQuote = '\0';
        strncpy(dataDirectives[dataDirCount].data.literal, value, sizeof(dataDirectives[dataDirCount].data.literal) - 1);
        dataDirectives[dataDirCount].data.literal[sizeof(dataDirectives[dataDirCount].data.literal) - 1] = '\0';
    }
    else if (strncmp(value, "size", 4) == 0)
    {
        /* Buffer allocation */
        dataDirectives[dataDirCount].type = DATA_BUFFER;
        char *sizeStr = value + 4;
        sizeStr = trim(sizeStr);
        if (!is_numeric(sizeStr))
        {
            fprintf(stderr, "Error: size must be a number\n");
            exit(1);
        }
        dataDirectives[dataDirCount].data.size = strtoull(sizeStr, NULL, 0);
    }
    else
    {
        fprintf(stderr, "Error: data directive requires either a string literal or 'size <number>'\n");
        exit(1);
    }
    dataDirCount++;
}

/* Check if a line is a label definition (ends with ':') */
static int is_label(const char *line)
{
    size_t len = strlen(line);
    return len > 0 && line[len - 1] == ':';
}

/* Process a label definition, returns the label name without the colon */
static char *process_label(const char *line)
{
    static char buf[MAX_LINE_LEN];
    size_t len = strlen(line);
    if (len <= 1)
        return NULL;            /* Empty label */
    memcpy(buf, line, len - 1); /* Copy without colon */
    buf[len - 1] = '\0';
    return trim(buf);
}

/* ---- File Reading and First Pass ---- */

/* Read all lines from a file into the global lines array.
   Returns number of lines read.
*/
static size_t read_all_lines(const char *filename)
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

/* Simulate instruction emission size.
   Supported:
     move <reg>, <immediate> -> 5 bytes if immediate fits in 32 bits, else 10 bytes.
     call -> 2 bytes.
     jump -> 5 bytes
     jumplt/jumpgt/jumpeq -> 6 bytes
     comp <reg>, <reg/immediate> -> 3-7 bytes
     add <reg>, <reg/immediate> -> 3-7 bytes
*/
static size_t simulate_instruction(const char *line)
{
    char buf[MAX_LINE_LEN];
    strncpy(buf, line, MAX_LINE_LEN - 1);
    buf[MAX_LINE_LEN - 1] = '\0';
    char *trimmed = trim(buf);
    if (strncmp(trimmed, "move", 4) == 0)
    {
        /* Format:
           move <register>, <immediate>
           move <register>, [<symbol>]  ; load from memory
           move [<symbol>], <register>  ; store to memory
        */
        char *token = strtok(trimmed, " ,\t"); /* "move" */
        token = strtok(NULL, " ,\t");          /* destination */
        if (!token)
            return 0;
        char dest[MAX_LINE_LEN];
        strncpy(dest, token, sizeof(dest) - 1);
        dest[sizeof(dest) - 1] = '\0';

        token = strtok(NULL, " ,\t"); /* source */
        if (!token)
            return 0;

        if (is_memory_ref(dest) || is_memory_ref(token))
        {
            /* Memory operations:
               - REX.W prefix (1 byte)
               - Opcode (1 byte)
               - ModR/M (1 byte)
               - 32-bit displacement (4 bytes)
            */
            return 7;
        }
        else if (is_numeric(token))
        {
            uint64_t val = strtoull(token, NULL, 0);
            if (val <= 0xffffffffULL)
            {
                /* 32-bit immediate:
                   - REX.W prefix (1 byte)
                   - Opcode (1 byte)
                   - ModR/M (1 byte)
                   - 32-bit immediate (4 bytes)
                */
                return 7;
            }
            else
            {
                /* 64-bit immediate:
                   - REX.W prefix (1 byte)
                   - Opcode (1 byte)
                   - 64-bit immediate (8 bytes)
                */
                return 10;
            }
        }
        else
        {
            /* Symbol address (using lea):
               - REX.W prefix (1 byte)
               - Opcode (1 byte)
               - ModR/M (1 byte)
               - 32-bit displacement (4 bytes)
            */
            return 7;
        }
    }
    else if (strncmp(trimmed, "call", 4) == 0)
    {
        return 2;
    }
    else if (strncmp(trimmed, "jumplt", 6) == 0 ||
             strncmp(trimmed, "jumpgt", 6) == 0 ||
             strncmp(trimmed, "jumpeq", 6) == 0)
    {
        /* Conditional jump:
           - Opcode (2 bytes)
           - 32-bit relative offset (4 bytes)
        */
        return 6;
    }
    else if (strncmp(trimmed, "jump", 4) == 0)
    {
        /* jump instruction:
           - Opcode (1 byte)
           - 32-bit relative offset (4 bytes)
        */
        return 5;
    }
    else if (strncmp(trimmed, "comp", 4) == 0 || strncmp(trimmed, "add", 3) == 0)
    {
        /* Format: comp/add <reg>, <reg/immediate> */
        char *token = strtok(trimmed, " ,\t"); /* "comp" or "add" */
        if (!token)
            return 0;
        char *first = strtok(NULL, " ,\t"); /* first operand */
        if (!first)
            return 0;

        char *second = strtok(NULL, " ,\t"); /* second operand */
        if (!second)
            return 0;

        if (is_numeric(second))
        {
            /* Operation with immediate:
               - REX.W (1 byte)
               - Opcode (1 byte)
               - ModR/M (1 byte)
               - 32-bit immediate (4 bytes)
            */
            return 7;
        }
        else
        {
            /* Register-register:
               - REX.W (1 byte)
               - Opcode (1 byte)
               - ModR/M (1 byte)
            */
            return 3;
        }
    }
    return 0;
}

/* First pass: simulate code emission and collect data directives.
   Returns total simulated code size.
*/
static size_t first_pass(size_t lineCount)
{
    size_t codeSize = 0;
    for (size_t i = 0; i < lineCount; i++)
    {
        char *trimmed = trim(lines[i]);
        if (trimmed[0] == '\0' || trimmed[0] == '#')
            continue;
        if (strncmp(trimmed, "data", 4) == 0)
        {
            /* Process data directive */
            process_data_directive(trimmed);
        }
        else if (is_label(trimmed))
        {
            /* Process label definition */
            char *label = process_label(trimmed);
            if (label)
            {
                /* Store label position in symbol table */
                add_symbol(label, BASE_ADDR + CODE_OFFSET + codeSize);
            }
        }
        else
        {
            /* Instruction line */
            size_t sz = simulate_instruction(trimmed);
            codeSize += sz;
        }
    }
    return codeSize;
}

/* ---- Instruction Emission ---- */

/* Register mapping for move instruction.
   The opcode is 0xB8 + offset; for 32-bit version no REX prefix.
*/
static struct
{
    const char *name;
    uint8_t offset;
} regmap[] = {
    {"rax", 0x00},
    {"rcx", 0x01},
    {"rdx", 0x02},
    {"rbx", 0x03},
    {"rsi", 0x06},
    {"rdi", 0x07},
    {NULL, 0}};

/* Get register opcode for move. */
static uint8_t get_register_opcode(const char *reg)
{
    for (int i = 0; regmap[i].name != NULL; i++)
    {
        if (strcmp(reg, regmap[i].name) == 0)
            return regmap[i].offset;
    }
    fprintf(stderr, "Error: unknown register '%s'\n", reg);
    exit(1);
}

static void emit_instruction_line(CodeBuffer *codeBuf, const char *line)
{
    char buf[MAX_LINE_LEN];
    strncpy(buf, line, MAX_LINE_LEN - 1);
    buf[MAX_LINE_LEN - 1] = '\0';
    char *trimmed = trim(buf);
    if (trimmed[0] == '\0' || trimmed[0] == '#')
        return;
    if (strncmp(trimmed, "data", 4) == 0)
        return; /* skip data directives */
    if (is_label(trimmed))
        return; /* skip label definitions */
    if (strncmp(trimmed, "move", 4) == 0)
    {
        /* Format:
           move <register>, <immediate>
           move <register>, [<symbol>]  ; load from memory
           move [<symbol>], <register>  ; store to memory
        */
        char *token = strtok(trimmed, " ,\t"); /* "move" */
        token = strtok(NULL, " ,\t");          /* destination */
        if (!token)
        {
            fprintf(stderr, "Error: expected destination after 'move'\n");
            exit(1);
        }
        char dest[MAX_LINE_LEN];
        strncpy(dest, token, sizeof(dest) - 1);
        dest[sizeof(dest) - 1] = '\0';

        token = strtok(NULL, " ,\t"); /* source */
        if (!token)
        {
            fprintf(stderr, "Error: expected source after destination\n");
            exit(1);
        }

        if (is_memory_ref(dest))
        {
            /* Store to memory: move [symbol], reg */
            char *symbol = extract_memory_ref(dest);
            uint64_t addr = lookup_symbol(symbol);
            uint8_t reg = get_register_opcode(token);

            /* Calculate relative offset from next instruction */
            int64_t rel_addr = addr - (BASE_ADDR + CODE_OFFSET + codeBuf->size + 7);

            /* mov [rip + disp32], reg */
            codeBuf->bytes[codeBuf->size++] = 0x48;              /* REX.W */
            codeBuf->bytes[codeBuf->size++] = 0x89;              /* mov r/m64, r64 */
            codeBuf->bytes[codeBuf->size++] = (reg << 3) | 0x05; /* ModR/M: RIP-relative */
            /* 32-bit relative address */
            for (int i = 0; i < 4; i++)
                codeBuf->bytes[codeBuf->size++] = (uint8_t)((rel_addr >> (8 * i)) & 0xff);
        }
        else if (is_memory_ref(token))
        {
            /* Load from memory: move reg, [symbol] */
            char *symbol = extract_memory_ref(token);
            uint64_t addr = lookup_symbol(symbol);
            uint8_t reg = get_register_opcode(dest);

            /* Calculate relative offset from next instruction */
            int64_t rel_addr = addr - (BASE_ADDR + CODE_OFFSET + codeBuf->size + 7);

            /* mov reg, [rip + disp32] */
            codeBuf->bytes[codeBuf->size++] = 0x48;              /* REX.W */
            codeBuf->bytes[codeBuf->size++] = 0x8B;              /* mov r64, r/m64 */
            codeBuf->bytes[codeBuf->size++] = (reg << 3) | 0x05; /* ModR/M: RIP-relative */
            /* 32-bit relative address */
            for (int i = 0; i < 4; i++)
                codeBuf->bytes[codeBuf->size++] = (uint8_t)((rel_addr >> (8 * i)) & 0xff);
        }
        else if (is_numeric(token))
        {
            /* Move immediate to register */
            uint64_t val = strtoull(token, NULL, 0);
            uint8_t reg = get_register_opcode(dest);

            /* For small values, use 32-bit move which zero-extends to 64 bits */
            if (val <= 0xffffffffULL)
            {
                codeBuf->bytes[codeBuf->size++] = 0x48;       /* REX.W */
                codeBuf->bytes[codeBuf->size++] = 0xC7;       /* mov r/m64, imm32 */
                codeBuf->bytes[codeBuf->size++] = 0xC0 | reg; /* ModR/M: register direct */
                for (int i = 0; i < 4; i++)
                    codeBuf->bytes[codeBuf->size++] = (uint8_t)((val >> (8 * i)) & 0xff);
            }
            else
            {
                /* For large values, use movabs */
                codeBuf->bytes[codeBuf->size++] = 0x48;       /* REX.W */
                codeBuf->bytes[codeBuf->size++] = 0xB8 + reg; /* movabs r64, imm64 */
                for (int i = 0; i < 8; i++)
                    codeBuf->bytes[codeBuf->size++] = (uint8_t)((val >> (8 * i)) & 0xff);
            }
        }
        else
        {
            /* Move symbol address to register */
            uint64_t symVal = lookup_symbol(token);
            uint8_t reg = get_register_opcode(dest);

            /* Use lea for symbol addresses */
            codeBuf->bytes[codeBuf->size++] = 0x48;              /* REX.W */
            codeBuf->bytes[codeBuf->size++] = 0x8D;              /* lea r64, m */
            codeBuf->bytes[codeBuf->size++] = (reg << 3) | 0x05; /* ModR/M: RIP-relative */

            /* Calculate relative offset from next instruction */
            int64_t rel_addr = symVal - (BASE_ADDR + CODE_OFFSET + codeBuf->size + 4);
            for (int i = 0; i < 4; i++)
                codeBuf->bytes[codeBuf->size++] = (uint8_t)((rel_addr >> (8 * i)) & 0xff);
        }
    }
    else if (strncmp(trimmed, "call", 4) == 0)
    {
        /* syscall opcode */
        codeBuf->bytes[codeBuf->size++] = 0x0F;
        codeBuf->bytes[codeBuf->size++] = 0x05;
    }
    else if (strncmp(trimmed, "jumplt", 6) == 0 ||
             strncmp(trimmed, "jumpgt", 6) == 0 ||
             strncmp(trimmed, "jumpeq", 6) == 0)
    {
        /* Get label name */
        char *label = strtok(trimmed + 6, " \t");
        if (!label)
        {
            fprintf(stderr, "Error: conditional jump instruction requires a label\n");
            exit(1);
        }
        label = trim(label);

        /* Look up label address */
        uint64_t target = lookup_symbol(label);

        /* Calculate relative offset from next instruction */
        int64_t rel_addr = target - (BASE_ADDR + CODE_OFFSET + codeBuf->size + 6);

        /* Check if offset fits in 32 bits */
        if (rel_addr < INT32_MIN || rel_addr > INT32_MAX)
        {
            fprintf(stderr, "Error: jump target too far\n");
            exit(1);
        }

        /* Emit conditional jump instruction based on type:
           jumplt: 0x0F 0x8C (jl)
           jumpgt: 0x0F 0x8F (jg)
           jumpeq: 0x0F 0x84 (je)
        */
        codeBuf->bytes[codeBuf->size++] = 0x0F;
        if (strncmp(trimmed, "jumplt", 6) == 0)
            codeBuf->bytes[codeBuf->size++] = 0x8C;
        else if (strncmp(trimmed, "jumpgt", 6) == 0)
            codeBuf->bytes[codeBuf->size++] = 0x8F;
        else /* jumpeq */
            codeBuf->bytes[codeBuf->size++] = 0x84;

        /* 32-bit relative offset */
        for (int i = 0; i < 4; i++)
            codeBuf->bytes[codeBuf->size++] = (uint8_t)((rel_addr >> (8 * i)) & 0xff);
    }
    else if (strncmp(trimmed, "jump", 4) == 0)
    {
        /* Get label name */
        char *label = strtok(trimmed + 4, " \t");
        if (!label)
        {
            fprintf(stderr, "Error: jump instruction requires a label\n");
            exit(1);
        }
        label = trim(label);

        /* Look up label address */
        uint64_t target = lookup_symbol(label);

        /* Calculate relative offset from next instruction */
        int64_t rel_addr = target - (BASE_ADDR + CODE_OFFSET + codeBuf->size + 5);

        /* Check if offset fits in 32 bits */
        if (rel_addr < INT32_MIN || rel_addr > INT32_MAX)
        {
            fprintf(stderr, "Error: jump target too far\n");
            exit(1);
        }

        /* Emit jump instruction:
           0xE9 <32-bit relative offset>
        */
        codeBuf->bytes[codeBuf->size++] = 0xE9;
        for (int i = 0; i < 4; i++)
            codeBuf->bytes[codeBuf->size++] = (uint8_t)((rel_addr >> (8 * i)) & 0xff);
    }
    else if (strncmp(trimmed, "comp", 4) == 0 || strncmp(trimmed, "add", 3) == 0)
    {
        /* Format: comp/add <reg>, <reg/immediate> */
        char *token = strtok(trimmed, " ,\t");
        if (!token)
        {
            fprintf(stderr, "Error: expected instruction\n");
            exit(1);
        }

        char *first = strtok(NULL, " ,\t"); /* first operand */
        if (!first)
        {
            fprintf(stderr, "Error: expected first operand\n");
            exit(1);
        }

        char *second = strtok(NULL, " ,\t"); /* second operand */
        if (!second)
        {
            fprintf(stderr, "Error: expected second operand\n");
            exit(1);
        }

        uint8_t reg = get_register_opcode(first);

        if (is_numeric(second))
        {
            uint64_t val = strtoull(second, NULL, 0);
            if (strncmp(trimmed, "comp", 4) == 0)
            {
                /* Compare with immediate */
                codeBuf->bytes[codeBuf->size++] = 0x48;       /* REX.W */
                codeBuf->bytes[codeBuf->size++] = 0x81;       /* cmp r/m64, imm32 */
                codeBuf->bytes[codeBuf->size++] = 0xF8 | reg; /* ModR/M: register direct */
                /* 32-bit immediate */
                for (int i = 0; i < 4; i++)
                    codeBuf->bytes[codeBuf->size++] = (uint8_t)((val >> (8 * i)) & 0xff);
            }
            else /* add */
            {
                /* Add immediate */
                codeBuf->bytes[codeBuf->size++] = 0x48;       /* REX.W */
                codeBuf->bytes[codeBuf->size++] = 0x81;       /* add r/m64, imm32 */
                codeBuf->bytes[codeBuf->size++] = 0xC0 | reg; /* ModR/M: register direct */
                /* 32-bit immediate */
                for (int i = 0; i < 4; i++)
                    codeBuf->bytes[codeBuf->size++] = (uint8_t)((val >> (8 * i)) & 0xff);
            }
        }
        else
        {
            /* Register-register operation */
            uint8_t reg2 = get_register_opcode(second);
            codeBuf->bytes[codeBuf->size++] = 0x48; /* REX.W */
            if (strncmp(trimmed, "comp", 4) == 0)
            {
                codeBuf->bytes[codeBuf->size++] = 0x39;                     /* cmp r/m64, r64 */
                codeBuf->bytes[codeBuf->size++] = 0xC0 | (reg2 << 3) | reg; /* ModR/M */
            }
            else /* add */
            {
                codeBuf->bytes[codeBuf->size++] = 0x01;                     /* add r/m64, r64 */
                codeBuf->bytes[codeBuf->size++] = 0xC0 | (reg2 << 3) | reg; /* ModR/M */
            }
        }
    }
    else
    {
        fprintf(stderr, "Error: unknown instruction '%s'\n", trimmed);
        exit(1);
    }
}

/* ---- ELF File Writing ---- */

static void write_elf(const char *outfile, CodeBuffer *codeBuf, DataBuffer *dataBuf)
{
    size_t file_size = CODE_OFFSET + codeBuf->size + dataBuf->size;
    uint8_t *file_buf = calloc(1, file_size);
    if (!file_buf)
    {
        perror("calloc");
        exit(1);
    }
    uint8_t *p = file_buf;
    /* Construct ELF header */
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
    Elf64_Ehdr eh = {0};
    memcpy(eh.e_ident, "\177ELF\2\1\1\0", 8);
    eh.e_type = 2;       /* EXEC */
    eh.e_machine = 0x3E; /* AMD x86-64 */
    eh.e_version = 1;
    eh.e_entry = BASE_ADDR + CODE_OFFSET;
    eh.e_phoff = ELF_HEADER_SIZE;
    eh.e_shoff = 0;
    eh.e_flags = 0;
    eh.e_ehsize = ELF_HEADER_SIZE;
    eh.e_phentsize = PROGRAM_HEADER_SIZE;
    eh.e_phnum = 1;
    memcpy(p, &eh, ELF_HEADER_SIZE);

    p = file_buf + ELF_HEADER_SIZE;
    /* Construct Program Header */
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
    Elf64_Phdr ph = {0};
    ph.p_type = 1;                           /* PT_LOAD */
    ph.p_flags = 7; /* PF_R | PF_W | PF_X */ // Make segment readable, writable and executable
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

/* ---- Public Function: assemble() ---- */

/* The assemble() function processes the input assembly file and creates an output ELF binary. */
int assemble(const char *input_filename, const char *output_filename)
{
    /* Read all lines from input. */
    size_t lineCount = read_all_lines(input_filename);

    /* First pass: simulate code emission and collect data directives */
    size_t simulatedCodeSize = first_pass(lineCount);

    /* Data section begins at: BASE_ADDR + CODE_OFFSET + simulatedCodeSize */
    uint64_t dataBase = BASE_ADDR + CODE_OFFSET + simulatedCodeSize;

    DataBuffer dataBuf = {.size = 0};
    /* Process collected data directives: assign symbol addresses and emit data */
    for (size_t i = 0; i < dataDirCount; i++)
    {
        uint64_t addr = dataBase + dataBuf.size;
        add_symbol(dataDirectives[i].label, addr);

        if (dataDirectives[i].type == DATA_STRING)
        {
            char processed[MAX_LINE_LEN];
            process_escape_sequences(dataDirectives[i].data.literal, processed);
            size_t len = strlen(processed);             // Don't include null terminator in length for sys_write
            if (dataBuf.size + len + 1 > MAX_DATA_SIZE) // But allocate space for it
            {
                fprintf(stderr, "Error: data buffer overflow\n");
                exit(1);
            }
            memcpy(dataBuf.bytes + dataBuf.size, processed, len + 1); // Copy with null terminator
            dataBuf.size += len + 1;
        }
        else /* DATA_BUFFER */
        {
            size_t size = dataDirectives[i].data.size;
            if (dataBuf.size + size > MAX_DATA_SIZE)
            {
                fprintf(stderr, "Error: data buffer overflow\n");
                exit(1);
            }
            /* Zero-initialize the buffer */
            memset(dataBuf.bytes + dataBuf.size, 0, size);
            dataBuf.size += size;
        }
    }

    CodeBuffer codeBuf = {.size = 0};
    /* Second pass: emit instructions (ignore data directives) */
    for (size_t i = 0; i < lineCount; i++)
    {
        char *trimmed = trim(lines[i]);
        if (trimmed[0] == '\0' || trimmed[0] == '#' ||
            strncmp(trimmed, "data", 4) == 0)
            continue;
        emit_instruction_line(&codeBuf, trimmed);
    }

    write_elf(output_filename, &codeBuf, &dataBuf);
    return 0;
}
