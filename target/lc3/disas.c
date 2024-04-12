/*
 * lc3 disassembler
 *
 * Copyright (c) 2019-2020 Richard Henderson <rth@twiddle.net>
 * Copyright (c) 2019-2020 Michael Rolnik <mrolnik@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "qemu/osdep.h"
#include "cpu.h"

typedef struct {
    disassemble_info *info;
    uint16_t next_word;
    bool next_word_used;
} DisasContext;

// static int to_regs_16_31_by_one(DisasContext *ctx, int indx)
// {
//     return 16 + (indx % 16);
// }

// static int to_regs_16_23_by_one(DisasContext *ctx, int indx)
// {
//     return 16 + (indx % 8);
// }

// static int to_regs_24_30_by_two(DisasContext *ctx, int indx)
// {
//     return 24 + (indx % 4) * 2;
// }

// static int to_regs_00_30_by_two(DisasContext *ctx, int indx)
// {
//     return (indx % 16) * 2;
// }

// static uint16_t next_word(DisasContext *ctx)
// {
//     ctx->next_word_used = true;
//     return ctx->next_word;
// }

// static int append_16(DisasContext *ctx, int x)
// {
//     return x << 16 | next_word(ctx);
// }

/* Include the auto-generated decoder.  */
static bool decode_insn(DisasContext *ctx, uint16_t insn);
#include "decode-insn.c.inc"

#define output(mnemonic, format, ...) \
    (pctx->info->fprintf_func(pctx->info->stream, "%-9s " format, \
                              mnemonic, ##__VA_ARGS__))

int lc3_print_insn(bfd_vma addr, disassemble_info *info)
{
    DisasContext ctx;
    DisasContext *pctx = &ctx;
    bfd_byte buffer[4];
    uint16_t insn;
    int status;

    ctx.info = info;

    status = info->read_memory_func(addr, buffer, 4, info);
    if (status != 0) {
        info->memory_error_func(status, addr, info);
        return -1;
    }
    insn = bfd_getl16(buffer);
    ctx.next_word = bfd_getl16(buffer + 2);
    ctx.next_word_used = false;

    if (!decode_insn(&ctx, insn)) {
        output(".db", "0x%02x, 0x%02x", buffer[0], buffer[1]);
    }

    return ctx.next_word_used ? 4 : 2;
}


#define INSN(opcode, format, ...)                                       \
static bool trans_##opcode(DisasContext *pctx, arg_##opcode * a)        \
{                                                                       \
    output(#opcode, format, ##__VA_ARGS__);                             \
    return true;                                                        \
}

#define INSN_MNEMONIC(opcode, mnemonic, format, ...)                    \
static bool trans_##opcode(DisasContext *pctx, arg_##opcode * a)        \
{                                                                       \
    output(mnemonic, format, ##__VA_ARGS__);                            \
    return true;                                                        \
}

/*
 *   C       Z       N       V       S       H       T       I
 *   0       1       2       3       4       5       6       7
 */
// static const char brbc[][5] = {
//     "BRCC", "BRNE", "BRPL", "BRVC", "BRGE", "BRHC", "BRTC", "BRID"
// };

// static const char brbs[][5] = {
//     "BRCS", "BREQ", "BRMI", "BRVS", "BRLT", "BRHS", "BRTS", "BRIE"
// };

// static const char bset[][4] = {
//     "SEC",  "SEZ",  "SEN",  "SEZ",  "SES",  "SEH",  "SET",  "SEI"
// };

// static const char bclr[][4] = {
//     "CLC",  "CLZ",  "CLN",  "CLZ",  "CLS",  "CLH",  "CLT",  "CLI"
// };

/*
 * Arithmetic Instructions
 */
INSN(ADD,    "r%d, r%d, r%d", a->DR, a->SR1, a->SR2)
INSN(ADDI,   "r%d, r%d, r%d", a->DR, a->SR1, a->imm5)
INSN(AND,    "r%d, r%d, r%d", a->DR, a->SR1, a->SR2)
INSN(ANDI,   "r%d, r%d, r%d", a->DR, a->SR1, a->imm5)
INSN(NOT,    "r%d, r%d", a->DR, a->SR)

/*
 * Branch Instructions
 */
INSN(BR,     "r%d, r%d", a->cond, a->PCoffset9)
INSN(JMP,    "r%d", a->BaseR)
INSN(JSR,    "r%d", a->PCoffset11)
INSN(JSRR,   "r%d", a->BaseR)

/*
 * Data Transfer Instructions
 */
INSN(LD,     "r%d, r%d", a->DR, a->PCoffset9)
INSN(LDI,    "r%d, r%d", a->DR, a->PCoffset9)
INSN(LDR,    "r%d, r%d, r%d", a->DR, a->BaseR, a->offset6)
INSN(LEA,    "r%d, r%d", a->DR, a->PCoffset9)
INSN(ST,     "r%d, r%d", a->SR, a->PCoffset9)
INSN(STI,    "r%d, r%d", a->SR, a->PCoffset9)
INSN(STR,    "r%d, r%d, r%d", a->SR, a->BaseR, a->offset6)
// INSN(RET,    "r%d", a->RET)
INSN(RTI, "")
INSN(TRAP_GETC, "")
INSN(TRAP_OUT, "")
INSN(TRAP_PUTS, "")
INSN(TRAP_IN, "")
INSN(TRAP_PUTSP, "")
INSN(TRAP_HALT, "")