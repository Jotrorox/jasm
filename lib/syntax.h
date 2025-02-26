/**
 * syntax.h - Syntax definitions for jasm assembler
 *
 * This header defines the syntax elements of the jasm assembly language,
 * making it easier to modify the language without changing the assembler code.
 */

#ifndef SYNTAX_H
#define SYNTAX_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Maximum length of various strings */
#define SYNTAX_MAX_LINE_LEN  256
#define SYNTAX_MAX_LABEL_LEN 32

/* Syntax element types */
typedef enum {
    SYNTAX_INSTRUCTION,
    SYNTAX_REGISTER,
    SYNTAX_LABEL,
    SYNTAX_COMMENT,
    SYNTAX_DATA_DIRECTIVE,
    SYNTAX_MEMORY_REFERENCE,
    SYNTAX_UNKNOWN
} SyntaxElementType;

/* Instruction types */
typedef enum {
    INSTR_MOVE,
    INSTR_CALL,
    INSTR_JUMP,
    INSTR_JUMPLT,
    INSTR_JUMPGT,
    INSTR_JUMPEQ,
    INSTR_COMP,
    INSTR_ADD,
    INSTR_UNKNOWN
} InstructionType;

/* Register types */
typedef enum { REG_RAX, REG_RCX, REG_RDX, REG_RBX, REG_RSI, REG_RDI, REG_UNKNOWN } RegisterType;

/* Data directive types */
typedef enum { DATA_STRING, DATA_BUFFER, DATA_FILE, DATA_RAW, DATA_UNKNOWN } DataDirectiveType;

/* Data directive structure for storing parsed data info */
typedef struct {
    char label[SYNTAX_MAX_LABEL_LEN];
    DataDirectiveType type;
    union {
        char literal[SYNTAX_MAX_LINE_LEN];
        size_t size;
        char filename[SYNTAX_MAX_LINE_LEN];
        uint64_t value; /* For raw numeric values */
    } data;
} SyntaxDataDirective;

/**
 * Initialize the syntax module.
 * This would allow for runtime configuration of syntax elements.
 */
void syntax_init(void);

/**
 * Functions to identify syntax elements
 */
SyntaxElementType syntax_get_element_type(const char *str);
bool syntax_is_instruction(const char *str);
bool syntax_is_register(const char *str);
bool syntax_is_label(const char *str);
bool syntax_is_comment(const char *str);
bool syntax_is_data_directive(const char *str);
bool syntax_is_memory_reference(const char *str);
bool syntax_is_numeric(const char *str);

/**
 * Functions to get specific element types
 */
InstructionType syntax_get_instruction_type(const char *str);
RegisterType syntax_get_register_type(const char *str);
uint8_t syntax_get_register_code(const char *reg);
uint8_t syntax_get_register_code_by_type(RegisterType reg);

/**
 * String conversion functions
 */
const char *syntax_instruction_to_string(InstructionType instr);
const char *syntax_register_to_string(RegisterType reg);

/**
 * Parsing functions
 */
char *syntax_extract_label_name(const char *str);
char *syntax_extract_memory_reference(const char *str);
char *syntax_trim(char *str);

/**
 * Data directive parsing
 */
bool syntax_process_data_directive(const char *line, SyntaxDataDirective *directive);
void syntax_process_escape_sequences(const char *input, char *output);

/**
 * Syntax configuration
 */
extern char syntax_comment_char;
extern const char *syntax_data_keyword;
extern const char *syntax_size_keyword;
extern const char *syntax_file_keyword;
extern const char *syntax_label_suffix;

#endif /* SYNTAX_H */