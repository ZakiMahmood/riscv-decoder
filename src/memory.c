#include "memory.h"
#include <string.h>

int load_hex_file(const char *filename, uint8_t *memory, size_t mem_size) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Cannot open hex file");
        return -1;
    }

    char line[32];
    uint32_t addr = 0;

    while (fgets(line, sizeof(line), fp) != NULL) {
        /* skip blank lines so trailing newlines in the file don't break anything */
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '\0') {
            continue;
        }

        if (addr + 3 >= mem_size) {
            fprintf(stderr, "Warning: hex file exceeds memory size, truncating\n");
            break;
        }

        uint32_t word = (uint32_t)strtoul(line, NULL, 16);

        /* RISC-V is little-endian, so the LSB goes at the lowest address */
        memory[addr + 0] = (uint8_t)(word >> 0);
        memory[addr + 1] = (uint8_t)(word >> 8);
        memory[addr + 2] = (uint8_t)(word >> 16);
        memory[addr + 3] = (uint8_t)(word >> 24);

        addr += INSTR_SIZE;
    }

    fclose(fp);
    return (int)(addr / INSTR_SIZE);
}
