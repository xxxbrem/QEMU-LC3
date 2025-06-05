/*
 * QEMU LC3 CPU helpers
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

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/error-report.h"
#include "cpu.h"
#include "hw/core/tcg-cpu-ops.h"
#include "exec/exec-all.h"
#include "exec/address-spaces.h"
#include "exec/helper-proto.h"

bool lc3_cpu_exec_interrupt(CPUState *cs, int interrupt_request)
{
    LC3CPU *cpu = LC3_CPU(cs);
    CPULC3State *env = &cpu->env;

    /*
     * We cannot separate a skip from the next instruction,
     * as the skip would not be preserved across the interrupt.
     * Separating the two insn normally only happens at page boundaries.
     */
    if (env->skip) {
        return false;
    }

    if (interrupt_request & CPU_INTERRUPT_RESET) {
        if (cpu_interrupts_enabled(env)) {
            cs->exception_index = EXCP_RESET;
            lc3_cpu_do_interrupt(cs);

            cs->interrupt_request &= ~CPU_INTERRUPT_RESET;
            return true;
        }
    }
    if (interrupt_request & CPU_INTERRUPT_HARD) {
        if (cpu_interrupts_enabled(env) && env->intsrc != 0) {
            int index = ctz64(env->intsrc);
            cs->exception_index = EXCP_INT(index);
            lc3_cpu_do_interrupt(cs);

            env->intsrc &= env->intsrc - 1; /* clear the interrupt */
            if (!env->intsrc) {
                cs->interrupt_request &= ~CPU_INTERRUPT_HARD;
            }
            return true;
        }
    }
    return false;
}

void lc3_cpu_do_interrupt(CPUState *cs)
{
    // LC3CPU *cpu = LC3_CPU(cs);
    // CPULC3State *env = &cpu->env;

    // uint32_t ret = env->pc_w;
    // int vector = 0;
    // int size = lc3_feature(env, LC3_FEATURE_JMP_CALL) ? 2 : 1;
    // int base = 0;

    // if (cs->exception_index == EXCP_RESET) {
    //     vector = 0;
    // } else if (env->intsrc != 0) {
    //     vector = ctz64(env->intsrc) + 1;
    // }

    // if (lc3_feature(env, LC3_FEATURE_3_BYTE_PC)) {
    //     cpu_stb_data(env, env->sp--, (ret & 0x0000ff));
    //     cpu_stb_data(env, env->sp--, (ret & 0x00ff00) >> 8);
    //     cpu_stb_data(env, env->sp--, (ret & 0xff0000) >> 16);
    // } else if (lc3_feature(env, LC3_FEATURE_2_BYTE_PC)) {
    //     cpu_stb_data(env, env->sp--, (ret & 0x0000ff));
    //     cpu_stb_data(env, env->sp--, (ret & 0x00ff00) >> 8);
    // } else {
    //     cpu_stb_data(env, env->sp--, (ret & 0x0000ff));
    // }

    // env->pc_w = base + vector * size;
    // env->sregI = 0; /* clear Global Interrupt Flag */

    // cs->exception_index = -1;
}

hwaddr lc3_cpu_get_phys_page_debug(CPUState *cs, vaddr addr)
{
    return addr; /* I assume 1:1 address correspondence */
}

bool lc3_cpu_tlb_fill(CPUState *cs, vaddr address, int size,
                      MMUAccessType access_type, int mmu_idx,
                      bool probe, uintptr_t retaddr)
{
    int prot, page_size = TARGET_PAGE_SIZE;
    uint32_t paddr;

    address &= TARGET_PAGE_MASK;

    if (mmu_idx == MMU_CODE_IDX) {
        /* Access to code in flash. */
        paddr = OFFSET_CODE + address;
        prot = PAGE_READ | PAGE_EXEC;
        if (paddr >= OFFSET_DATA) {
            /*
             * This should not be possible via any architectural operations.
             * There is certainly not an exception that we can deliver.
             * Accept probing that might come from generic code.
             */
            if (probe) {
                return false;
            }
            error_report("execution left flash memory");
            abort();
        }
    } else {
        /* Access to memory. */
        paddr = OFFSET_DATA + address;
        prot = PAGE_READ | PAGE_WRITE;
        if (address < NUMBER_OF_CPU_REGISTERS + NUMBER_OF_IO_REGISTERS) {
            /*
             * Access to CPU registers, exit and rebuilt this TB to use
             * full access in case it touches specially handled registers
             * like SREG or SP.  For probing, set page_size = 1, in order
             * to force tlb_fill to be called for the next access.
             */
            if (probe) {
                page_size = 1;
            } else {
                LC3CPU *cpu = LC3_CPU(cs);
                CPULC3State *env = &cpu->env;
                env->fullacc = 1;
                cpu_loop_exit_restore(cs, retaddr);
            }
        }
    }

    tlb_set_page(cs, address, paddr, prot, mmu_idx, page_size);
    return true;
}

