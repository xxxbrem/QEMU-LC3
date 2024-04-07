/*
 * QEMU LC3 CPU
 *
 * Copyright (c) 2019-2020 Michael Rolnik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/lgpl-2.1.html>
 */

#include "qemu/osdep.h"
#include "qemu/qemu-print.h"
#include "tcg/tcg.h"
#include "cpu.h"
#include "exec/exec-all.h"
#include "tcg/tcg-op.h"
#include "exec/cpu_ldst.h"
#include "exec/helper-proto.h"
#include "exec/helper-gen.h"
#include "exec/log.h"
#include "exec/translator.h"

#define HELPER_H "helper.h"
#include "exec/helper-info.c.inc"
#undef  HELPER_H


/*
 *  Define if you want a BREAK instruction translated to a breakpoint
 *  Active debugging connection is assumed
 *  This is for
 *  https://github.com/seharris/qemu-lc3-tests/tree/master/instruction-tests
 *  tests
 */
#undef BREAKPOINT_ON_BREAK

static TCGv cpu_pc;

// static TCGv cpu_Cf;
// static TCGv cpu_Zf;
// static TCGv cpu_Nf;
// static TCGv cpu_Vf;
// static TCGv cpu_Sf;
// static TCGv cpu_Hf;
// static TCGv cpu_Tf;
// static TCGv cpu_If;

// static TCGv cpu_rampD;
// static TCGv cpu_rampX;
// static TCGv cpu_rampY;
// static TCGv cpu_rampZ;

// static TCGv cpu_r[NUMBER_OF_CPU_REGISTERS];
// static TCGv cpu_eind;
// static TCGv cpu_sp;

static TCGv cpu_skip;

// static const char reg_names[NUMBER_OF_CPU_REGISTERS][8] = {
//     "r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",
//     "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",
//     "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
//     "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31",
// };
#define REG(x) (cpu_r[x])

#define DISAS_EXIT   DISAS_TARGET_0  /* We want return to the cpu main loop.  */
#define DISAS_LOOKUP DISAS_TARGET_1  /* We have a variable condition exit.  */
#define DISAS_CHAIN  DISAS_TARGET_2  /* We have a single condition exit.  */

typedef struct DisasContext DisasContext;

/* This is the state at translation time. */
struct DisasContext {
    DisasContextBase base;

    CPULC3State *env;
    CPUState *cs;

    target_long npc;
    uint32_t opcode;

    /* Routine used to access memory */
    int memidx;

    /*
     * some LC3 instructions can make the following instruction to be skipped
     * Let's name those instructions
     *     A   - instruction that can skip the next one
     *     B   - instruction that can be skipped. this depends on execution of A
     * there are two scenarios
     * 1. A and B belong to the same translation block
     * 2. A is the last instruction in the translation block and B is the last
     *
     * following variables are used to simplify the skipping logic, they are
     * used in the following manner (sketch)
     *
     * TCGLabel *skip_label = NULL;
     * if (ctx->skip_cond != TCG_COND_NEVER) {
     *     skip_label = gen_new_label();
     *     tcg_gen_brcond_tl(skip_cond, skip_var0, skip_var1, skip_label);
     * }
     *
     * translate(ctx);
     *
     * if (skip_label) {
     *     gen_set_label(skip_label);
     * }
     */
    TCGv skip_var0;
    TCGv skip_var1;
    TCGCond skip_cond;
};

