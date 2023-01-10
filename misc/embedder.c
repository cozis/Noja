#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

// gcc embedder.c -o embedder -Wall -Wextra

int main(int argc, char **argv)
{
    const char *variable = "__variable_name__";
    const char *input = NULL;
    const char *output = "output.c";
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input> <variable-name> [<output>]\n", 
                argv[0]);
        return -1;
    }

    input = argv[1];
    variable = argv[2];
    if (argc > 3)
        output = argv[3];

    FILE *in_stream = fopen(input, "rb");
    if (in_stream == NULL) {
        fprintf(stderr, "Error: Failed to open \"%s\"\n", input);
        return -1;
    }

    FILE *out_stream = fopen(output, "wb");
    if (out_stream == NULL) {
        fprintf(stderr, "Error: Failed to open \"%s\"\n", output);
        fclose(in_stream);
        return -1;
    }
    
    fprintf(out_stream, "const unsigned char %s[] = {\n\t", variable);

    size_t w = 0;
    bool done = false;
    while (!done) {
        uint8_t buffer[1024];
        size_t num = fread(buffer, 1, sizeof(buffer), in_stream);
        if (num < sizeof(buffer)) {
            if (ferror(in_stream)) {
                abort();
            } else {
                assert(feof(in_stream));
                done = true;
            }
        }

        for (size_t i = 0; i < num; i++, w++) {
            fprintf(out_stream, "%3d, ", buffer[i]);
            if ((w+1) % 16 == 0)
                fprintf(out_stream, "\n\t");
        }
    }
    
    fprintf(out_stream, "\n};\n");

    fclose(in_stream);
    fclose(out_stream);
    return 0;
}