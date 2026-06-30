#include "decoder.h"
#include <string.h>

/* small helper so we don't repeat strncpy+null-term everywhere below */
static void set_mnemonic(decoded_instr_t *out, const char *name) {
    strncpy(out->mnemonic, name, sizeof(out->mnemonic) - 1);
    out->mnemonic[sizeof(out->mnemonic) - 1] = '\0';
}

static void decode_r_type(uint32_t raw, decoded_instr_t *out) {
    out->format = FMT_R;
    out->valid = 1;

    switch (out->funct3) {
        case 0x0: set_mnemonic(out, out->funct7 == 0x20 ? "sub" : "add"); break;
        case 0x1: set_mnemonic(out, "sll");  break;
        case 0x2: set_mnemonic(out, "slt");  break;
        case 0x3: set_mnemonic(out, "sltu"); break;
        case 0x4: set_mnemonic(out, "xor");  break;
        case 0x5: set_mnemonic(out, out->funct7 == 0x20 ? "sra" : "srl"); break;
        case 0x6: set_mnemonic(out, "or");   break;
        case 0x7: set_mnemonic(out, "and");  break;
        default:
            out->valid = 0;
            out->format = FMT_UNKNOWN;
            set_mnemonic(out, "???");
    }
    (void)raw;
}

static void decode_i_arith(uint32_t raw, decoded_instr_t *out) {
    out->format = FMT_I_ARITH;
    out->valid = 1;

    /* funct3 == 1 or 5 means this is actually a shift, where the low 5 bits
     * of the immediate are the shift amount and funct7 picks SRL vs SRA */
    if (out->funct3 == 0x1 || out->funct3 == 0x5) {
        uint32_t shamt = EXTRACT_BITS(raw, 24, 20);
        uint32_t shift_funct7 = EXTRACT_BITS(raw, 31, 25);
        out->imm = (int32_t)shamt;
        if (out->funct3 == 0x1) {
            set_mnemonic(out, "slli");
        } else {
            set_mnemonic(out, shift_funct7 == 0x20 ? "srai" : "srli");
        }
        return;
    }

    out->imm = sign_extend(EXTRACT_BITS(raw, 31, 20), 12);

    switch (out->funct3) {
        case 0x0: set_mnemonic(out, "addi");  break;
        case 0x2: set_mnemonic(out, "slti");  break;
        case 0x3: set_mnemonic(out, "sltiu"); break;
        case 0x4: set_mnemonic(out, "xori");  break;
        case 0x6: set_mnemonic(out, "ori");   break;
        case 0x7: set_mnemonic(out, "andi");  break;
        default:
            out->valid = 0;
            out->format = FMT_UNKNOWN;
            set_mnemonic(out, "???");
    }
}

static void decode_load(uint32_t raw, decoded_instr_t *out) {
    out->format = FMT_I_LOAD;
    out->valid = 1;
    out->imm = sign_extend(EXTRACT_BITS(raw, 31, 20), 12);

    switch (out->funct3) {
        case 0x0: set_mnemonic(out, "lb");  break;
        case 0x1: set_mnemonic(out, "lh");  break;
        case 0x2: set_mnemonic(out, "lw");  break;
        case 0x4: set_mnemonic(out, "lbu"); break;
        case 0x5: set_mnemonic(out, "lhu"); break;
        default:
            out->valid = 0;
            out->format = FMT_UNKNOWN;
            set_mnemonic(out, "???");
    }
}

static void decode_store(uint32_t raw, decoded_instr_t *out) {
    out->format = FMT_S;
    out->valid = 1;

    /* S-type splits the immediate across two fields - bits[11:5] live up by
     * funct7 and bits[4:0] live where rd normally would */
    uint32_t imm_hi = EXTRACT_BITS(raw, 31, 25);
    uint32_t imm_lo = EXTRACT_BITS(raw, 11, 7);
    out->imm = sign_extend((imm_hi << 5) | imm_lo, 12);

    switch (out->funct3) {
        case 0x0: set_mnemonic(out, "sb"); break;
        case 0x1: set_mnemonic(out, "sh"); break;
        case 0x2: set_mnemonic(out, "sw"); break;
        default:
            out->valid = 0;
            out->format = FMT_UNKNOWN;
            set_mnemonic(out, "???");
    }
}

static void decode_branch(uint32_t raw, decoded_instr_t *out) {
    out->format = FMT_B;
    out->valid = 1;

    /* B-type immediate bits are scattered all over the instruction in a
     * weird order (imm[12|10:5|4:1|11]) so the hardware can decode rs1/rs2
     * without waiting on the immediate to be assembled. Bit 0 is always 0
     * since branch targets are 2-byte aligned, so it's just left out. */
    uint32_t bit12   = EXTRACT_BITS(raw, 31, 31);
    uint32_t bit11   = EXTRACT_BITS(raw, 7, 7);
    uint32_t bits105 = EXTRACT_BITS(raw, 30, 25);
    uint32_t bits41  = EXTRACT_BITS(raw, 11, 8);

    uint32_t imm = (bit12 << 12) | (bit11 << 11) | (bits105 << 5) | (bits41 << 1);
    out->imm = sign_extend(imm, 13);

    switch (out->funct3) {
        case 0x0: set_mnemonic(out, "beq");  break;
        case 0x1: set_mnemonic(out, "bne");  break;
        case 0x4: set_mnemonic(out, "blt");  break;
        case 0x5: set_mnemonic(out, "bge");  break;
        case 0x6: set_mnemonic(out, "bltu"); break;
        case 0x7: set_mnemonic(out, "bgeu"); break;
        default:
            out->valid = 0;
            out->format = FMT_UNKNOWN;
            set_mnemonic(out, "???");
    }
}