void lc3_cpu_tcg_init(void)
{
    // int i;

#define LC3_REG_OFFS(x) offsetof(CPULC3State, x)
    cpu_pc = tcg_global_mem_new_i32(cpu_env, LC3_REG_OFFS(pc_w), "pc");
    // cpu_Cf = tcg_global_mem_new_i32(cpu_env, LC3_REG_OFFS(sregC), "Cf");
    // cpu_Zf = tcg_global_mem_new_i32(cpu_env, LC3_REG_OFFS(sregZ), "Zf");
    // cpu_Nf = tcg_global_mem_new_i32(cpu_env, LC3_REG_OFFS(sregN), "Nf");
    // cpu_Vf = tcg_global_mem_new_i32(cpu_env, LC3_REG_OFFS(sregV), "Vf");
    // cpu_Sf = tcg_global_mem_new_i32(cpu_env, LC3_REG_OFFS(sregS), "Sf");
    // cpu_Hf = tcg_global_mem_new_i32(cpu_env, LC3_REG_OFFS(sregH), "Hf");
    // cpu_Tf = tcg_global_mem_new_i32(cpu_env, LC3_REG_OFFS(sregT), "Tf");
    // cpu_If = tcg_global_mem_new_i32(cpu_env, LC3_REG_OFFS(sregI), "If");
    // cpu_rampD = tcg_global_mem_new_i32(cpu_env, LC3_REG_OFFS(rampD), "rampD");
    // cpu_rampX = tcg_global_mem_new_i32(cpu_env, LC3_REG_OFFS(rampX), "rampX");
    // cpu_rampY = tcg_global_mem_new_i32(cpu_env, LC3_REG_OFFS(rampY), "rampY");
    // cpu_rampZ = tcg_global_mem_new_i32(cpu_env, LC3_REG_OFFS(rampZ), "rampZ");
    // cpu_eind = tcg_global_mem_new_i32(cpu_env, LC3_REG_OFFS(eind), "eind");
    // cpu_sp = tcg_global_mem_new_i32(cpu_env, LC3_REG_OFFS(sp), "sp");
    cpu_skip = tcg_global_mem_new_i32(cpu_env, LC3_REG_OFFS(skip), "skip");

    // for (i = 0; i < NUMBER_OF_CPU_REGISTERS; i++) {
    //     cpu_r[i] = tcg_global_mem_new_i32(cpu_env, LC3_REG_OFFS(r[i]),
    //                                       reg_names[i]);
    // }
#undef LC3_REG_OFFS
}

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

static uint16_t next_word(DisasContext *ctx)
{
    return cpu_lduw_code(ctx->env, ctx->npc++ * 2);
}

// static int append_16(DisasContext *ctx, int x)
// {
//     return x << 16 | next_word(ctx);
// }

// static bool lc3_have_feature(DisasContext *ctx, int feature)
// {
//     if (!lc3_feature(ctx->env, feature)) {
//         gen_helper_unsupported(cpu_env);
//         ctx->base.is_jmp = DISAS_NORETURN;
//         return false;
//     }
//     return true;
// }

static bool decode_insn(DisasContext *ctx, uint16_t insn);
#include "decode-insn.c.inc"

static void gen_goto_tb(DisasContext *ctx, int n, target_ulong dest)
{
    const TranslationBlock *tb = ctx->base.tb;

    if (translator_use_goto_tb(&ctx->base, dest)) {
        tcg_gen_goto_tb(n);
        tcg_gen_movi_i32(cpu_pc, dest);
        tcg_gen_exit_tb(tb, n);
    } else {
        tcg_gen_movi_i32(cpu_pc, dest);
        tcg_gen_lookup_and_goto_ptr();
    }
    ctx->base.is_jmp = DISAS_NORETURN;
}


/*
 * Arithmetic Instructions
 */

static bool trans_ADD(DisasContext *ctx, arg_ADD *a)
{
    return true;
}

static bool trans_ADDI(DisasContext *ctx, arg_ADDI *a)
{
    return true;
}

static bool trans_AND(DisasContext *ctx, arg_AND *a)
{
    return true;
}

static bool trans_ANDI(DisasContext *ctx, arg_ANDI *a)
{
    return true;
}

static bool trans_NOT(DisasContext *ctx, arg_NOT *a)
{
    return true;
}

static bool trans_BR(DisasContext *ctx, arg_BR *a)
{
    return true;
}

static bool trans_JMP(DisasContext *ctx, arg_JMP *a)
{
    return true;
}

static bool trans_JSR(DisasContext *ctx, arg_JSR *a)
{
    return true;
}

static bool trans_JSRR(DisasContext *ctx, arg_JSRR *a)
{
    return true;
}

static bool trans_LD(DisasContext *ctx, arg_LD *a)
{
    return true;
}

static bool trans_LDI(DisasContext *ctx, arg_LDI *a)
{
    return true;
}

static bool trans_LDR(DisasContext *ctx, arg_LDR *a)
{
    return true;
}

