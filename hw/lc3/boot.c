/*
 * AVR loader helpers
 *
 * Copyright (c) 2019-2020 Philippe Mathieu-Daudé
 *
 * This work is licensed under the terms of the GNU GPLv2 or later.
 * See the COPYING file in the top-level directory.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "qemu/datadir.h"
#include "hw/loader.h"
#include "elf.h"
#include "boot.h"
#include "qemu/error-report.h"

#define MEMORY_MAX (1 << 16)
uint16_t memory[MEMORY_MAX];

uint16_t swap16(uint16_t x)
{
    return (x << 8) | (x >> 8);
}

bool lc3_load_firmware(LC3CPU *cpu, MachineState *ms,
                       MemoryRegion *program_mr, const char *firmware)
{
    FILE* file = fopen(firmware, "rb");
    if (!file) { return 0; };
        /* the origin tells us where in memory to place the image */
    uint16_t origin;
    fread(&origin, sizeof(origin), 1, file);
    origin = swap16(origin);

    /* we know the maximum file size so we only need one fread */
    uint16_t max_read = MEMORY_MAX - origin;
    uint16_t* p = memory + origin;
    size_t read = fread(p, sizeof(uint16_t), max_read, file);

    /* swap to little endian */
    while (read-- > 0)
    {
        *p = swap16(*p);
        ++p;
    }
    fclose(file);

    address_space_write(&address_space_memory, 0, MEMTXATTRS_UNSPECIFIED, memory, sizeof(memory));
    uint16_t a = 1;
    address_space_read(&address_space_memory, 0x3000*2, MEMTXATTRS_UNSPECIFIED, &a, 2);
    printf("%hu\n", a);

    return true;
}
