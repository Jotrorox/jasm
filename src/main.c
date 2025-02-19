#include <stdio.h>
#include <sys/stat.h>
#include "assembler.h"

int main(int argc, char **argv)
{
    const char *output_name = "a.out";
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <input.asm> [output binary]\n", argv[0]);
        return 1;
    }

    // Use provided output name if given, otherwise use default
    const char *output_file = (argc == 3) ? argv[2] : output_name;

    int result = assemble(argv[1], output_file);
    if (result == 0)
    {
        // Make the output file executable (rwxr-xr-x)
        chmod(output_file, 0755);
    }
    return result;
}