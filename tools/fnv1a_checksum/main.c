/* Copyright (c) 2026 Adam Ogiba - Licensed under MIT */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define FNV1A_64_OFFSET_BASIS 0xcbf29ce484222325ULL
#define FNV1A_64_PRIME        0x00000100000001b3ULL

uint64_t fnv1a_64(const uint8_t *data, size_t len) {
    uint64_t hash = FNV1A_64_OFFSET_BASIS;
    for (size_t i = 0; i < len; ++i) {
        hash ^= data[i];
        hash *= FNV1A_64_PRIME;
    }
    return hash;
}

int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 4) {
        fprintf(stderr, "Usage: %s <input_path> <start_byte> [length|\"calc\"]\n", argv[0]);
        return 1;
    }

    const char *input_path = argv[1];
    long start_byte = strtol(argv[2], NULL, 0);
    long length = -1;
    int calc_mode = 0;
    if (argc == 4) {
        if (strcmp(argv[3], "calc") == 0) {
            calc_mode = 1;
        } else {
            length = strtol(argv[3], NULL, 0);
        }
    }

    FILE *in = fopen(input_path, "rb");
    if (!in) {
        perror("Failed to open input file");
        return 1;
    }

    if (fseek(in, 0, SEEK_END) != 0) {
        perror("Failed to seek input file");
        fclose(in);
        return 1;
    }
    long file_size = ftell(in);
    if (file_size < 0) {
        perror("Failed to get file size");
        fclose(in);
        return 1;
    }

    if (start_byte < 0 || start_byte >= file_size) {
        fprintf(stderr, "Invalid start_byte\n");
        fclose(in);
        return 1;
    }

    if (calc_mode) {
        uint32_t remaining = (uint32_t)(file_size - start_byte);
        for (int i = 0; i < 4; ++i) {
            putchar((uint8_t)(remaining >> (8 * i)));
        }
        fclose(in);
        return 0;
    }

    if (length < 0) {
        length = file_size - start_byte;
    }
    if (start_byte + length > file_size) {
        fprintf(stderr, "Invalid length\n");
        fclose(in);
        return 1;
    }

    if (fseek(in, start_byte, SEEK_SET) != 0) {
        perror("Failed to seek input file");
        fclose(in);
        return 1;
    }

    uint8_t *buffer = malloc(length);
    if (!buffer) {
        perror("Failed to allocate buffer");
        fclose(in);
        return 1;
    }

    size_t read_bytes = fread(buffer, 1, length, in);
    fclose(in);
    if (read_bytes != (size_t)length) {
        fprintf(stderr, "Failed to read input file\n");
        free(buffer);
        return 1;
    }

    uint64_t hash = fnv1a_64(buffer, length);
    free(buffer);

    // Output hash as 8 bytes to stdout
    for (int i = 0; i < 8; ++i) {
        putchar((uint8_t)(hash >> (8 * i)));
    }

    return 0;
}
