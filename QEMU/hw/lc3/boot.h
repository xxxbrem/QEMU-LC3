/*
 * AVR loader helpers
 *
 * Copyright (c) 2019-2020 Philippe Mathieu-Daudé
 *
 * This work is licensed under the terms of the GNU GPLv2 or later.
 * See the COPYING file in the top-level directory.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HW_AVR_BOOT_H
#define HW_AVR_BOOT_H

#include "hw/boards.h"
#include "cpu.h"
#include "sysemu/reset.h"
#include "lc3mcu.h"
/**
 * avr_load_firmware:   load an image into a memory region
 *
 * @cpu:        Handle a AVR CPU object
 * @ms:         A MachineState
 * @mr:         Memory Region to load into
 * @firmware:   Path to the firmware file (raw binary or ELF format)
 *
 * Load a firmware supplied by the machine or by the user  with the
 * '-bios' command line option, and put it in target memory.
 *
 * Returns: true on success, false on error.
 */
bool lc3_load_firmware(LC3CPU *cpu, MachineState *ms,
                       MemoryRegion *mr, const char *firmware);
// void read_image_file(FILE* file);
uint16_t swap16(uint16_t x);
#endif
