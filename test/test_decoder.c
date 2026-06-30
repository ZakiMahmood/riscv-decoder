/*
 * Quick sanity-check tests for decoder.c. Not a real testing framework,
 * just asserts + a pass/fail counter - good enough for catching regressions
 * while working on the bit-field math.
 *
 * Run with: make test   (or just compile + run this file directly)
 */
#include "decoder.h"
#include <string.h>

static int tests_run = 0;
static int tests_failed = 0;

#define CHECK(cond, msg) do { \
    tests_run++; \
    if (!(cond)) { \
        tests_failed++; \
        printf("FAIL: %s (line %d)\n", msg, __LINE__); \
    } \
} while (0)

static void test_add(void) {
    decoded_instr_t d;
    decode_instruction(0x003100B3, &d); /* add x1, x2, x3 */
    CHECK(d.valid, "add: should be valid");
    CHECK(strcmp(d.mnemonic, "add") == 0, "add: mnemonic");
    CHECK(d.rd == 1 && d.rs1 == 2 && d.rs2 == 3, "add: register fields");
}

static void test_sub(void) {
    decoded_instr_t d;
    decode_instruction(0x40310133, &d); /* sub x2, x2, x3 */
    CHECK(d.valid, "sub: should be valid");
    CHECK(strcmp(d.mnemonic, "sub") == 0, "sub: mnemonic");
}

static void test_addi_positive(void) {
    decoded_instr_t d;
    decode_instruction(0x00500113, &d); /* addi x2, x0, 5 */
    CHECK(strcmp(d.mnemonic, "addi") == 0, "addi: mnemonic");
    CHECK(d.imm == 5, "addi: imm should be 5");
}

static void test_addi_negative(void) {
    decoded_instr_t d;
    decode_instruction(0xFFF0A113, &d); /* slti x2, x1, -1 */
    CHECK(strcmp(d.mnemonic, "slti") == 0, "slti: mnemonic");
    CHECK(d.imm == -1, "slti: sign extension of -1 broken");
}

static void test_sw(void) {
    decoded_instr_t d;
    decode_instruction(0x0020A023, &d); /* sw x2, 0(x1) */
    CHECK(strcmp(d.mnemonic, "sw") == 0, "sw: mnemonic");
    CHECK(d.rs1 == 1 && d.rs2 == 2 && d.imm == 0, "sw: fields");
}

static void test_lw(void) {
    decoded_instr_t d;
    decode_instruction(0x0040A103, &d); /* lw x2, 4(x1) */
    CHECK(strcmp(d.mnemonic, "lw") == 0, "lw: mnemonic");
    CHECK(d.imm == 4, "lw: offset should be 4");
}

static void test_branch_negative_offset(void) {
    decoded_instr_t d;
    decode_instruction(0xFE209CE3, &d); /* bne x1, x2, -8 */
    CHECK(strcmp(d.mnemonic, "bne") == 0, "bne: mnemonic");
    CHECK(d.imm == -8, "bne: branch offset sign extension broken");
}

static void test_jal(void) {
    decoded_instr_t d;
    decode_instruction(0x004000EF, &d); /* jal x1, 4 */
    CHECK(strcmp(d.mnemonic, "jal") == 0, "jal: mnemonic");
    CHECK(d.rd == 1 && d.imm == 4, "jal: fields");
}

static void test_lui(void) {
    decoded_instr_t d;
    decode_instruction(0x123452B7, &d); /* lui x5, 0x12345 */
    CHECK(strcmp(d.mnemonic, "lui") == 0, "lui: mnemonic");
    CHECK(d.imm == 0x12345, "lui: should show unshifted 20-bit immediate");
}

static void test_unknown_opcode(void) {
    decoded_instr_t d;
    decode_instruction(0xFFFFFFFF, &d); /* opcode 0x7F doesn't exist in RV32I */
    CHECK(d.valid == 0, "unknown opcode should be marked invalid");
}

static void test_shifts(void) {
    decoded_instr_t d;
    decode_instruction(0x4030D113, &d); /* srai x2, x1, 3 */
    CHECK(strcmp(d.mnemonic, "srai") == 0, "srai: should distinguish from srli via funct7");
    CHECK(d.imm == 3, "srai: shamt should be 3");
}

int main(void) {
    test_add();
    test_sub();
    test_addi_positive();
    test_addi_negative();
    test_sw();
    test_lw();
    test_branch_negative_offset();
    test_jal();
    test_lui();
    test_unknown_opcode();
    test_shifts();

    printf("\n%d/%d tests passed\n", tests_run - tests_failed, tests_run);
    return tests_failed == 0 ? 0 : 1;
}