static void decode_jal(uint32_t raw, decoded_instr_t *out) {
    out->format = FMT_J;
    out->valid = 1;
    set_mnemonic(out, "jal");

    /* same idea as B-type but scrambled differently: imm[20|10:1|11|19:12] */
    uint32_t bit20    = EXTRACT_BITS(raw, 31, 31);
    uint32_t bits1912 = EXTRACT_BITS(raw, 19, 12);
    uint32_t bit11    = EXTRACT_BITS(raw, 20, 20);
    uint32_t bits101  = EXTRACT_BITS(raw, 30, 21);

    uint32_t imm = (bit20 << 20) | (bits1912 << 12) | (bit11 << 11) | (bits101 << 1);
    out->imm = sign_extend(imm, 21);
}

static void decode_jalr(uint32_t raw, decoded_instr_t *out) {
    out->format = FMT_I_JALR;
    out->imm = sign_extend(EXTRACT_BITS(raw, 31, 20), 12);

    if (out->funct3 == 0x0) {
        out->valid = 1;
        set_mnemonic(out, "jalr");
    } else {
        out->valid = 0;
        out->format = FMT_UNKNOWN;
        set_mnemonic(out, "???");
    }
}

static void decode_upper(uint32_t raw, decoded_instr_t *out, int is_auipc) {
    out->format = FMT_U;
    out->valid = 1;
    set_mnemonic(out, is_auipc ? "auipc" : "lui");
    /* assembly syntax shows the raw 20-bit field, not the shifted-by-12
     * value that actually ends up in the register - that shift happens at
     * execution time, not in the encoding */
    out->imm = (int32_t)EXTRACT_BITS(raw, 31, 12);
}

void decode_instruction(uint32_t raw_instr, decoded_instr_t *out) {
    memset(out, 0, sizeof(*out));

    out->opcode = EXTRACT_BITS(raw_instr, 6, 0);
    out->rd     = EXTRACT_BITS(raw_instr, 11, 7);
    out->funct3 = EXTRACT_BITS(raw_instr, 14, 12);
    out->rs1    = EXTRACT_BITS(raw_instr, 19, 15);
    out->rs2    = EXTRACT_BITS(raw_instr, 24, 20);
    out->funct7 = EXTRACT_BITS(raw_instr, 31, 25);

    switch ((opcode_t)out->opcode) {
        case OP_R_TYPE: decode_r_type(raw_instr, out);        break;
        case OP_I_TYPE: decode_i_arith(raw_instr, out);       break;
        case OP_LOAD:   decode_load(raw_instr, out);          break;
        case OP_STORE:  decode_store(raw_instr, out);         break;
        case OP_BRANCH: decode_branch(raw_instr, out);        break;
        case OP_JAL:    decode_jal(raw_instr, out);           break;
        case OP_JALR:   decode_jalr(raw_instr, out);          break;
        case OP_LUI:    decode_upper(raw_instr, out, 0);      break;
        case OP_AUIPC:  decode_upper(raw_instr, out, 1);      break;
        default:
            out->valid = 0;
            out->format = FMT_UNKNOWN;
            set_mnemonic(out, "???");
    }
}

void print_decoded(uint32_t pc, uint32_t raw_instr, const decoded_instr_t *d) {
    printf("0x%08X %08X    ", pc, raw_instr);

    if (!d->valid) {
        printf("UNKNOWN\n");
        return;
    }

    switch (d->format) {
        case FMT_R:
            printf("%-7s x%u, x%u, x%u\n", d->mnemonic, d->rd, d->rs1, d->rs2);
            break;
        case FMT_I_ARITH:
            printf("%-7s x%u, x%u, %d\n", d->mnemonic, d->rd, d->rs1, d->imm);
            break;
        case FMT_I_LOAD:
        case FMT_I_JALR:
            printf("%-7s x%u, %d(x%u)\n", d->mnemonic, d->rd, d->imm, d->rs1);
            break;
        case FMT_S:
            printf("%-7s x%u, %d(x%u)\n", d->mnemonic, d->rs2, d->imm, d->rs1);
            break;
        case FMT_B:
            printf("%-7s x%u, x%u, %d\n", d->mnemonic, d->rs1, d->rs2, d->imm);
            break;
        case FMT_U:
            printf("%-7s x%u, 0x%X\n", d->mnemonic, d->rd, (uint32_t)d->imm);
            break;
        case FMT_J:
            printf("%-7s x%u, %d\n", d->mnemonic, d->rd, d->imm);
            break;
        default:
            printf("UNKNOWN\n");
    }
}
