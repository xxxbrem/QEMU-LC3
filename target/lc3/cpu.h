/*
 * QEMU AVR CPU
 *
 * Copyright (c) 2016-2020 Michael Rolnik
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

#ifndef QEMU_LC3_CPU_H
#define QEMU_LC3_CPU_H

#include "cpu-qom.h"
#include "exec/cpu-defs.h"

#ifdef CONFIG_USER_ONLY
#error "AVR 8-bit does not support user mode"
#endif

#define LC3_CPU_TYPE_SUFFIX "-" TYPE_LC3_CPU
#define LC3_CPU_TYPE_NAME(name) (name LC3_CPU_TYPE_SUFFIX)
#define CPU_RESOLVING_TYPE TYPE_LC3_CPU

#define TCG_GUEST_DEFAULT_MO 0

/*
 * AVR has two memory spaces, data & code.
 * e.g. both have 0 address
 * ST/LD instructions access data space
 * LPM/SPM and instruction fetching access code memory space
 */
#define MMU_CODE_IDX 0
#define MMU_DATA_IDX 1

#define EXCP_RESET 1
#define EXCP_INT(n) (EXCP_RESET + (n) + 1)

/* Number of CPU registers */
#define NUMBER_OF_CPU_REGISTERS 10
/* Number of IO registers accessible by ld/st/in/out */
#define NUMBER_OF_IO_REGISTERS 64

/*
 * Offsets of AVR memory regions in host memory space.
 *
 * This is needed because the AVR has separate code and data address
 * spaces that both have start from zero but have to go somewhere in
 * host memory.
 *
 * It's also useful to know where some things are, like the IO registers.
 */
/* Flash program memory */
#define OFFSET_CODE 0x00000000
/* CPU registers, IO registers, and SRAM */
#define OFFSET_DATA 0x00800000
/* CPU registers specifically, these are mapped at the start of data */
#define OFFSET_CPU_REGISTERS OFFSET_DATA
/*
 * IO registers, including status register, stack pointer, and memory
 * mapped peripherals, mapped just after CPU registers
 */
#define OFFSET_IO_REGISTERS (OFFSET_DATA + NUMBER_OF_CPU_REGISTERS)

enum
{
    MR_KBSR = 0xFE00, /* keyboard status */
    MR_KBDR = 0xFE02,  /* keyboard data */
    KBSR_V = 65534,
};

enum
{
    R_R0 = 0,
    R_R1,
    R_R2,
    R_R3,
    R_R4,
    R_R5,
    R_R6,
    R_R7,
    R_PC, /* program counter */
    R_COND,
    R_COUNT
};

typedef enum LC3Feature {
    LC3_FEATURE_SRAM,

    LC3_FEATURE_1_BYTE_PC,
    LC3_FEATURE_2_BYTE_PC,
    LC3_FEATURE_3_BYTE_PC,

    LC3_FEATURE_1_BYTE_SP,
    LC3_FEATURE_2_BYTE_SP,

    LC3_FEATURE_BREAK,
    LC3_FEATURE_DES,
    LC3_FEATURE_RMW, /* Read Modify Write - XCH LAC LAS LAT */

    LC3_FEATURE_EIJMP_EICALL,
    LC3_FEATURE_IJMP_ICALL,
    LC3_FEATURE_JMP_CALL,

    LC3_FEATURE_ADIW_SBIW,

    LC3_FEATURE_SPM,
    LC3_FEATURE_SPMX,

    LC3_FEATURE_ELPMX,
    LC3_FEATURE_ELPM,
    LC3_FEATURE_LPMX,
    LC3_FEATURE_LPM,

    LC3_FEATURE_MOVW,
    LC3_FEATURE_MUL,
    LC3_FEATURE_RAMPD,
    LC3_FEATURE_RAMPX,
    LC3_FEATURE_RAMPY,
    LC3_FEATURE_RAMPZ,
} LC3Feature;