/*
 *  helpers
 */

void helper_sleep(CPULC3State *env)
{
    CPUState *cs = env_cpu(env);

    cs->exception_index = EXCP_HLT;
    cpu_loop_exit(cs);
}

void helper_unsupported(CPULC3State *env)
{
    CPUState *cs = env_cpu(env);

    /*
     *  I count not find what happens on the real platform, so
     *  it's EXCP_DEBUG for meanwhile
     */
    cs->exception_index = EXCP_DEBUG;
    if (qemu_loglevel_mask(LOG_UNIMP)) {
        qemu_log("UNSUPPORTED\n");
        cpu_dump_state(cs, stderr, 0);
    }
    cpu_loop_exit(cs);
}

void helper_debug(CPULC3State *env)
{
    CPUState *cs = env_cpu(env);

    cs->exception_index = EXCP_DEBUG;
    cpu_loop_exit(cs);
}

void helper_break(CPULC3State *env)
{
    CPUState *cs = env_cpu(env);

    cs->exception_index = EXCP_DEBUG;
    cpu_loop_exit(cs);
}

void helper_print_reg(CPULC3State *env)
{
    printf("\n---------------------\n");
    for (int i = 0; i < 10; i++) {
        printf("r%d: %d\n", i, env->r[i]);
    }
    // printf("pc: %d\n", env->pc_w);
    printf("---------------------\n");
}

void helper_print(uint32_t a)
{
    printf("word: %d\n", a);

    // uint16_t c = 0;
    // address_space_read(&address_space_memory, a, MEMTXATTRS_UNSPECIFIED, &c, 2);
    // printf("addr1:%d\n", c);
    // // uint16_t c = 0;
    // address_space_read(&address_space_memory, a*2, MEMTXATTRS_UNSPECIFIED, &c, 2);
    // printf("addr2:%d\n", c);
}

void helper_st(uint32_t SR, uint32_t addr)
{
    uint16_t num = (uint16_t)SR;
    uint16_t *num_p = &num;
    address_space_write(&address_space_memory, addr, MEMTXATTRS_UNSPECIFIED, num_p, 2);
}

/*
 * This function implements IN instruction
 *
 * It does the following
 * a.  if an IO register belongs to CPU, its value is read and returned
 * b.  otherwise io address is translated to mem address and physical memory
 *     is read.
 * c.  it caches the value for sake of SBI, SBIC, SBIS & CBI implementation
 *
 */
target_ulong helper_inb(CPULC3State *env, uint32_t port)
{
    target_ulong data = 0;

    // switch (port) {
    // case 0x38: /* RAMPD */
    //     data = 0xff & (env->rampD >> 16);
    //     break;
    // case 0x39: /* RAMPX */
    //     data = 0xff & (env->rampX >> 16);
    //     break;
    // case 0x3a: /* RAMPY */
    //     data = 0xff & (env->rampY >> 16);
    //     break;
    // case 0x3b: /* RAMPZ */
    //     data = 0xff & (env->rampZ >> 16);
    //     break;
    // case 0x3c: /* EIND */
    //     data = 0xff & (env->eind >> 16);
    //     break;
    // case 0x3d: /* SPL */
    //     data = env->sp & 0x00ff;
    //     break;
    // case 0x3e: /* SPH */
    //     data = env->sp >> 8;
    //     break;
    // case 0x3f: /* SREG */
    //     data = cpu_get_sreg(env);
    //     break;
    // default:
    //     /* not a special register, pass to normal memory access */
    //     data = address_space_ldub(&address_space_memory,
    //                               OFFSET_IO_REGISTERS + port,
    //                               MEMTXATTRS_UNSPECIFIED, NULL);
    // }

    return data;
}

