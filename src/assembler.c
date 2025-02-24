#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include "assembler.h"
#include "binary_writer.h"

/* ---- Internal Constants and Structures ---- */

/* Data directive structure for storing parsed data info. */
typedef struct
{
    char label[32];
    enum
    {
        DATA_STRING, /* String literal */
        DATA_BUFFER, /* Raw buffer of given size */
        DATA_FILE,   /* File inclusion */
        DATA_RAW,    /* Raw numeric value */
    } type;
    union
    {
        char literal[MAX_LINE_LEN];
        size_t size;
        char filename[MAX_LINE_LEN];
        uint64_t value;  /* For raw numeric values */
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
   - data <label> file <filename>
   - data <label> 0x1234ABCD    (hex)
   - data <label> 0b10110011    (binary)
   - data <label> 123           (decimal)
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
    else if (strncmp(value, "file", 4) == 0)
    {
        /* File inclusion */
        dataDirectives[dataDirCount].type = DATA_FILE;
        
        /* Get filename after "file" keyword */
        char *filename = value + 4;
        filename = trim(filename);
        if (!filename || !*filename)
        {
            fprintf(stderr, "Error: file directive requires a filename\n");
            exit(1);
        }
        
        strncpy(dataDirectives[dataDirCount].data.filename, filename, 
                sizeof(dataDirectives[dataDirCount].data.filename) - 1);
        dataDirectives[dataDirCount].data.filename[
            sizeof(dataDirectives[dataDirCount].data.filename) - 1] = '\0';
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
    else if (value[0] == '0' && (value[1] == 'x' || value[1] == 'X' || value[1] == 'b' || value[1] == 'B'))
    {
        /* Hex or binary value */
        dataDirectives[dataDirCount].type = DATA_RAW;
        char *endptr;
        if (value[1] == 'x' || value[1] == 'X')
        {
            dataDirectives[dataDirCount].data.value = strtoull(value, &endptr, 16);
        }
        else /* binary */
        {
            /* Skip 0b prefix */
            value += 2;
            dataDirectives[dataDirCount].data.value = 0;
            while (*value == '0' || *value == '1')
            {
                dataDirectives[dataDirCount].data.value = (dataDirectives[dataDirCount].data.value << 1) | (*value - '0');
                value++;
            }
            if (*value && !isspace((unsigned char)*value))
            {
                fprintf(stderr, "Error: invalid binary number\n");
                exit(1);
            }
        }
    }
    else if (isdigit((unsigned char)value[0]) || value[0] == '-')
    {
        /* Decimal value */
        dataDirectives[dataDirCount].type = DATA_RAW;
        char *endptr;
        dataDirectives[dataDirCount].data.value = strtoull(value, &endptr, 10);
        if (*endptr && !isspace((unsigned char)*endptr))
        {
            fprintf(stderr, "Error: invalid decimal number\n");
            exit(1);
        }
    }
    else
    {
        fprintf(stderr, "Error: data directive requires a string literal, 'size <number>', or a numeric value (decimal, 0x... for hex, 0b... for binary)\n");
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

/* Ensure there's enough space in a buffer for additional bytes */
static void ensure_buffer_capacity(CodeBuffer* codeBuf, size_t additional_bytes) {
    if (codeBuf->size + additional_bytes > codeBuf->capacity) {
        size_t new_capacity = codeBuf->capacity * 2;
        if (new_capacity < codeBuf->size + additional_bytes) {
            new_capacity = codeBuf->size + additional_bytes + 1024; // Add some extra space
        }
        
        uint8_t* new_buffer = (uint8_t*)realloc(codeBuf->bytes, new_capacity);
        if (!new_buffer) {
            perror("realloc for code buffer");
            exit(1);
        }
        
        codeBuf->bytes = new_buffer;
        codeBuf->capacity = new_capacity;
    }
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
            ensure_buffer_capacity(codeBuf, 7); // Need 7 bytes
            
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
            ensure_buffer_capacity(codeBuf, 7); // Need 7 bytes
            
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
                ensure_buffer_capacity(codeBuf, 7); // Need 7 bytes
                
                codeBuf->bytes[codeBuf->size++] = 0x48;       /* REX.W */
                codeBuf->bytes[codeBuf->size++] = 0xC7;       /* mov r/m64, imm32 */
                codeBuf->bytes[codeBuf->size++] = 0xC0 | reg; /* ModR/M: register direct */
                for (int i = 0; i < 4; i++)
                    codeBuf->bytes[codeBuf->size++] = (uint8_t)((val >> (8 * i)) & 0xff);
            }
            else
            {
                ensure_buffer_capacity(codeBuf, 10); // Need 10 bytes
                
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
            ensure_buffer_capacity(codeBuf, 7); // Need 7 bytes
            
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
        ensure_buffer_capacity(codeBuf, 2); // Need 2 bytes
        
        /* syscall opcode */
        codeBuf->bytes[codeBuf->size++] = 0x0F;
        codeBuf->bytes[codeBuf->size++] = 0x05;
    }
    else if (strncmp(trimmed, "jumplt", 6) == 0 ||
             strncmp(trimmed, "jumpgt", 6) == 0 ||
             strncmp(trimmed, "jumpeq", 6) == 0)
    {
        ensure_buffer_capacity(codeBuf, 6); // Need 6 bytes
        
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
        ensure_buffer_capacity(codeBuf, 5); // Need 5 bytes
        
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
            ensure_buffer_capacity(codeBuf, 7); // Need up to 7 bytes
            
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
            ensure_buffer_capacity(codeBuf, 3); // Need 3 bytes
            
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

/* Process data directives and copy data to the data buffer */
static void process_data_buffer(DataBuffer *dataBuf, uint64_t dataBase)
{
    /* Process collected data directives: assign symbol addresses and emit data */
    for (size_t i = 0; i < dataDirCount; i++)
    {
        uint64_t addr = dataBase + dataBuf->size;
        add_symbol(dataDirectives[i].label, addr);

        if (dataDirectives[i].type == DATA_STRING)
        {
            char processed[MAX_LINE_LEN];
            process_escape_sequences(dataDirectives[i].data.literal, processed);
            size_t len = strlen(processed) + 1; // Include null terminator
            
            ensure_buffer_capacity(dataBuf, len);
            
            memcpy(dataBuf->bytes + dataBuf->size, processed, len);
            dataBuf->size += len;
        }
        else if (dataDirectives[i].type == DATA_BUFFER)
        {
            size_t size = dataDirectives[i].data.size;
            
            ensure_buffer_capacity(dataBuf, size);
            
            /* Zero-initialize the buffer */
            memset(dataBuf->bytes + dataBuf->size, 0, size);
            dataBuf->size += size;
        }
        else if (dataDirectives[i].type == DATA_FILE)
        {
            /* Read file contents */
            FILE *fp = fopen(dataDirectives[i].data.filename, "rb");
            if (!fp)
            {
                fprintf(stderr, "Error: cannot open file '%s'\n", 
                        dataDirectives[i].data.filename);
                exit(1);
            }
            
            /* Get file size */
            fseek(fp, 0, SEEK_END);
            size_t fileSize = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            
            ensure_buffer_capacity(dataBuf, fileSize);
            
            /* Read file into data buffer */
            if (fread(dataBuf->bytes + dataBuf->size, 1, fileSize, fp) != fileSize)
            {
                fprintf(stderr, "Error: failed to read file '%s'\n",
                        dataDirectives[i].data.filename);
                fclose(fp);
                exit(1);
            }
            fclose(fp);
            dataBuf->size += fileSize;
        }
        else /* DATA_RAW */
        {
            size_t size = sizeof(dataDirectives[i].data.value);
            
            ensure_buffer_capacity(dataBuf, size);
            
            memcpy(dataBuf->bytes + dataBuf->size, &dataDirectives[i].data.value, size);
            dataBuf->size += size;
        }
    }
}

/* ---- Main Assembly Function ---- */

/* Convenience wrapper for ELF output */
int assemble_to_elf(const char* input_filename, const char* output_filename) {
    AssemblerOptions options = {
        .input_filename = input_filename,
        .output_filename = output_filename,
        .writer = write_elf_file,
        .verbose = 0
    };
    return assemble(&options);
}

/* The main assembly function that processes the input assembly file,
   generates code and data, and calls the binary writer function. */
int assemble(const AssemblerOptions* options)
{
    /* Reset global state for multiple invocations */
    symbolCount = 0;
    dataDirCount = 0;
    
    /* Read all lines from input. */
    size_t lineCount = read_all_lines(options->input_filename);

    /* First pass: simulate code emission and collect data directives */
    size_t simulatedCodeSize = first_pass(lineCount);

    /* Data section begins after code section */
    uint64_t dataBase = BASE_ADDR + CODE_OFFSET + simulatedCodeSize;
    uint64_t entry_point = BASE_ADDR + CODE_OFFSET;

    /* Initialize dynamically allocated buffers */
    CodeBuffer codeBuf;
    DataBuffer dataBuf;
    init_code_buffer(&codeBuf, simulatedCodeSize > 0 ? simulatedCodeSize : 1024);
    init_data_buffer(&dataBuf, 1024);  // Start with a reasonable default size
    
    /* Process data directives and fill data buffer */
    process_data_buffer(&dataBuf, dataBase);
    
    /* Second pass: emit instructions (ignore data directives) */
    for (size_t i = 0; i < lineCount; i++)
    {
        char *trimmed = trim(lines[i]);
        if (trimmed[0] == '\0' || trimmed[0] == '#' ||
            strncmp(trimmed, "data", 4) == 0)
            continue;
        emit_instruction_line(&codeBuf, trimmed);
    }

    /* Call the binary writer function */
    int result = options->writer(options->output_filename, &codeBuf, &dataBuf, entry_point);
    
    /* Clean up */
    free_code_buffer(&codeBuf);
    free_data_buffer(&dataBuf);
    
    return result;
}
