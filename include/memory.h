#ifndef MEMORY_H
#define MEMORY_H

#include "common.h"

/*
 * Reads a text hex file (one 32-bit instruction per line, e.g. "00500113")
 * and stores it little-endian into 'memory' starting at address 0.
 * Returns the number of instructions loaded, or -1 if the file couldn't
 * be opened.
 */
int load_hex_file(const char *filename, uint8_t *memory, size_t mem_size);

#endif