/*
 *  This function implements OUT instruction
 *
 *  It does the following
 *  a.  if an IO register belongs to CPU, its value is written into the register
 *  b.  otherwise io address is translated to mem address and physical memory
 *      is written.
 *  c.  it caches the value for sake of SBI, SBIC, SBIS & CBI implementation
 *
 */
void helper_outb(CPULC3State *env, uint32_t port, uint32_t data)
{
    data &= 0x000000ff;

    // switch (port) {
    // case 0x38: /* RAMPD */
    //     if (lc3_feature(env, LC3_FEATURE_RAMPD)) {
    //         env->rampD = (data & 0xff) << 16;
    //     }
    //     break;
    // case 0x39: /* RAMPX */
    //     if (lc3_feature(env, LC3_FEATURE_RAMPX)) {
    //         env->rampX = (data & 0xff) << 16;
    //     }
    //     break;
    // case 0x3a: /* RAMPY */
    //     if (lc3_feature(env, LC3_FEATURE_RAMPY)) {
    //         env->rampY = (data & 0xff) << 16;
    //     }
    //     break;
    // case 0x3b: /* RAMPZ */
    //     if (lc3_feature(env, LC3_FEATURE_RAMPZ)) {
    //         env->rampZ = (data & 0xff) << 16;
    //     }
    //     break;
    // case 0x3c: /* EIDN */
    //     env->eind = (data & 0xff) << 16;
    //     break;
    // case 0x3d: /* SPL */
    //     env->sp = (env->sp & 0xff00) | (data);
    //     break;
    // case 0x3e: /* SPH */
    //     if (lc3_feature(env, LC3_FEATURE_2_BYTE_SP)) {
    //         env->sp = (env->sp & 0x00ff) | (data << 8);
    //     }
    //     break;
    // case 0x3f: /* SREG */
    //     cpu_set_sreg(env, data);
    //     break;
    // default:
    //     /* not a special register, pass to normal memory access */
    //     address_space_stb(&address_space_memory, OFFSET_IO_REGISTERS + port,
    //                       data, MEMTXATTRS_UNSPECIFIED, NULL);
    // }
}

/*
 *  this function implements LD instruction when there is a possibility to read
 *  from a CPU register
 */
target_ulong helper_fullrd(CPULC3State *env, uint32_t addr)
{
    uint8_t data;

    env->fullacc = false;

    if (addr < NUMBER_OF_CPU_REGISTERS) {
        /* CPU registers */
        data = env->r[addr];
    } else if (addr < NUMBER_OF_CPU_REGISTERS + NUMBER_OF_IO_REGISTERS) {
        /* IO registers */
        data = helper_inb(env, addr - NUMBER_OF_CPU_REGISTERS);
    } else {
        /* memory */
        data = address_space_ldub(&address_space_memory, OFFSET_DATA + addr,
                                  MEMTXATTRS_UNSPECIFIED, NULL);
    }
    return data;
}

/*
 *  this function implements ST instruction when there is a possibility to write
 *  into a CPU register
 */
void helper_fullwr(CPULC3State *env, uint32_t data, uint32_t addr)
{
    env->fullacc = false;

    /* Following logic assumes this: */
    assert(OFFSET_CPU_REGISTERS == OFFSET_DATA);
    assert(OFFSET_IO_REGISTERS == OFFSET_CPU_REGISTERS +
                                  NUMBER_OF_CPU_REGISTERS);

    if (addr < NUMBER_OF_CPU_REGISTERS) {
        /* CPU registers */
        env->r[addr] = data;
    } else if (addr < NUMBER_OF_CPU_REGISTERS + NUMBER_OF_IO_REGISTERS) {
        /* IO registers */
        helper_outb(env, addr - NUMBER_OF_CPU_REGISTERS, data);
    } else {
        /* memory */
        address_space_stb(&address_space_memory, OFFSET_DATA + addr, data,
                          MEMTXATTRS_UNSPECIFIED, NULL);
    }
}

