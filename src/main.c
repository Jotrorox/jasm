#include <stdio.h>
#include "assembler.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input.asm> <output binary>\n", argv[0]);
        return 1;
    }
    return assemble(argv[1], argv[2]);
}