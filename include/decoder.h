#ifndef DECODER_H
#define DECODER_H

#include "common.h"

/* the 7-bit opcode field tells us which instruction format we're dealing with */
typedef enum {
    OP_R_TYPE = 0x33,  /* register-register ops: add, sub, and, or, ... */
    OP_I_TYPE = 0x13,  /* immediate arithmetic: addi, andi, slli, ... */
    OP_LOAD   = 0x03,  /* loads: lb, lh, lw, lbu, lhu */
    OP_STORE  = 0x23,  /* stores: sb, sh, sw */
    OP_BRANCH = 0x63,  /* conditional branches: beq, bne, blt, ... */
    OP_JAL    = 0x6F,  /* jal */
    OP_JALR   = 0x67,  /* jalr */
    OP_LUI    = 0x37,  /* load upper immediate */
    OP_AUIPC  = 0x17   /* add upper immediate to pc */
} opcode_t;

/*
 * Not every instruction prints the same way. R-type wants "op rd, rs1, rs2",
 * loads/stores want offset(base) syntax, branches compare two registers
 * against a relative offset, etc. Tagging the decoded struct with its format
 * lets the printer pick the right layout without re-deriving it from the
 * opcode every time.
 */
typedef enum {
    FMT_R,
    FMT_I_ARITH,
    FMT_I_LOAD,
    FMT_I_JALR,
    FMT_S,
    FMT_B,
    FMT_U,
    FMT_J,
    FMT_UNKNOWN
} instr_format_t;

typedef struct {
    uint32_t opcode;
    uint32_t rd;
    uint32_t funct3;
    uint32_t rs1;
    uint32_t rs2;
    uint32_t funct7;
    int32_t  imm;
    instr_format_t format;
    char mnemonic[8];   /* "add", "addi", "beq", etc. "???" if we don't recognize it */
    int valid;          /* 0 if the decoder couldn't identify the instruction */
} decoded_instr_t;

/* fills out everything in 'out' based on raw_instr. always succeeds; check out->valid */
void decode_instruction(uint32_t raw_instr, decoded_instr_t *out);

/* prints one line of assembly for the given address + already-decoded instruction */
void print_decoded(uint32_t pc, uint32_t raw_instr, const decoded_instr_t *d);

#endif