// target_ulong helper_sign_extend(CPULC3State *env, uint32_t x)
// {
//     int bit_count = 0;
//     uint32_t y = x;
//     while (y > 0) {
//         y = y >> 1;
//         bit_count += 1;
//     }
//     printf("%d %d\n", x, bit_count);
//     if ((x >> (bit_count - 1)) & 1) {
//         x |= (0xFFFF << bit_count);
//     }
//     return x & 0xFFFF;
// }
enum
{
    FL_POS = 1 << 0, /* P */
    FL_ZRO = 1 << 1, /* Z */
    FL_NEG = 1 << 2, /* N */
};

void helper_update_flags(CPULC3State *env, uint32_t r)
{
    if (r == 0)
    {
        env->r[9] = FL_ZRO;
    }
    else if (r >> 15) /* a 1 in the left-most bit indicates negative */
    {
        env->r[9] = FL_NEG;
    }
    else
    {
        env->r[9] = FL_POS;
    }
}

void helper_PUTS(CPULC3State *env, uint32_t addr)
{
    /* one char per word */
    uint16_t c = 0;
    address_space_read(&address_space_memory, addr*2, MEMTXATTRS_UNSPECIFIED, &c, 2);
    while (c)
    {
        putc((char)c, stdout);
        addr++;
        address_space_read(&address_space_memory, addr*2, MEMTXATTRS_UNSPECIFIED, &c, 2);
    }
    fflush(stdout);
}

void helper_PUTSP(CPULC3State *env, uint32_t addr)
{
    /* one char per word */
    uint16_t c = 0;
    address_space_read(&address_space_memory, addr*2, MEMTXATTRS_UNSPECIFIED, &c, 2);
    while (c)
    {
        // char char1 = (*c) & 0xFF;
        // putc(char1, stdout);
        // char char2 = (*c) >> 8;
        // if (char2) putc(char2, stdout);
        // ++c;
        char char1 = c & 0xFF;
        putc(char1, stdout);
        char char2 = c >> 8;
        if (char2) putc(char2, stdout);
        addr++;
        address_space_read(&address_space_memory, addr*2, MEMTXATTRS_UNSPECIFIED, &c, 2);
    }
    fflush(stdout);
}

void helper_OUT(CPULC3State *env, uint32_t addr)
{
    putc((char)addr, stdout);
    fflush(stdout);
}

void helper_GETC(CPULC3State *env)
{
    env->r[R_R0] = (uint16_t)getchar();
    helper_update_flags(env, env->r[R_R0]);
}

void helper_IN(CPULC3State *env)
{
    printf("Enter a character: ");
    char c = getchar();
    putc(c, stdout);
    fflush(stdout);
    env->r[R_R0] = (uint16_t)c;
    helper_update_flags(env, env->r[R_R0]);
}

void helper_HALT(CPULC3State *env)
{
    puts("HALT");
    fflush(stdout);
    exit(0);
}

void helper_key(CPULC3State *env, uint32_t addr)
{
    if (addr == MR_KBSR)
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
        if (select(1, &readfds, NULL, NULL, &timeout) != 0)
        {
            // memory[MR_KBSR] = (1 << 15);
            // memory[MR_KBDR] = getchar();
            uint16_t arr = 1 << 15;
            address_space_write(&address_space_memory, MR_KBSR*2, MEMTXATTRS_UNSPECIFIED, &arr, 2);
            arr = getchar();
            address_space_write(&address_space_memory, MR_KBDR*2, MEMTXATTRS_UNSPECIFIED, &arr, 2);
        }
        else
        {
            uint16_t arr[1] = {0};
            address_space_write(&address_space_memory, MR_KBSR*2, MEMTXATTRS_UNSPECIFIED, arr, 2);
        }
    }
}

target_ulong helper_convert(uint32_t num)
{
    return num;
}