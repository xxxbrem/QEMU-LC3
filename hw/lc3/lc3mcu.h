/*
 * QEMU ATmega MCU
 *
 * Copyright (c) 2019-2020 Philippe Mathieu-Daudé
 *
 * This work is licensed under the terms of the GNU GPLv2 or later.
 * See the COPYING file in the top-level directory.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HW_AVR_ATMEGA_H
#define HW_AVR_ATMEGA_H

#include "hw/char/avr_usart.h"
#include "hw/timer/avr_timer16.h"
#include "hw/misc/avr_power.h"
#include "target/lc3/cpu.h"
#include "qom/object.h"

#define TYPE_LC3_MCU     "ATmega"
#define TYPE_LC3168_MCU  "ATmega168"
#define TYPE_LC3328_MCU  "ATmega328"
#define TYPE_LC31280_MCU "ATmega1280"
#define TYPE_LC32560_MCU "ATmega2560"

typedef struct LC3MCUState LC3MCUState;
DECLARE_INSTANCE_CHECKER(LC3MCUState, LC3_MCU,
                         TYPE_LC3_MCU)

#define POWER_MAX 2
#define USART_MAX 4
#define TIMER_MAX 6
#define GPIO_MAX 12

struct LC3MCUState {
    /*< private >*/
    SysBusDevice parent_obj;
    /*< public >*/

    LC3CPU cpu;
    MemoryRegion memory;
    MemoryRegion eeprom;
    MemoryRegion cpu_ram;
    DeviceState *io;
    AVRMaskState pwr[POWER_MAX];
    AVRUsartState usart[USART_MAX];
    AVRTimer16State timer[TIMER_MAX];
    uint64_t xtal_freq_hz;
};

#endif /* HW_AVR_ATMEGA_H */
