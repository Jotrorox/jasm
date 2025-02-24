#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "assembler.h"
#include "binary_writer.h"
#include "color_utils.h"

#define VERSION "0.1"

/* Output format types supported by the assembler */
typedef enum {
    FORMAT_ELF,
    FORMAT_BINARY,
    FORMAT_UNKNOWN
} OutputFormat;

/* Print usage information */
static void print_usage(const char *program_name) {
    printf("\n");
    color_printf(COLOR_BOLD COLOR_BRIGHT_BLUE, "JASM Assembler - Usage Guide\n");
    printf("\n");
    
    color_printf(COLOR_BOLD, "USAGE:\n");
    color_printf(COLOR_BRIGHT_WHITE, "  %s [options] <input.jasm> [output]\n", program_name);
    
    printf("\n");
    color_printf(COLOR_BOLD, "OPTIONS:\n");
    color_printf(COLOR_BRIGHT_GREEN, "  -h, --help            ");
    printf("Display this help message and exit\n");
    
    color_printf(COLOR_BRIGHT_GREEN, "  -v, --verbose         ");
    printf("Enable verbose output\n");
    
    color_printf(COLOR_BRIGHT_GREEN, "  -V, --version         ");
    printf("Display version information and exit\n");
    
    color_printf(COLOR_BRIGHT_GREEN, "  -f, --format <format> ");
    printf("Specify output format (elf, bin)\n");
    
    printf("\n");
    color_printf(COLOR_BOLD, "FORMATS:\n");
    color_printf(COLOR_BRIGHT_YELLOW, "  elf                   ");
    printf("ELF executable (default)\n");
    
    color_printf(COLOR_BRIGHT_YELLOW, "  bin                   ");
    printf("Raw binary file\n");
    
    printf("\n");
    color_printf(COLOR_BOLD, "EXAMPLES:\n");
    color_printf(COLOR_BRIGHT_CYAN, "  %s program.jasm                  ", program_name);
    printf("Assemble to a.out (ELF)\n");
    
    color_printf(COLOR_BRIGHT_CYAN, "  %s -f bin program.jasm prog.bin  ", program_name);
    printf("Assemble to raw binary\n");
    
    color_printf(COLOR_BRIGHT_CYAN, "  %s -v program.jasm prog          ", program_name);
    printf("Assemble with verbose output\n");
    
    printf("\n");
}

/* Print version information */
static void print_version() {
    color_printf(COLOR_BOLD COLOR_BRIGHT_BLUE, "JASM Assembler v%s\n", VERSION);
    color_printf(COLOR_BRIGHT_WHITE, "Copyright (c) 2025 Johannes (Jotrorox) Müller\n");
}

/* Parse format string into OutputFormat enum */
static OutputFormat parse_format(const char *format_str) {
    if (!format_str)
        return FORMAT_ELF; // Default

    if (strcmp(format_str, "elf") == 0)
        return FORMAT_ELF;
    else if (strcmp(format_str, "bin") == 0)
        return FORMAT_BINARY;
    else
        return FORMAT_UNKNOWN;
}

/* Prototype for binary format writer (to be implemented) */
int write_binary_file(const char* output_filename, 
                     CodeBuffer* codeBuf, 
                     DataBuffer* dataBuf,
                     uint64_t entry_point);

int main(int argc, char **argv) {
    const char *output_name = "a.out";
    const char *input_file = NULL;
    const char *output_file = NULL;
    const char *format_str = NULL;
    int verbose = 0;
    OutputFormat output_format = FORMAT_ELF;
    
    /* Initialize color utilities */
    color_init();
    
    /* Parse command line arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-V") == 0 || strcmp(argv[i], "--version") == 0) {
            print_version();
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--format") == 0) {
            if (i + 1 < argc) {
                format_str = argv[++i];
            } else {
                color_error("--format requires an argument");
                return 1;
            }
        } else if (argv[i][0] == '-') {
            color_error("Unknown option '%s'", argv[i]);
            return 1;
        } else if (!input_file) {
            input_file = argv[i];
        } else if (!output_file) {
            output_file = argv[i];
        } else {
            color_error("Too many arguments");
            return 1;
        }
    }
    
    /* Check if input file was provided */
    if (!input_file) {
        color_error("No input file specified");
        print_usage(argv[0]);
        return 1;
    }
    
    /* If no output file was specified, use the default */
    if (!output_file) {
        output_file = output_name;
    }
    
    /* Parse the output format */
    output_format = parse_format(format_str);
    if (output_format == FORMAT_UNKNOWN) {
        color_error("Unknown output format '%s'", format_str);
        return 1;
    }
    
    /* Set up the assembler options */
    AssemblerOptions options = {
        .input_filename = input_file,
        .output_filename = output_file,
        .writer = NULL,
        .verbose = verbose
    };
    
    /* Select the appropriate writer function based on format */
    switch (output_format) {
        case FORMAT_ELF:
            options.writer = write_elf_file;
            break;
        case FORMAT_BINARY:
            options.writer = write_binary_file;
            break;
        default:
            color_error("Internal error - unknown format");
            return 1;
    }
    
    /* Print a welcome banner if verbose */
    if (verbose) {
        printf("\n");
        color_printf(COLOR_BOLD COLOR_BRIGHT_BLUE, "JASM Assembler v%s\n", VERSION);
        color_printf(COLOR_BRIGHT_CYAN, "----------------------------------------\n");
        color_printf(COLOR_RESET, "Input file:  ");
        color_printf(COLOR_BRIGHT_WHITE, "%s\n", input_file);
        color_printf(COLOR_RESET, "Output file: ");
        color_printf(COLOR_BRIGHT_WHITE, "%s\n", output_file);
        color_printf(COLOR_RESET, "Format:      ");
        color_printf(COLOR_BRIGHT_YELLOW, "%s\n", output_format == FORMAT_ELF ? "ELF" : "Binary");
        printf("\n");
    }
    
    /* Assemble the file */
    int result = assemble(&options);
    
    /* Make ELF files executable */
    if (result == 0 && output_format == FORMAT_ELF) {
        chmod(output_file, 0755);
        
        if (!options.verbose) {
            /* Only print success message if not verbose (verbose mode already prints one) */
            color_success("Assembled '%s' to '%s'", input_file, output_file);
        }
    }
    
    return result;
}