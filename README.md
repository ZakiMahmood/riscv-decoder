# riscv-decoder

A command-line RV32I instruction decoder, written for the MEDS Module 2 grand
assignment (Summer Training Programme 2026, Cohort 4). It reads a hex file
containing RISC-V machine code (one 32-bit instruction per line) and prints
the decoded assembly for each instruction.

This is basically the front end of a CPU simulator — fetch the raw word,
pull the opcode/funct3/funct7 fields out of it, figure out which instruction
it is, and print it in a readable form. The actual execution part comes
later in the training (Weeks 3-4).

## Supported instructions

- R-type: add, sub, and, or, xor, sll, srl, sra, slt, sltu
- I-type arithmetic: addi, andi, ori, xori, slti, sltiu, slli, srli, srai
- I-type loads: lb, lh, lw, lbu, lhu
- I-type jump: jalr
- S-type: sb, sh, sw
- B-type: beq, bne, blt, bge, bltu, bgeu
- U-type: lui, auipc
- J-type: jal

Anything that doesn't match a known opcode/funct3/funct7 combination gets
printed as `UNKNOWN` instead of crashing or guessing.

## Building

Needs `gcc` and `make`. Tested with `-std=c11` on Linux.

```
make           # builds bin/riscv-decoder
make test      # runs the unit tests, then a sample decode
make debug     # build with -DDEBUG -O0
make valgrind  # build + run under valgrind to check for leaks
make clean     # remove build/ and bin/
```

## Usage

```
./bin/riscv-decoder <hex_file>
```

The hex file format is just one instruction per line, in hex, no `0x`
prefix needed:

```
00500113
00A00193
003100B3
```

### Example

```
$ ./bin/riscv-decoder test/programs/mixed.hex
RISC-V RV32I Instruction Decoder
================================
Loaded 12 instructions from test/programs/mixed.hex

Addr        Hex         Assembly
----------- ----------- -------------------------
0x00000000 00500113    addi    x2, x0, 5
0x00000004 00A00193    addi    x3, x0, 10
0x00000008 003100B3    add     x1, x2, x3
0x0000000C 40310133    sub     x2, x2, x3
0x00000010 0020A023    sw      x2, 0(x1)
0x00000014 0000A103    lw      x2, 0(x1)
0x00000018 FE209CE3    bne     x1, x2, -8
0x0000001C 004000EF    jal     x1, 4
0x00000020 123452B7    lui     x5, 0x12345
0x00000024 00001297    auipc   x5, 0x1
0x00000028 000280E7    jalr    x1, 0(x5)
0x0000002C FFFFFFFF    UNKNOWN

Decoded 12 instructions (11 valid, 1 unknown)
```

## Project layout

```
riscv-decoder/
├── Makefile
├── include/
│   ├── common.h     # EXTRACT_BITS macro, sign_extend(), shared constants
│   ├── decoder.h     # opcode enum, decoded_instr_t, decode/print prototypes
│   └── memory.h      # hex file loader prototype
├── src/
│   ├── main.c         # CLI entry point
│   ├── decoder.c       # the actual decode logic, one function per format
│   └── memory.c        # loads a hex file into a simulated byte array
├── test/
│   ├── test_decoder.c   # unit tests for decoder.c
│   └── programs/         # sample hex files used for testing
│       ├── r_type.hex
│       ├── i_type.hex
│       ├── branch.hex
│       └── mixed.hex
└── docs/
    └── DESIGN.md
```

See `docs/DESIGN.md` for notes on how the bit-field extraction for each
instruction format works.

## Testing

`test/test_decoder.c` has a handful of assert-style checks against known
instruction encodings (mostly cross-checked against the RISC-V spec's
example encodings and the Venus simulator). Run them with `make test`.

There's also no memory leaks - `make valgrind` confirms a clean run.
