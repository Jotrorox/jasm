#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "binary_writer.h"

/* Write the assembled code and data as a raw binary file.
   This format just outputs the raw machine code and data, without any headers.
*/
int write_binary_file(const char* outfile, CodeBuffer* codeBuf, DataBuffer* dataBuf, uint64_t entry_point) {
    (void)entry_point;

    /* For raw binary format, we just write the code and data sections consecutively */
    size_t total_size = codeBuf->size + dataBuf->size;
    uint8_t* combined_buffer = malloc(total_size);
    if (!combined_buffer) {
        perror("malloc for combined buffer");
        return 1;
    }
    
    /* Copy code and data into the combined buffer */
    memcpy(combined_buffer, codeBuf->bytes, codeBuf->size);
    memcpy(combined_buffer + codeBuf->size, dataBuf->bytes, dataBuf->size);
    
    /* Write the combined buffer to the output file */
    FILE* out = fopen(outfile, "wb");
    if (!out) {
        perror("fopen");
        free(combined_buffer);
        return 1;
    }
    
    size_t written = fwrite(combined_buffer, 1, total_size, out);
    if (written != total_size) {
        perror("fwrite");
        fclose(out);
        free(combined_buffer);
        return 1;
    }
    
    fclose(out);
    free(combined_buffer);
    
    printf("Assembled %zu bytes of machine code and %zu bytes of data into raw binary file: %s\n",
           codeBuf->size, dataBuf->size, outfile);
    
    return 0;
} 