#include "arguments.h"
#include <stdio.h>
#include <string.h>

void parse_arguments(int argc, char* argv[]) {
    if (argc < 2) {
        printf("No arguments provided. Default configuration will be used.\n");
    } else {
        printf("Arguments provided: %s\n", argv[1]);
    }
}
