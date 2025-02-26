/**
 * syntax.c - Implementation of syntax definitions for jasm assembler
 */

#include "syntax.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

/* Default syntax configuration */
char syntax_comment_char = '#';
const char* syntax_data_keyword = "data";
const char* syntax_size_keyword = "size";
const char* syntax_file_keyword = "file";
const char* syntax_label_suffix = ":";

/* Static lookup tables for instructions and registers */
typedef struct {
    const char* name;
    InstructionType type;
} InstructionEntry;

typedef struct {
    const char* name;
    RegisterType type;
    uint8_t code;
} RegisterEntry;

static InstructionEntry instructions[] = {
    {"mov", INSTR_MOVE},
    {"call", INSTR_CALL},
    {"jmp", INSTR_JUMP},
    {"jmplt", INSTR_JUMPLT},
    {"jmpgt", INSTR_JUMPGT},
    {"jmpeq", INSTR_JUMPEQ},
    {"cmp", INSTR_COMP},
    {"add", INSTR_ADD},
    {NULL, INSTR_UNKNOWN}
};

static RegisterEntry registers[] = {
    {"rax", REG_RAX, 0x00},
    {"rcx", REG_RCX, 0x01},
    {"rdx", REG_RDX, 0x02},
    {"rbx", REG_RBX, 0x03},
    {"rsi", REG_RSI, 0x06},
    {"rdi", REG_RDI, 0x07},
    {NULL, REG_UNKNOWN, 0}
};

/* Buffer for extracted strings */
static char syntax_buffer[SYNTAX_MAX_LINE_LEN];

/* Initialization function */
void syntax_init(void) {
    /* Default initialization already done with static values */
    /* Could be extended to load syntax from config file */
}

/* Trim leading and trailing whitespace; modifies string in place. */
char* syntax_trim(char *str) {
    if (!str) return NULL;
    
    while (*str && isspace((unsigned char)*str))
        str++;
        
    if (*str == '\0')
        return str;
        
    char *end = str + strlen(str) - 1;
    while (end >= str && (isspace((unsigned char)*end) || *end == '\n' || *end == '\r')) {
        *end = '\0';
        end--;
    }
    
    return str;
}

/* Determine the type of a syntax element */
SyntaxElementType syntax_get_element_type(const char* str) {
    if (!str || !*str)
        return SYNTAX_UNKNOWN;
        
    if (syntax_is_instruction(str))
        return SYNTAX_INSTRUCTION;
        
    if (syntax_is_register(str))
        return SYNTAX_REGISTER;
        
    if (syntax_is_label(str))
        return SYNTAX_LABEL;
        
    if (syntax_is_comment(str))
        return SYNTAX_COMMENT;
        
    if (syntax_is_data_directive(str))
        return SYNTAX_DATA_DIRECTIVE;
        
    if (syntax_is_memory_reference(str))
        return SYNTAX_MEMORY_REFERENCE;
        
    return SYNTAX_UNKNOWN;
}

/* Check if string is an instruction */
bool syntax_is_instruction(const char* str) {
    if (!str || !*str)
        return false;
        
    /* Skip leading whitespace */
    while (*str && isspace((unsigned char)*str))
        str++;
        
    for (int i = 0; instructions[i].name != NULL; i++) {
        if (strncmp(str, instructions[i].name, strlen(instructions[i].name)) == 0) {
            /* Check if it's exactly the instruction, or followed by whitespace */
            char next = str[strlen(instructions[i].name)];
            if (next == '\0' || isspace((unsigned char)next))
                return true;
        }
    }
    
    return false;
}

/* Get instruction type from string */
InstructionType syntax_get_instruction_type(const char* str) {
    if (!str || !*str)
        return INSTR_UNKNOWN;
        
    /* Skip leading whitespace */
    while (*str && isspace((unsigned char)*str))
        str++;
        
    for (int i = 0; instructions[i].name != NULL; i++) {
        if (strncmp(str, instructions[i].name, strlen(instructions[i].name)) == 0) {
            /* Check if it's exactly the instruction, or followed by whitespace */
            char next = str[strlen(instructions[i].name)];
            if (next == '\0' || isspace((unsigned char)next))
                return instructions[i].type;
        }
    }
    
    return INSTR_UNKNOWN;
}

