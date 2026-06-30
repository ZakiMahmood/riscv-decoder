# Design Notes

## Overall structure

The decoder is split into three pieces:

- `memory.c` just deals with getting bytes off disk and into a simulated
  memory array (little-endian, like real RISC-V).
- `decoder.c` is the actual brains - takes a raw 32-bit word and figures
  out what it is.
- `main.c` glues them together: load the file, loop over the words,
  reassemble each 4-byte chunk back into a 32-bit instruction, decode it,
  print it.

I kept `decode_instruction()` separate from `print_decoded()` on purpose.
The decode step fills in a `decoded_instr_t` struct with all the fields
(rd, rs1, rs2, imm, mnemonic, etc.) and doesn't touch stdout at all. That
made it possible to write proper unit tests against the struct contents
instead of having to compare printed strings character by character.

## How the opcode dispatch works

Every RV32I instruction has a 7-bit opcode in bits [6:0]. That alone tells
you the *format* (R/I/S/B/U/J) but not the specific instruction - for that
you also need `funct3` (bits [14:12]) and, for R-type and some I-type shift
instructions, `funct7` (bits [31:25]).

So `decode_instruction()` does a two-level switch: first on opcode to pick
the format-specific helper function (`decode_r_type`, `decode_i_arith`,
etc.), then inside each helper, a switch on `funct3` (and sometimes
`funct7`) to pick the actual mnemonic.

One thing that tripped me up initially: ADD and SUB share the exact same
opcode *and* funct3 (0x33, 0x0). The only thing that tells them apart is
bit 30 of the instruction (which shows up as the top bit of funct7 - 0x00
vs 0x20). Same story for SRL/SRA and SRLI/SRAI.

## Immediate encoding is the hard part

Each instruction format packs its immediate bits differently, and B-type
and J-type don't even keep them in order - I'm guessing this is so the
hardware can start decoding rs1/rs2 without waiting for the whole
immediate to be reassembled, but for software it just means more shifting.

- **I-type**: imm is one contiguous 12-bit field at [31:20]. Easy.
- **S-type**: split across [31:25] (high 7 bits) and [11:7] (low 5 bits) -
  because rd's normal slot got reused for the bottom of the immediate.
- **B-type**: imm[12|10:5|4:1|11], and bit 0 isn't stored at all since
  branch targets are always 2-byte aligned.
- **J-type**: imm[20|10:1|11|19:12] - scrambled in a different order than
  B-type.
- **U-type**: imm is just the top 20 bits, used as-is (no sign extension
  needed since it already occupies the full upper half).

All the non-U-type immediates need sign extension afterward since they
represent signed offsets but only occupy part of the word. `sign_extend()`
in `common.h` handles that generically for any bit width.

## EXTRACT_BITS

`EXTRACT_BITS(val, high, low)` shows up constantly throughout `decoder.c`.
It shifts the field down to bit 0 and masks off everything above the
field width. I pulled this into a macro instead of a function mostly
because it gets used so often and the call sites read more naturally as
`EXTRACT_BITS(raw, 11, 7)` than a function call with the same args.

## What "UNKNOWN" actually catches

If the opcode itself isn't one of the 9 RV32I opcodes I handle, the
`default` case in the outer switch marks the instruction invalid
immediately. But it's also possible to have a *valid* opcode with a
funct3 (or funct3+funct7) combination that doesn't correspond to a real
instruction - each format-specific decode function has its own `default`
case for that. Either way `decoded_instr_t.valid` ends up 0 and the
printer just prints `UNKNOWN` instead of trying to guess.

## Known limitations

- No support for RV32M (mul/div) or RV32F (floating point) - out of scope
  for this assignment, RV32I only.
- ECALL/EBREAK (the OP_SYSTEM opcode) aren't decoded - they'd print as
  UNKNOWN currently. Could add as a bonus extension.
- The hex file parser assumes one valid hex word per line and doesn't
  handle comments or addresses in the file - matches the simple format
  used throughout the module.
