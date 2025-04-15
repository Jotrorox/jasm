#include <stdio.h>
#include <sys/stat.h>
#include "assembler.h"
#include "binary_writer.h"
#include "cli.h"
#include "color_utils.h"
#include "error.h"
#include "syntax.h"

int main(const int argc, char **argv)
{
    const char *input_file = NULL;
    const char *output_file = NULL;
    int verbose = 0;
    OutputFormat output_format = FORMAT_ELF;

    /* Initialize color utilities */
    color_init();

    /* Initialize syntax module */
    syntax_init();

    /* Initialize error handling */
    error_init();

    /* Process command line arguments */
    int result = process_arguments(argc, argv, &input_file, &output_file, &output_format, &verbose);
    if (result != 0)
        /* -1 indicates help/version was shown, exit with success */
        return result < 0 ? 0 : result;

    /* If no output file was specified, use the default */
    if (!output_file) {
        const char *DEFAULT_OUTPUT = "a.out";
        output_file = DEFAULT_OUTPUT;
    }

    /* Set up the assembler options */
    const AssemblerOptions options = {
        .input_filename = input_file,
        .output_filename = output_file,
        .writer = (output_format == FORMAT_ELF) ? write_elf_file : write_binary_file,
        .verbose = verbose};

    /* Print a welcome banner if verbose */
    if (verbose)
        print_assembly_info(input_file, output_file, output_format);

    /* Assemble the file */
    result = assemble(&options);

    /* Check for errors */
    if (error_has_errors()) {
        if (error_has_fatal_errors()) {
            color_error("Assembly failed due to fatal errors");
            return 1;
        }
        color_warning("Assembly completed with errors");
        return 1;
    }

    /* Make ELF files executable */
    if (result == 0 && output_format == FORMAT_ELF) {
        chmod(output_file, 0755);

        if (!options.verbose)
            color_success("Assembled '%s' to '%s'", input_file, output_file);
    }

    return result;
}