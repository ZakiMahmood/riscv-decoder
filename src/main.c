#include "common.h"
#include "memory.h"
#include "decoder.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <hex_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    uint8_t *memory = calloc(MEMORY_SIZE, sizeof(uint8_t));
    if (memory == NULL) {
        fprintf(stderr, "Memory allocation failed!\n");
        return EXIT_FAILURE;
    }

    int num_instructions = load_hex_file(argv[1], memory, MEMORY_SIZE);
    if (num_instructions < 0) {
        free(memory);
        return EXIT_FAILURE;
    }

    printf("RISC-V RV32I Instruction Decoder\n");
    printf("================================\n");
    printf("Loaded %d instructions from %s\n\n", num_instructions, argv[1]);
    printf("Addr        Hex         Assembly\n");
    printf("----------- ----------- -------------------------\n");

    int unknown_count = 0;
    for (int i = 0; i < num_instructions; i++) {
        uint32_t pc = (uint32_t)i * INSTR_SIZE;

        /* reassemble the 4 little-endian bytes back into one 32-bit word */
        uint32_t raw_instr = ((uint32_t)memory[pc + 3] << 24) |
                              ((uint32_t)memory[pc + 2] << 16) |
                              ((uint32_t)memory[pc + 1] << 8)  |
                               (uint32_t)memory[pc + 0];

        decoded_instr_t decoded;
        decode_instruction(raw_instr, &decoded);
        print_decoded(pc, raw_instr, &decoded);

        if (!decoded.valid) {
            unknown_count++;
        }
    }

    printf("\nDecoded %d instructions (%d valid, %d unknown)\n",
           num_instructions, num_instructions - unknown_count, unknown_count);

    free(memory);
    memory = NULL;

    return EXIT_SUCCESS;
}
