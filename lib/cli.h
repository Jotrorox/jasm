#ifndef CLI_H
#define CLI_H

#include <stdio.h>

/* Output format types supported by the assembler */
typedef enum { FORMAT_ELF, FORMAT_BINARY, FORMAT_UNKNOWN } OutputFormat;

/* Function declarations */
void print_usage(const char *program_name);
void print_version(void);
OutputFormat parse_format(const char *format_str);
int process_arguments(int argc,
                     char **argv,
                     const char **input_file,
                     const char **output_file,
                     OutputFormat *output_format,
                     int *verbose);
void print_assembly_info(const char *input_file,
                        const char *output_file,
                        OutputFormat output_format);

#endif /* CLI_H */ 