static bool trans_LEA(DisasContext *ctx, arg_LEA *a)
{
    return true;
}

static bool trans_ST(DisasContext *ctx, arg_ST *a)
{
    return true;
}

static bool trans_STI(DisasContext *ctx, arg_STI *a)
{
    return true;
}

static bool trans_STR(DisasContext *ctx, arg_STR *a)
{
    return true;
}

// static bool trans_RET(DisasContext *ctx, arg_MOV *a)
// {
//     return true;
// }

static bool trans_TRAP(DisasContext *ctx, arg_TRAP *a)
{
    return true;
}

static bool trans_RTI(DisasContext *ctx, arg_RTI *a)
{
    return true;
}

/*
 *  Core translation mechanism functions:
 *
 *    - translate()
 *    - canonicalize_skip()
 *    - gen_intermediate_code()
 *    - restore_state_to_opc()
 *
 */
static void translate(DisasContext *ctx)
{
    uint32_t opcode = next_word(ctx);

    if (!decode_insn(ctx, opcode)) {
        gen_helper_unsupported(cpu_env);
        ctx->base.is_jmp = DISAS_NORETURN;
    }
}

/* Standardize the cpu_skip condition to NE.  */
static bool canonicalize_skip(DisasContext *ctx)
{
    switch (ctx->skip_cond) {
    case TCG_COND_NEVER:
        /* Normal case: cpu_skip is known to be false.  */
        return false;

    case TCG_COND_ALWAYS:
        /*
         * Breakpoint case: cpu_skip is known to be true, via TB_FLAGS_SKIP.
         * The breakpoint is on the instruction being skipped, at the start
         * of the TranslationBlock.  No need to update.
         */
        return false;

    case TCG_COND_NE:
        if (ctx->skip_var1 == NULL) {
            tcg_gen_mov_tl(cpu_skip, ctx->skip_var0);
        } else {
            tcg_gen_xor_tl(cpu_skip, ctx->skip_var0, ctx->skip_var1);
            ctx->skip_var1 = NULL;
        }
        break;

    default:
        /* Convert to a NE condition vs 0. */
        if (ctx->skip_var1 == NULL) {
            tcg_gen_setcondi_tl(ctx->skip_cond, cpu_skip, ctx->skip_var0, 0);
        } else {
            tcg_gen_setcond_tl(ctx->skip_cond, cpu_skip,
                               ctx->skip_var0, ctx->skip_var1);
            ctx->skip_var1 = NULL;
        }
        ctx->skip_cond = TCG_COND_NE;
        break;
    }
    ctx->skip_var0 = cpu_skip;
    return true;
}

static void lc3_tr_init_disas_context(DisasContextBase *dcbase, CPUState *cs)
{
    DisasContext *ctx = container_of(dcbase, DisasContext, base);
    CPULC3State *env = cs->env_ptr;
    uint32_t tb_flags = ctx->base.tb->flags;

    ctx->cs = cs;
    ctx->env = env;
    ctx->npc = ctx->base.pc_first / 2;

    ctx->skip_cond = TCG_COND_NEVER;
    if (tb_flags & TB_FLAGS_SKIP) {
        ctx->skip_cond = TCG_COND_ALWAYS;
        ctx->skip_var0 = cpu_skip;
    }

    if (tb_flags & TB_FLAGS_FULL_ACCESS) {
        /*
         * This flag is set by ST/LD instruction we will regenerate it ONLY
         * with mem/cpu memory access instead of mem access
         */
        ctx->base.max_insns = 1;
    }
}

static void lc3_tr_tb_start(DisasContextBase *db, CPUState *cs)
{
}

static void lc3_tr_insn_start(DisasContextBase *dcbase, CPUState *cs)
{
    DisasContext *ctx = container_of(dcbase, DisasContext, base);

    tcg_gen_insn_start(ctx->npc);
}

