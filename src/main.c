#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "assembler.h"
#include "binary_writer.h"
#include "color_utils.h"
#include "syntax.h"

#define VERSION "0.1"

/* Output format types supported by the assembler */
typedef enum { FORMAT_ELF, FORMAT_BINARY, FORMAT_UNKNOWN } OutputFormat;

/* Function prototypes */
static void print_usage(const char *program_name);
static void print_version(void);
static OutputFormat parse_format(const char *format_str);
static int process_arguments(int argc,
                             char **argv,
                             const char **input_file,
                             const char **output_file,
                             OutputFormat *output_format,
                             int *verbose);
static void print_assembly_info(const char *input_file,
                                const char *output_file,
                                OutputFormat output_format);

/* External binary writer functions - these are implemented elsewhere */
extern int write_elf_file(const char *output_filename,
                          CodeBuffer *codeBuf,
                          DataBuffer *dataBuf,
                          uint64_t entry_point);

extern int write_binary_file(const char *output_filename,
                             CodeBuffer *codeBuf,
                             DataBuffer *dataBuf,
                             uint64_t entry_point);

/* Print usage information */
static void print_usage(const char *program_name)
{
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
static void print_version(void)
{
    color_printf(COLOR_BOLD COLOR_BRIGHT_BLUE, "JASM Assembler v%s\n", VERSION);
    color_printf(COLOR_BRIGHT_WHITE, "Copyright (c) 2025 Johannes (Jotrorox) Müller\n");
}

/* Parse format string into OutputFormat enum */
static OutputFormat parse_format(const char *format_str)
{
    if (!format_str)
        return FORMAT_ELF;  // Default

    if (strcmp(format_str, "elf") == 0)
        return FORMAT_ELF;
    else if (strcmp(format_str, "bin") == 0)
        return FORMAT_BINARY;
    else
        return FORMAT_UNKNOWN;
}

/* Process command line arguments */
static int process_arguments(int argc,
                             char **argv,
                             const char **input_file,
                             const char **output_file,
                             OutputFormat *output_format,
                             int *verbose)
{
    const char *format_str = NULL;

    /* Parse command line arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return -1; /* Special return code to indicate early exit */
        } else if (strcmp(argv[i], "-V") == 0 || strcmp(argv[i], "--version") == 0) {
            print_version();
            return -1; /* Special return code to indicate early exit */
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            *verbose = 1;
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
        } else if (!*input_file) {
            *input_file = argv[i];
        } else if (!*output_file) {
            *output_file = argv[i];
        } else {
            color_error("Too many arguments");
            return 1;
        }
    }

    /* Check if input file was provided */
    if (!*input_file) {
        color_error("No input file specified");
        print_usage(argv[0]);
        return 1;
    }

    /* Parse the output format */
    if (format_str) {
        *output_format = parse_format(format_str);
        if (*output_format == FORMAT_UNKNOWN) {
            color_error("Unknown output format '%s'", format_str);
            return 1;
        }
    }

    return 0;
}

/* Print assembly information if verbose mode is enabled */
static void print_assembly_info(const char *input_file,
                                const char *output_file,
                                OutputFormat output_format)
{
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

int main(int argc, char **argv)
{
    const char *DEFAULT_OUTPUT = "a.out";
    const char *input_file = NULL;
    const char *output_file = NULL;
    int verbose = 0;
    OutputFormat output_format = FORMAT_ELF;
    int result;

    /* Initialize color utilities */
    color_init();

    /* Initialize syntax module */
    syntax_init();

    /* Process command line arguments */
    result = process_arguments(argc, argv, &input_file, &output_file, &output_format, &verbose);
    if (result != 0) {
        /* -1 indicates help/version was shown, exit with success */
        return result < 0 ? 0 : result;
    }

    /* If no output file was specified, use the default */
    if (!output_file) {
        output_file = DEFAULT_OUTPUT;
    }

    /* Set up the assembler options */
    AssemblerOptions options = {
        .input_filename = input_file,
        .output_filename = output_file,
        .writer = (output_format == FORMAT_ELF) ? write_elf_file : write_binary_file,
        .verbose = verbose};

    /* Print a welcome banner if verbose */
    if (verbose) {
        print_assembly_info(input_file, output_file, output_format);
    }

    /* Assemble the file */
    result = assemble(&options);

    /* Make ELF files executable */
    if (result == 0 && output_format == FORMAT_ELF) {
        chmod(output_file, 0755);

        if (!options.verbose) {
            /* Only print success message if not verbose (verbose mode already prints
             * one) */
            color_success("Assembled '%s' to '%s'", input_file, output_file);
        }
    }

    return result;
}