/* Check if string is a register */
bool syntax_is_register(const char* str) {
    if (!str || !*str)
        return false;
        
    for (int i = 0; registers[i].name != NULL; i++) {
        if (strcmp(str, registers[i].name) == 0)
            return true;
    }
    
    return false;
}

/* Get register type from string */
RegisterType syntax_get_register_type(const char* str) {
    if (!str || !*str)
        return REG_UNKNOWN;
        
    for (int i = 0; registers[i].name != NULL; i++) {
        if (strcmp(str, registers[i].name) == 0)
            return registers[i].type;
    }
    
    return REG_UNKNOWN;
}

/* Get register code for the given register name */
uint8_t syntax_get_register_code(const char* reg) {
    for (int i = 0; registers[i].name != NULL; i++) {
        if (strcmp(reg, registers[i].name) == 0)
            return registers[i].code;
    }
    
    return 0xFF; /* Unknown register */
}

/* Get register code from register type */
uint8_t syntax_get_register_code_by_type(RegisterType reg) {
    for (int i = 0; registers[i].name != NULL; i++) {
        if (registers[i].type == reg)
            return registers[i].code;
    }
    
    return 0xFF; /* Unknown register */
}

/* Convert instruction type to string */
const char* syntax_instruction_to_string(InstructionType instr) {
    for (int i = 0; instructions[i].name != NULL; i++) {
        if (instructions[i].type == instr)
            return instructions[i].name;
    }
    
    return "unknown";
}

/* Convert register type to string */
const char* syntax_register_to_string(RegisterType reg) {
    for (int i = 0; registers[i].name != NULL; i++) {
        if (registers[i].type == reg)
            return registers[i].name;
    }
    
    return "unknown";
}

/* Check if string is a label definition */
bool syntax_is_label(const char* str) {
    if (!str || !*str)
        return false;
        
    /* Skip leading whitespace */
    while (*str && isspace((unsigned char)*str))
        str++;
        
    if (!*str)
        return false;
        
    size_t len = strlen(str);
    return len > 0 && str[len - 1] == syntax_label_suffix[0];
}

/* Extract label name from label definition */
char* syntax_extract_label_name(const char* str) {
    if (!syntax_is_label(str))
        return NULL;
        
    /* Skip leading whitespace */
    while (*str && isspace((unsigned char)*str))
        str++;
        
    size_t len = strlen(str);
    if (len <= 1)
        return NULL; /* Empty label */
        
    /* Copy without trailing colon - use safer approach */
    size_t copy_len = (len - 1 < sizeof(syntax_buffer) - 1) ? (len - 1) : (sizeof(syntax_buffer) - 1);
    memcpy(syntax_buffer, str, copy_len);
    syntax_buffer[copy_len] = '\0';
    
    return syntax_trim(syntax_buffer);
}

/* Check if string is a comment */
bool syntax_is_comment(const char* str) {
    if (!str || !*str)
        return false;
        
    /* Skip leading whitespace */
    while (*str && isspace((unsigned char)*str))
        str++;
        
    return *str == syntax_comment_char;
}

/* Check if string is a data directive */
bool syntax_is_data_directive(const char* str) {
    if (!str || !*str)
        return false;
        
    /* Skip leading whitespace */
    while (*str && isspace((unsigned char)*str))
        str++;
        
    /* Check for data keyword followed by whitespace */
    if (strncmp(str, syntax_data_keyword, strlen(syntax_data_keyword)) == 0 && 
        (str[strlen(syntax_data_keyword)] == '\0' || 
         isspace((unsigned char)str[strlen(syntax_data_keyword)])))
        return true;
        
    return false;
}

/* Check if string is a memory reference */
bool syntax_is_memory_reference(const char* str) {
    if (!str || !*str)
        return false;
        
    return str[0] == '[' && str[strlen(str) - 1] == ']';
}

/* Extract symbol name from memory reference */
char* syntax_extract_memory_reference(const char* str) {
    if (!syntax_is_memory_reference(str))
        return NULL;
        
    size_t len = strlen(str) - 2; /* exclude [] */
    strncpy(syntax_buffer, str + 1, len);
    syntax_buffer[len] = '\0';
    
    return syntax_trim(syntax_buffer);
}

