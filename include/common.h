#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

/* simulated memory size for the decoder - 64KB is plenty for test programs */
#define MEMORY_SIZE 65536

/* how many bytes one RV32I instruction takes up */
#define INSTR_SIZE 4

/* pulls bits [high:low] out of a 32-bit value, e.g. EXTRACT_BITS(x, 11, 7) for rd */
#define EXTRACT_BITS(val, high, low) \
    (((val) >> (low)) & ((1U << ((high) - (low) + 1)) - 1))

/*
 * Sign-extends a value that's currently bit_width bits wide up to a full
 * 32-bit signed int. Needed for every immediate field in RV32I since they're
 * all narrower than 32 bits but represent signed offsets.
 *
 * Trick: XOR with the sign bit then subtract it back off. Works because if
 * the top bit of val is 1, this effectively fills all the upper bits with 1s.
 */
static inline int32_t sign_extend(uint32_t val, int bit_width) {
    uint32_t sign_bit = 1U << (bit_width - 1);
    return (int32_t)((val ^ sign_bit) - sign_bit);
}

#endif
