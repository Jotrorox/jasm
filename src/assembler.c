#include "assembler.h"
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "binary_writer.h"
#include "color_utils.h" /* Include our new color utilities */
#include "syntax.h"      /* Include the new syntax module */

/* ---- Internal Constants and Structures ---- */

/* Internal arrays for symbol and data directive storage. */
static char lines[MAX_LINES][MAX_LINE_LEN];
static SyntaxDataDirective dataDirectives[MAX_SYMBOLS];
static size_t dataDirCount = 0;

/* Symbol structure. */
typedef struct {
    char name[32];
    uint64_t value;
} Symbol;

/* Internal symbol table. */
static Symbol symbols[MAX_SYMBOLS];
static size_t symbolCount = 0;

/* ---- Utility Functions ---- */

/* Add a symbol to the symbol table. */
static void add_symbol(const char *name, uint64_t value)
{
    if (symbolCount >= MAX_SYMBOLS) {
        color_error("symbol table overflow");
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
    for (size_t i = 0; i < symbolCount; i++) {
        if (strcmp(symbols[i].name, name) == 0)
            return symbols[i].value;
    }
    color_error("unknown symbol '%s'", name);
    exit(1);
}

/* Check if a line is a label definition (ends with ':') */
static int is_label(const char *line) __attribute__((unused));
static int is_label(const char *line)
{
    size_t len = strlen(line);
    return len > 0 && line[len - 1] == ':';
}

/* Process label definition, returns the label name without the colon */
static char *process_label(const char *line) __attribute__((unused));
static char *process_label(const char *line)
{
    static char buf[MAX_LINE_LEN];
    size_t len = strlen(line);
    if (len <= 1)
        return NULL;            /* Empty label */
    memcpy(buf, line, len - 1); /* Copy without colon */
    buf[len - 1] = '\0';
    return syntax_trim(buf);
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
static void process_data_directive(char *trimmed) __attribute__((unused));
static void process_data_directive(char *trimmed)
{
    char *p = trimmed + 4;
    p = syntax_trim(p);
    char *label = strtok(p, " \t");
    if (!label) {
        color_error("data directive requires a label");
        exit(1);
    }
    char *value = strtok(NULL, "\n");
    if (!value) {
        color_error("data directive missing value");
        exit(1);
    }
    value = syntax_trim(value);

    /* Save the directive for later processing */
    strncpy(
        dataDirectives[dataDirCount].label, label, sizeof(dataDirectives[dataDirCount].label) - 1);
    dataDirectives[dataDirCount].label[sizeof(dataDirectives[dataDirCount].label) - 1] = '\0';

    if (value[0] == '"') {
        /* String literal */
        dataDirectives[dataDirCount].type = DATA_STRING;
        value++; /* skip opening quote */
        char *endQuote = strchr(value, '"');
        if (!endQuote) {
            color_error("missing closing quote in data directive");
            exit(1);
        }
        *endQuote = '\0';
        strncpy(dataDirectives[dataDirCount].data.literal,
                value,
                sizeof(dataDirectives[dataDirCount].data.literal) - 1);
        dataDirectives[dataDirCount]
            .data.literal[sizeof(dataDirectives[dataDirCount].data.literal) - 1] = '\0';
    } else if (strncmp(value, "file", 4) == 0) {
        /* File inclusion */
        dataDirectives[dataDirCount].type = DATA_FILE;

        /* Get filename after "file" keyword */
        char *filename = value + 4;
        filename = syntax_trim(filename);
        if (!filename || !*filename) {
            color_error("file directive requires a filename");
            exit(1);
        }

        strncpy(dataDirectives[dataDirCount].data.filename,
                filename,
                sizeof(dataDirectives[dataDirCount].data.filename) - 1);
        dataDirectives[dataDirCount]
            .data.filename[sizeof(dataDirectives[dataDirCount].data.filename) - 1] = '\0';
    } else if (strncmp(value, "size", 4) == 0) {
        /* Buffer allocation */
        dataDirectives[dataDirCount].type = DATA_BUFFER;
        char *sizeStr = value + 4;
        sizeStr = syntax_trim(sizeStr);
        if (!syntax_is_numeric(sizeStr)) {
            color_error("size must be a number");
            exit(1);
        }
        dataDirectives[dataDirCount].data.size = strtoull(sizeStr, NULL, 0);
    } else if (value[0] == '0'
               && (value[1] == 'x' || value[1] == 'X' || value[1] == 'b' || value[1] == 'B')) {
        /* Hex or binary value */
        dataDirectives[dataDirCount].type = DATA_RAW;
        char *endptr;
        if (value[1] == 'x' || value[1] == 'X') {
            dataDirectives[dataDirCount].data.value = strtoull(value, &endptr, 16);
        } else /* binary */
        {
            /* Skip 0b prefix */
            value += 2;
            dataDirectives[dataDirCount].data.value = 0;
            while (*value == '0' || *value == '1') {
                dataDirectives[dataDirCount].data.value =
                    (dataDirectives[dataDirCount].data.value << 1) | (*value - '0');
                value++;
            }
            if (*value && !isspace((unsigned char)*value)) {
                color_error("invalid binary number");
                exit(1);
            }
        }
    } else if (isdigit((unsigned char)value[0]) || value[0] == '-') {
        /* Decimal value */
        dataDirectives[dataDirCount].type = DATA_RAW;
        char *endptr;
        dataDirectives[dataDirCount].data.value = strtoull(value, &endptr, 10);
        if (*endptr && !isspace((unsigned char)*endptr)) {
            color_error("invalid decimal number");
            exit(1);
        }
    } else {
        color_error("data directive requires a string literal, 'size <number>', or "
                    "a numeric value (decimal, 0x... for hex, 0b... for binary)");
        exit(1);
    }
    dataDirCount++;
}

/* ---- File Reading and First Pass ---- */

/* Read all lines from a file into the global lines array.
   Returns number of lines read.
*/
static size_t read_all_lines(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        color_error("Failed to open file: %s", filename);
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
     move <reg>, <immediate> -> 5 bytes if immediate fits in 32 bits, else 10
   bytes. call -> 2 bytes. jump -> 5 bytes jumplt/jumpgt/jumpeq -> 6 bytes comp
   <reg>, <reg/immediate> -> 3-7 bytes add <reg>, <reg/immediate> -> 3-7 bytes
*/
static size_t simulate_instruction(const char *line)
{
    char buf[MAX_LINE_LEN];
    strncpy(buf, line, MAX_LINE_LEN - 1);
    buf[MAX_LINE_LEN - 1] = '\0';
    char *trimmed = syntax_trim(buf);

    InstructionType instrType = syntax_get_instruction_type(trimmed);

    switch (instrType) {
        case INSTR_MOVE: {
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

            if (syntax_is_memory_reference(dest) || syntax_is_memory_reference(token)) {
                /* Memory operations:
                   - REX.W prefix (1 byte)
                   - Opcode (1 byte)
                   - ModR/M (1 byte)
                   - 32-bit displacement (4 bytes)
                */
                return 7;
            } else if (syntax_is_numeric(token)) {
                uint64_t val = strtoull(token, NULL, 0);
                if (val <= 0xffffffffULL) {
                    /* 32-bit immediate:
                       - REX.W prefix (1 byte)
                       - Opcode (1 byte)
                       - ModR/M (1 byte)
                       - 32-bit immediate (4 bytes)
                    */
                    return 7;
                } else {
                    /* 64-bit immediate:
                       - REX.W prefix (1 byte)
                       - Opcode (1 byte)
                       - 64-bit immediate (8 bytes)
                    */
                    return 10;
                }
            } else {
                /* Symbol address (using lea):
                   - REX.W prefix (1 byte)
                   - Opcode (1 byte)
                   - ModR/M (1 byte)
                   - 32-bit displacement (4 bytes)
                */
                return 7;
            }
            break;
        }

        case INSTR_CALL:
            return 2;

        case INSTR_JUMPLT:
        case INSTR_JUMPGT:
        case INSTR_JUMPEQ:
            /* Conditional jump:
               - Opcode (2 bytes)
               - 32-bit relative offset (4 bytes)
            */
            return 6;

        case INSTR_JUMP:
            /* jump instruction:
               - Opcode (1 byte)
               - 32-bit relative offset (4 bytes)
            */
            return 5;

        case INSTR_COMP:
        case INSTR_ADD: {
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

            if (syntax_is_numeric(second)) {
                /* Operation with immediate:
                   - REX.W (1 byte)
                   - Opcode (1 byte)
                   - ModR/M (1 byte)
                   - 32-bit immediate (4 bytes)
                */
                return 7;
            } else {
                /* Register-register:
                   - REX.W (1 byte)
                   - Opcode (1 byte)
                   - ModR/M (1 byte)
                */
                return 3;
            }
            break;
        }

        default:
            return 0;
    }

    return 0;
}

/* First pass: simulate code emission and collect data directives.
   Returns total simulated code size.
*/
static size_t first_pass(size_t lineCount)
{
    size_t codeSize = 0;
    for (size_t i = 0; i < lineCount; i++) {
        char *trimmed = syntax_trim(lines[i]);
        if (trimmed[0] == '\0' || syntax_is_comment(trimmed))
            continue;
        if (syntax_is_data_directive(trimmed)) {
            /* Process data directive */
            if (dataDirCount >= MAX_SYMBOLS) {
                color_error("too many data directives");
                exit(1);
            }

            if (!syntax_process_data_directive(trimmed, &dataDirectives[dataDirCount])) {
                color_error("invalid data directive: %s", trimmed);
                exit(1);
            }

            dataDirCount++;
        } else if (syntax_is_label(trimmed)) {
            /* Process label definition */
            char *label = syntax_extract_label_name(trimmed);
            if (label) {
                /* Store label position in symbol table */
                add_symbol(label, BASE_ADDR + CODE_OFFSET + codeSize);
            }
        } else {
            /* Instruction line */
            size_t sz = simulate_instruction(trimmed);
            codeSize += sz;
        }
    }
    return codeSize;
}

/* ---- Instruction Emission ---- */

/* Ensure there's enough space in the code buffer for additional bytes */
static void ensure_code_buffer_capacity(CodeBuffer *codeBuf, size_t additional_bytes)
{
    ensure_buffer_capacity(codeBuf, additional_bytes);
}

/* Ensure there's enough space in a data buffer for additional bytes */
static void ensure_data_buffer_capacity(DataBuffer *dataBuf, size_t additional_bytes)
{
    ensure_buffer_capacity(dataBuf, additional_bytes);
}

static void emit_instruction_line(CodeBuffer *codeBuf, const char *line)
{
    char buf[MAX_LINE_LEN];
    strncpy(buf, line, MAX_LINE_LEN - 1);
    buf[MAX_LINE_LEN - 1] = '\0';
    char *trimmed = syntax_trim(buf);

    if (trimmed[0] == '\0' || syntax_is_comment(trimmed))
        return;
    if (syntax_is_data_directive(trimmed))
        return; /* skip data directives */
    if (syntax_is_label(trimmed))
        return; /* skip label definitions */

    InstructionType instrType = syntax_get_instruction_type(trimmed);

    switch (instrType) {
        case INSTR_MOVE: {
            /* Format:
               move <register>, <immediate>
               move <register>, [<symbol>]  ; load from memory
               move [<symbol>], <register>  ; store to memory
            */
            char *token = strtok(trimmed, " ,\t"); /* "move" */
            token = strtok(NULL, " ,\t");          /* destination */
            if (!token) {
                color_error("expected destination after 'move'");
                exit(1);
            }
            char dest[MAX_LINE_LEN];
            strncpy(dest, token, sizeof(dest) - 1);
            dest[sizeof(dest) - 1] = '\0';

            token = strtok(NULL, " ,\t"); /* source */
            if (!token) {
                color_error("expected source after destination");
                exit(1);
            }

            if (syntax_is_memory_reference(dest)) {
                /* Store to memory: move [symbol], reg */
                ensure_code_buffer_capacity(codeBuf, 7);  // Need 7 bytes

                char *symbol = syntax_extract_memory_reference(dest);
                uint64_t addr = lookup_symbol(symbol);
                uint8_t reg = syntax_get_register_code(token);

                if (reg == 0xFF) {
                    color_error("unknown register '%s'", token);
                    exit(1);
                }

                /* Calculate relative offset from next instruction */
                int64_t rel_addr = addr - (BASE_ADDR + CODE_OFFSET + codeBuf->size + 7);

                /* mov [rip + disp32], reg */
                codeBuf->bytes[codeBuf->size++] = 0x48;              /* REX.W */
                codeBuf->bytes[codeBuf->size++] = 0x89;              /* mov r/m64, r64 */
                codeBuf->bytes[codeBuf->size++] = (reg << 3) | 0x05; /* ModR/M: RIP-relative */
                /* 32-bit relative address */
                for (int i = 0; i < 4; i++)
                    codeBuf->bytes[codeBuf->size++] = (uint8_t)((rel_addr >> (8 * i)) & 0xff);
            } else if (syntax_is_memory_reference(token)) {
                /* Load from memory: move reg, [symbol] */
                ensure_code_buffer_capacity(codeBuf, 7);  // Need 7 bytes

                char *symbol = syntax_extract_memory_reference(token);
                uint64_t addr = lookup_symbol(symbol);
                uint8_t reg = syntax_get_register_code(dest);

                if (reg == 0xFF) {
                    color_error("unknown register '%s'", dest);
                    exit(1);
                }

                /* Calculate relative offset from next instruction */
                int64_t rel_addr = addr - (BASE_ADDR + CODE_OFFSET + codeBuf->size + 7);

                /* mov reg, [rip + disp32] */
                codeBuf->bytes[codeBuf->size++] = 0x48;              /* REX.W */
                codeBuf->bytes[codeBuf->size++] = 0x8B;              /* mov r64, r/m64 */
                codeBuf->bytes[codeBuf->size++] = (reg << 3) | 0x05; /* ModR/M: RIP-relative */
                /* 32-bit relative address */
                for (int i = 0; i < 4; i++)
                    codeBuf->bytes[codeBuf->size++] = (uint8_t)((rel_addr >> (8 * i)) & 0xff);
            } else if (syntax_is_numeric(token)) {
                /* Move immediate to register */
                uint64_t val = strtoull(token, NULL, 0);
                uint8_t reg = syntax_get_register_code(dest);

                if (reg == 0xFF) {
                    color_error("unknown register '%s'", dest);
                    exit(1);
                }

                /* For small values, use 32-bit move which zero-extends to 64 bits */
                if (val <= 0xffffffffULL) {
                    ensure_code_buffer_capacity(codeBuf, 7);  // Need 7 bytes

                    codeBuf->bytes[codeBuf->size++] = 0x48;       /* REX.W */
                    codeBuf->bytes[codeBuf->size++] = 0xC7;       /* mov r/m64, imm32 */
                    codeBuf->bytes[codeBuf->size++] = 0xC0 | reg; /* ModR/M: register direct */
                    for (int i = 0; i < 4; i++)
                        codeBuf->bytes[codeBuf->size++] = (uint8_t)((val >> (8 * i)) & 0xff);
                } else {
                    ensure_code_buffer_capacity(codeBuf, 10);  // Need 10 bytes

                    /* For large values, use movabs */
                    codeBuf->bytes[codeBuf->size++] = 0x48;       /* REX.W */
                    codeBuf->bytes[codeBuf->size++] = 0xB8 + reg; /* movabs r64, imm64 */
                    for (int i = 0; i < 8; i++)
                        codeBuf->bytes[codeBuf->size++] = (uint8_t)((val >> (8 * i)) & 0xff);
                }
            } else {
                /* Move symbol address to register */
                ensure_code_buffer_capacity(codeBuf, 7);  // Need 7 bytes

                uint64_t symVal = lookup_symbol(token);
                uint8_t reg = syntax_get_register_code(dest);

                if (reg == 0xFF) {
                    color_error("unknown register '%s'", dest);
                    exit(1);
                }

                /* Use lea for symbol addresses */
                codeBuf->bytes[codeBuf->size++] = 0x48;              /* REX.W */
                codeBuf->bytes[codeBuf->size++] = 0x8D;              /* lea r64, m */
                codeBuf->bytes[codeBuf->size++] = (reg << 3) | 0x05; /* ModR/M: RIP-relative */

                /* Calculate relative offset from next instruction */
                int64_t rel_addr = symVal - (BASE_ADDR + CODE_OFFSET + codeBuf->size + 4);
                for (int i = 0; i < 4; i++)
                    codeBuf->bytes[codeBuf->size++] = (uint8_t)((rel_addr >> (8 * i)) & 0xff);
            }
            break;
        }

        case INSTR_CALL: {
            ensure_code_buffer_capacity(codeBuf, 2);  // Need 2 bytes

            /* syscall opcode */
            codeBuf->bytes[codeBuf->size++] = 0x0F;
            codeBuf->bytes[codeBuf->size++] = 0x05;
            break;
        }

        case INSTR_JUMPLT:
        case INSTR_JUMPGT:
        case INSTR_JUMPEQ: {
            ensure_code_buffer_capacity(codeBuf, 6);  // Need 6 bytes

            /* Get label name */
            char *label = strtok(trimmed + strlen(syntax_instruction_to_string(instrType)), " \t");
            if (!label) {
                color_error("conditional jump instruction requires a label");
                exit(1);
            }
            label = syntax_trim(label);

            /* Look up label address */
            uint64_t target = lookup_symbol(label);

            /* Calculate relative offset from next instruction */
            int64_t rel_addr = target - (BASE_ADDR + CODE_OFFSET + codeBuf->size + 6);

            /* Check if offset fits in 32 bits */
            if (rel_addr < INT32_MIN || rel_addr > INT32_MAX) {
                color_error("jump target too far");
                exit(1);
            }

            /* Emit conditional jump instruction based on type */
            codeBuf->bytes[codeBuf->size++] = 0x0F;

            switch (instrType) {
                case INSTR_JUMPLT:
                    codeBuf->bytes[codeBuf->size++] = 0x8C; /* jl */
                    break;
                case INSTR_JUMPGT:
                    codeBuf->bytes[codeBuf->size++] = 0x8F; /* jg */
                    break;
                case INSTR_JUMPEQ:
                    codeBuf->bytes[codeBuf->size++] = 0x84; /* je */
                    break;
                default:
                    /* Should never reach here */
                    color_error("internal error: unknown jump type");
                    exit(1);
            }

            /* 32-bit relative offset */
            for (int i = 0; i < 4; i++)
                codeBuf->bytes[codeBuf->size++] = (uint8_t)((rel_addr >> (8 * i)) & 0xff);
            break;
        }

        case INSTR_JUMP: {
            ensure_code_buffer_capacity(codeBuf, 5);  // Need 5 bytes

            /* Get label name */
            char *label = strtok(trimmed + 4, " \t");
            if (!label) {
                color_error("jump instruction requires a label");
                exit(1);
            }
            label = syntax_trim(label);

            /* Look up label address */
            uint64_t target = lookup_symbol(label);

            /* Calculate relative offset from next instruction */
            int64_t rel_addr = target - (BASE_ADDR + CODE_OFFSET + codeBuf->size + 5);

            /* Check if offset fits in 32 bits */
            if (rel_addr < INT32_MIN || rel_addr > INT32_MAX) {
                color_error("jump target too far");
                exit(1);
            }

            /* Emit jump instruction:
               0xE9 <32-bit relative offset>
            */
            codeBuf->bytes[codeBuf->size++] = 0xE9;
            for (int i = 0; i < 4; i++)
                codeBuf->bytes[codeBuf->size++] = (uint8_t)((rel_addr >> (8 * i)) & 0xff);
            break;
        }

        case INSTR_COMP:
        case INSTR_ADD: {
            /* Format: comp/add <reg>, <reg/immediate> */
            char *token = strtok(trimmed, " ,\t");
            if (!token) {
                color_error("expected instruction");
                exit(1);
            }

            char *first = strtok(NULL, " ,\t"); /* first operand */
            if (!first) {
                color_error("expected first operand");
                exit(1);
            }

            char *second = strtok(NULL, " ,\t"); /* second operand */
            if (!second) {
                color_error("expected second operand");
                exit(1);
            }

            uint8_t reg = syntax_get_register_code(first);
            if (reg == 0xFF) {
                color_error("unknown register '%s'", first);
                exit(1);
            }

            if (syntax_is_numeric(second)) {
                ensure_code_buffer_capacity(codeBuf, 7);  // Need up to 7 bytes

                uint64_t val = strtoull(second, NULL, 0);

                if (instrType == INSTR_COMP) {
                    /* Compare with immediate */
                    codeBuf->bytes[codeBuf->size++] = 0x48;       /* REX.W */
                    codeBuf->bytes[codeBuf->size++] = 0x81;       /* cmp r/m64, imm32 */
                    codeBuf->bytes[codeBuf->size++] = 0xF8 | reg; /* ModR/M: register direct */
                    /* 32-bit immediate */
                    for (int i = 0; i < 4; i++)
                        codeBuf->bytes[codeBuf->size++] = (uint8_t)((val >> (8 * i)) & 0xff);
                } else /* add */
                {
                    /* Add immediate */
                    codeBuf->bytes[codeBuf->size++] = 0x48;       /* REX.W */
                    codeBuf->bytes[codeBuf->size++] = 0x81;       /* add r/m64, imm32 */
                    codeBuf->bytes[codeBuf->size++] = 0xC0 | reg; /* ModR/M: register direct */
                    /* 32-bit immediate */
                    for (int i = 0; i < 4; i++)
                        codeBuf->bytes[codeBuf->size++] = (uint8_t)((val >> (8 * i)) & 0xff);
                }
            } else {
                ensure_code_buffer_capacity(codeBuf, 3);  // Need 3 bytes

                /* Register-register operation */
                uint8_t reg2 = syntax_get_register_code(second);
                if (reg2 == 0xFF) {
                    color_error("unknown register '%s'", second);
                    exit(1);
                }

                codeBuf->bytes[codeBuf->size++] = 0x48; /* REX.W */

                if (instrType == INSTR_COMP) {
                    codeBuf->bytes[codeBuf->size++] = 0x39;                     /* cmp r/m64, r64 */
                    codeBuf->bytes[codeBuf->size++] = 0xC0 | (reg2 << 3) | reg; /* ModR/M */
                } else                                                          /* add */
                {
                    codeBuf->bytes[codeBuf->size++] = 0x01;                     /* add r/m64, r64 */
                    codeBuf->bytes[codeBuf->size++] = 0xC0 | (reg2 << 3) | reg; /* ModR/M */
                }
            }
            break;
        }

        default:
            color_error("unknown instruction '%s'", trimmed);
            exit(1);
    }
}

/* Process data directives and copy data to the data buffer */
static void process_data_buffer(DataBuffer *dataBuf, uint64_t dataBase)
{
    /* Process collected data directives: assign symbol addresses and emit data */
    for (size_t i = 0; i < dataDirCount; i++) {
        uint64_t addr = dataBase + dataBuf->size;
        add_symbol(dataDirectives[i].label, addr);

        switch (dataDirectives[i].type) {
            case DATA_STRING: {
                char processed[MAX_LINE_LEN];
                syntax_process_escape_sequences(dataDirectives[i].data.literal, processed);
                size_t len = strlen(processed) + 1;  // Include null terminator

                ensure_data_buffer_capacity(dataBuf, len);

                memcpy(dataBuf->bytes + dataBuf->size, processed, len);
                dataBuf->size += len;
                break;
            }

            case DATA_BUFFER: {
                size_t size = dataDirectives[i].data.size;

                ensure_data_buffer_capacity(dataBuf, size);

                /* Zero-initialize the buffer */
                memset(dataBuf->bytes + dataBuf->size, 0, size);
                dataBuf->size += size;
                break;
            }

            case DATA_FILE: {
                /* Read file contents */
                FILE *fp = fopen(dataDirectives[i].data.filename, "rb");
                if (!fp) {
                    color_error("cannot open file '%s'", dataDirectives[i].data.filename);
                    exit(1);
                }

                /* Get file size */
                fseek(fp, 0, SEEK_END);
                size_t fileSize = ftell(fp);
                fseek(fp, 0, SEEK_SET);

                ensure_data_buffer_capacity(dataBuf, fileSize);

                /* Read file into data buffer */
                if (fread(dataBuf->bytes + dataBuf->size, 1, fileSize, fp) != fileSize) {
                    color_error("failed to read file '%s'", dataDirectives[i].data.filename);
                    fclose(fp);
                    exit(1);
                }
                fclose(fp);
                dataBuf->size += fileSize;
                break;
            }

            case DATA_RAW: {
                size_t size = sizeof(dataDirectives[i].data.value);

                ensure_data_buffer_capacity(dataBuf, size);

                memcpy(dataBuf->bytes + dataBuf->size, &dataDirectives[i].data.value, size);
                dataBuf->size += size;
                break;
            }

            default:
                color_error("internal error: unknown data directive type");
                exit(1);
        }
    }
}

/* ---- Main Assembly Function ---- */

/* Convenience wrapper for ELF output */
int assemble_to_elf(const char *input_filename, const char *output_filename)
{
    AssemblerOptions options = {.input_filename = input_filename,
                                .output_filename = output_filename,
                                .writer = write_elf_file,
                                .verbose = 0};
    return assemble(&options);
}

/* The main assembly function that processes the input assembly file,
   generates code and data, and calls the binary writer function. */
int assemble(const AssemblerOptions *options)
{
    /* Reset global state for multiple invocations */
    symbolCount = 0;
    dataDirCount = 0;

    /* Initialize the syntax module */
    syntax_init();

    if (options->verbose) {
        color_section("Assembly Process");
        color_info("Assembling file: %s", options->input_filename);
    }

    /* Read all lines from input. */
    size_t lineCount = read_all_lines(options->input_filename);

    if (options->verbose) {
        color_info("Read %zu lines from input file", lineCount);
    }

    /* First pass: simulate code emission and collect data directives */
    size_t simulatedCodeSize = first_pass(lineCount);

    if (options->verbose) {
        color_section("First Pass Results");
        color_info("Estimated code size: %zu bytes", simulatedCodeSize);
        color_info("Found %zu data directives", dataDirCount);
        color_info("Found %zu symbols", symbolCount);

        /* Display collected symbols if very verbose */
        if (options->verbose > 1) {
            color_section("Symbol Table");
            for (size_t i = 0; i < symbolCount; i++) {
                color_printf(COLOR_MAGENTA, "  %-20s", symbols[i].name);
                color_printf(COLOR_RESET, " = ");
                color_printf(COLOR_YELLOW, "0x%016lx\n", symbols[i].value);
            }
        }
    }

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

    if (options->verbose) {
        color_section("Data Processing");
        color_info("Processed data directives. Total data size: %zu bytes", dataBuf.size);
    }

    /* Second pass: emit instructions (ignore data directives) */
    for (size_t i = 0; i < lineCount; i++) {
        char *trimmed = syntax_trim(lines[i]);
        if (trimmed[0] == '\0' || syntax_is_comment(trimmed) || syntax_is_data_directive(trimmed))
            continue;
        emit_instruction_line(&codeBuf, trimmed);
    }

    if (options->verbose) {
        color_section("Second Pass Results");
        color_info("Actual code size: %zu bytes", codeBuf.size);
        color_info("Total binary size: %zu bytes", codeBuf.size + dataBuf.size);
        color_info("Writing output to: %s", options->output_filename);
    }

    /* Call the binary writer function */
    int result = options->writer(options->output_filename, &codeBuf, &dataBuf, entry_point);

    if (options->verbose) {
        if (result == 0) {
            color_success("Assembly completed successfully");
        } else {
            color_error("Assembly failed with code %d", result);
        }
    }

    /* Clean up */
    free_code_buffer(&codeBuf);
    free_data_buffer(&dataBuf);

    return result;
}