typedef struct CPUArchState {
    uint32_t pc_w; /* 0x003fffff up to 22 bits */

    // uint32_t sregC; /* 0x00000001 1 bit */
    // uint32_t sregZ; /* 0x00000001 1 bit */
    // uint32_t sregN; /* 0x00000001 1 bit */
    // uint32_t sregV; /* 0x00000001 1 bit */
    // uint32_t sregS; /* 0x00000001 1 bit */
    // uint32_t sregH; /* 0x00000001 1 bit */
    // uint32_t sregT; /* 0x00000001 1 bit */
    uint32_t sregI; /* 0x00000001 1 bit */

    // uint32_t rampD; /* 0x00ff0000 8 bits */
    // uint32_t rampX; /* 0x00ff0000 8 bits */
    // uint32_t rampY; /* 0x00ff0000 8 bits */
    // uint32_t rampZ; /* 0x00ff0000 8 bits */
    // uint32_t eind; /* 0x00ff0000 8 bits */

    uint32_t r[NUMBER_OF_CPU_REGISTERS]; /* 8 bits each */
    uint32_t sp; /* 16 bits */

    uint32_t skip; /* if set skip instruction */

    uint64_t intsrc; /* interrupt sources */
    bool fullacc; /* CPU/MEM if true MEM only otherwise */

    uint64_t features;
} CPULC3State;

/**
 *  AVRCPU:
 *  @env: #CPULC3State
 *
 *  A AVR CPU.
 */
struct ArchCPU {
    /*< private >*/
    CPUState parent_obj;
    /*< public >*/

    CPUNegativeOffsetState neg;
    CPULC3State env;
};

extern const struct VMStateDescription vms_lc3_cpu;

void lc3_cpu_do_interrupt(CPUState *cpu);
bool lc3_cpu_exec_interrupt(CPUState *cpu, int int_req);
hwaddr lc3_cpu_get_phys_page_debug(CPUState *cpu, vaddr addr);
int lc3_cpu_gdb_read_register(CPUState *cpu, GByteArray *buf, int reg);
int lc3_cpu_gdb_write_register(CPUState *cpu, uint8_t *buf, int reg);
int lc3_print_insn(bfd_vma addr, disassemble_info *info);
vaddr lc3_cpu_gdb_adjust_breakpoint(CPUState *cpu, vaddr addr);

static inline int lc3_feature(CPULC3State *env, LC3Feature feature)
{
    return (env->features & (1U << feature)) != 0;
}

static inline void set_lc3_feature(CPULC3State *env, int feature)
{
    // env->features |= (1U << feature);
}

#define cpu_list lc3_cpu_list
#define cpu_mmu_index lc3_cpu_mmu_index

static inline int lc3_cpu_mmu_index(CPULC3State *env, bool ifetch)
{
    return ifetch ? MMU_CODE_IDX : MMU_DATA_IDX;
}

void lc3_cpu_tcg_init(void);

void lc3_cpu_list(void);
// int cpu_avr_exec(CPUState *cpu);

enum {
    TB_FLAGS_FULL_ACCESS = 1,
    TB_FLAGS_SKIP = 2,
};

static inline void cpu_get_tb_cpu_state(CPULC3State *env, vaddr *pc,
                                        uint64_t *cs_base, uint32_t *pflags)
{
    uint32_t flags = 0;

    *pc = env->pc_w * 2;
    *cs_base = 0;

    if (env->fullacc) {
        flags |= TB_FLAGS_FULL_ACCESS;
    }
    if (env->skip) {
        flags |= TB_FLAGS_SKIP;
    }

    *pflags = flags;
}

static inline int cpu_interrupts_enabled(CPULC3State *env)
{
    return env->sregI != 0;
}

// static inline uint8_t cpu_get_sreg(CPULC3State *env)
// {
//     return (env->sregC) << 0
//          | (env->sregZ) << 1
//          | (env->sregN) << 2
//          | (env->sregV) << 3
//          | (env->sregS) << 4
//          | (env->sregH) << 5
//          | (env->sregT) << 6
//          | (env->sregI) << 7;
// }

// static inline void cpu_set_sreg(CPULC3State *env, uint8_t sreg)
// {
//     env->sregC = (sreg >> 0) & 0x01;
//     env->sregZ = (sreg >> 1) & 0x01;
//     env->sregN = (sreg >> 2) & 0x01;
//     env->sregV = (sreg >> 3) & 0x01;
//     env->sregS = (sreg >> 4) & 0x01;
//     env->sregH = (sreg >> 5) & 0x01;
//     env->sregT = (sreg >> 6) & 0x01;
//     env->sregI = (sreg >> 7) & 0x01;
// }

bool lc3_cpu_tlb_fill(CPUState *cs, vaddr address, int size,
                      MMUAccessType access_type, int mmu_idx,
                      bool probe, uintptr_t retaddr);

#include "exec/cpu-all.h"

#endif /* QEMU_LC3_CPU_H */
