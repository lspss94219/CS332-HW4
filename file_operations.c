#include "file_operations.h"
#include <stdio.h>

void save_result_to_file(const char* filename, int result) {
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        printf("Failed to open file: %s\n", filename);
        return;
    }

    fprintf(file, "Result: %d\n", result);
    fclose(file);
}