/* Check if string is a numeric constant */
bool syntax_is_numeric(const char* str) {
    if (!str || !*str)
        return false;
        
    /* Skip leading whitespace */
    while (*str && isspace((unsigned char)*str))
        str++;
        
    if (isdigit((unsigned char)str[0]) || str[0] == '-' ||
        (str[0] == '0' && (str[1] == 'x' || str[1] == 'X' || str[1] == 'b' || str[1] == 'B')))
        return true;
        
    return false;
}

/* Process escape sequences in string literal */
void syntax_process_escape_sequences(const char* input, char* output) {
    if (!input || !output) return;
    
    while (*input) {
        if (*input == '\\') {
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
        else {
            *output = *input;
        }
        input++;
        output++;
    }
    *output = '\0';
}

/* Process a data directive */
bool syntax_process_data_directive(const char* line, SyntaxDataDirective* directive) {
    if (!line || !directive)
        return false;
        
    char line_copy[SYNTAX_MAX_LINE_LEN];
    strncpy(line_copy, line, SYNTAX_MAX_LINE_LEN - 1);
    line_copy[SYNTAX_MAX_LINE_LEN - 1] = '\0';
    
    char* p = syntax_trim(line_copy);
    
    /* Skip data keyword */
    if (strncmp(p, syntax_data_keyword, strlen(syntax_data_keyword)) != 0)
        return false;
        
    p += strlen(syntax_data_keyword);
    p = syntax_trim(p);
    
    /* Extract label */
    char* label = strtok(p, " \t");
    if (!label)
        return false;
        
    strncpy(directive->label, label, SYNTAX_MAX_LABEL_LEN - 1);
    directive->label[SYNTAX_MAX_LABEL_LEN - 1] = '\0';
    
    /* Extract value */
    char* value = strtok(NULL, "\n");
    if (!value)
        return false;
        
    value = syntax_trim(value);
    
    /* Determine the data type */
    if (value[0] == '"') {
        /* String literal */
        directive->type = DATA_STRING;
        value++; /* Skip opening quote */
        
        char* endQuote = strchr(value, '"');
        if (!endQuote)
            return false;
            
        *endQuote = '\0';
        strncpy(directive->data.literal, value, SYNTAX_MAX_LINE_LEN - 1);
        directive->data.literal[SYNTAX_MAX_LINE_LEN - 1] = '\0';
    }
    else if (strncmp(value, syntax_file_keyword, strlen(syntax_file_keyword)) == 0 &&
             (value[strlen(syntax_file_keyword)] == '\0' || 
              isspace((unsigned char)value[strlen(syntax_file_keyword)]))) {
        /* File inclusion */
        directive->type = DATA_FILE;
        
        value += strlen(syntax_file_keyword);
        value = syntax_trim(value);
        
        if (!value || !*value)
            return false;
            
        strncpy(directive->data.filename, value, SYNTAX_MAX_LINE_LEN - 1);
        directive->data.filename[SYNTAX_MAX_LINE_LEN - 1] = '\0';
    }
    else if (strncmp(value, syntax_size_keyword, strlen(syntax_size_keyword)) == 0 &&
             (value[strlen(syntax_size_keyword)] == '\0' || 
              isspace((unsigned char)value[strlen(syntax_size_keyword)]))) {
        /* Buffer allocation */
        directive->type = DATA_BUFFER;
        
        value += strlen(syntax_size_keyword);
        value = syntax_trim(value);
        
        if (!syntax_is_numeric(value))
            return false;
            
        directive->data.size = strtoull(value, NULL, 0);
    }
    else if (syntax_is_numeric(value)) {
        /* Raw numeric value */
        directive->type = DATA_RAW;
        
        if (value[0] == '0' && (value[1] == 'b' || value[1] == 'B')) {
            /* Binary value */
            value += 2; /* Skip 0b prefix */
            directive->data.value = 0;
            
            while (*value == '0' || *value == '1') {
                directive->data.value = (directive->data.value << 1) | (*value - '0');
                value++;
            }
        }
        else {
            /* Decimal or hex value */
            directive->data.value = strtoull(value, NULL, 0);
        }
    }
    else {
        return false;
    }
    
    return true;
} 