static void lc3_tr_translate_insn(DisasContextBase *dcbase, CPUState *cs)
{
    DisasContext *ctx = container_of(dcbase, DisasContext, base);
    TCGLabel *skip_label = NULL;

    /* Conditionally skip the next instruction, if indicated.  */
    if (ctx->skip_cond != TCG_COND_NEVER) {
        skip_label = gen_new_label();
        if (ctx->skip_var0 == cpu_skip) {
            /*
             * Copy cpu_skip so that we may zero it before the branch.
             * This ensures that cpu_skip is non-zero after the label
             * if and only if the skipped insn itself sets a skip.
             */
            ctx->skip_var0 = tcg_temp_new();
            tcg_gen_mov_tl(ctx->skip_var0, cpu_skip);
            tcg_gen_movi_tl(cpu_skip, 0);
        }
        if (ctx->skip_var1 == NULL) {
            tcg_gen_brcondi_tl(ctx->skip_cond, ctx->skip_var0, 0, skip_label);
        } else {
            tcg_gen_brcond_tl(ctx->skip_cond, ctx->skip_var0,
                              ctx->skip_var1, skip_label);
            ctx->skip_var1 = NULL;
        }
        ctx->skip_cond = TCG_COND_NEVER;
        ctx->skip_var0 = NULL;
    }

    translate(ctx);

    ctx->base.pc_next = ctx->npc * 2;

    if (skip_label) {
        canonicalize_skip(ctx);
        gen_set_label(skip_label);

        switch (ctx->base.is_jmp) {
        case DISAS_NORETURN:
            ctx->base.is_jmp = DISAS_CHAIN;
            break;
        case DISAS_NEXT:
            if (ctx->base.tb->flags & TB_FLAGS_SKIP) {
                ctx->base.is_jmp = DISAS_TOO_MANY;
            }
            break;
        default:
            break;
        }
    }

    if (ctx->base.is_jmp == DISAS_NEXT) {
        target_ulong page_first = ctx->base.pc_first & TARGET_PAGE_MASK;

        if ((ctx->base.pc_next - page_first) >= TARGET_PAGE_SIZE - 4) {
            ctx->base.is_jmp = DISAS_TOO_MANY;
        }
    }
}

static void lc3_tr_tb_stop(DisasContextBase *dcbase, CPUState *cs)
{
    DisasContext *ctx = container_of(dcbase, DisasContext, base);
    bool nonconst_skip = canonicalize_skip(ctx);
    /*
     * Because we disable interrupts while env->skip is set,
     * we must return to the main loop to re-evaluate afterward.
     */
    bool force_exit = ctx->base.tb->flags & TB_FLAGS_SKIP;

    switch (ctx->base.is_jmp) {
    case DISAS_NORETURN:
        assert(!nonconst_skip);
        break;
    case DISAS_NEXT:
    case DISAS_TOO_MANY:
    case DISAS_CHAIN:
        if (!nonconst_skip && !force_exit) {
            /* Note gen_goto_tb checks singlestep.  */
            gen_goto_tb(ctx, 1, ctx->npc);
            break;
        }
        tcg_gen_movi_tl(cpu_pc, ctx->npc);
        /* fall through */
    case DISAS_LOOKUP:
        if (!force_exit) {
            tcg_gen_lookup_and_goto_ptr();
            break;
        }
        /* fall through */
    case DISAS_EXIT:
        tcg_gen_exit_tb(NULL, 0);
        break;
    default:
        g_assert_not_reached();
    }
}

static void lc3_tr_disas_log(const DisasContextBase *dcbase,
                             CPUState *cs, FILE *logfile)
{
    fprintf(logfile, "IN: %s\n", lookup_symbol(dcbase->pc_first));
    target_disas(logfile, cs, dcbase->pc_first, dcbase->tb->size);
}

static const TranslatorOps lc3_tr_ops = {
    .init_disas_context = lc3_tr_init_disas_context,
    .tb_start           = lc3_tr_tb_start,
    .insn_start         = lc3_tr_insn_start,
    .translate_insn     = lc3_tr_translate_insn,
    .tb_stop            = lc3_tr_tb_stop,
    .disas_log          = lc3_tr_disas_log,
};

void gen_intermediate_code(CPUState *cs, TranslationBlock *tb, int *max_insns,
                           target_ulong pc, void *host_pc)
{
    DisasContext dc = { };
    translator_loop(cs, tb, max_insns, pc, host_pc, &lc3_tr_ops, &dc.base);
}
