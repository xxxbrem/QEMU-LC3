/*
 * QEMU Arduino boards
 *
 * Copyright (c) 2019-2020 Philippe Mathieu-Daudé
 *
 * This work is licensed under the terms of the GNU GPLv2 or later.
 * See the COPYING file in the top-level directory.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* TODO: Implement the use of EXTRAM */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "lc3mcu.h"
#include "boot.h"
#include "qom/object.h"

struct MYLC3MachineState {
    /*< private >*/
    MachineState parent_obj;
    /*< public >*/
    LC3MCUState mcu;
};
typedef struct MYLC3MachineState MYLC3MachineState;

struct MYLC3MachineClass {
    /*< private >*/
    MachineClass parent_class;
    /*< public >*/
    const char *mcu_type;
    uint64_t xtal_hz;
};
typedef struct MYLC3MachineClass MYLC3MachineClass;

#define TYPE_MYLC3_MACHINE \
        MACHINE_TYPE_NAME("mylc3")
DECLARE_OBJ_CHECKERS(MYLC3MachineState, MYLC3MachineClass,
                     MYLC3_MACHINE, TYPE_MYLC3_MACHINE)

static void mylc3_init(MachineState *machine)
{
    MYLC3MachineClass *amc = MYLC3_MACHINE_GET_CLASS(machine);
    MYLC3MachineState *ams = MYLC3_MACHINE(machine);

    object_initialize_child(OBJECT(machine), "mcu", &ams->mcu, amc->mcu_type);
    object_property_set_uint(OBJECT(&ams->mcu), "xtal-frequency-hz",
                             amc->xtal_hz, &error_abort);
    sysbus_realize(SYS_BUS_DEVICE(&ams->mcu), &error_abort);

    if (machine->firmware) {
        if (!lc3_load_firmware(&ams->mcu.cpu, machine,
                               &ams->mcu.memory, machine->firmware)) {
            exit(1);
        }
    }
}

static void mylc3_machine_class_init(ObjectClass *oc, void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);

    mc->init = mylc3_init;
    mc->default_cpus = 1;
    mc->min_cpus = mc->default_cpus;
    mc->max_cpus = mc->default_cpus;
    mc->no_floppy = 1;
    mc->no_cdrom = 1;
    mc->no_parallel = 1;
}

static void mylc3_mega2560_class_init(ObjectClass *oc, void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);
    MYLC3MachineClass *amc = MYLC3_MACHINE_CLASS(oc);

    /*
     * https://store.mylc3.cc/mylc3-mega-2560-rev3
     * https://www.mylc3.cc/en/uploads/Main/mylc3-mega2560_R3-sch.pdf
     */
    mc->desc        = "Arduino Mega 2560 (ATmega2560)";
    mc->alias       = "mega2560";
    amc->mcu_type   = TYPE_LC32560_MCU;
    amc->xtal_hz    = 16 * 1000 * 1000; /* CSTCE16M0V53-R0 */
};

static const TypeInfo mylc3_machine_types[] = {
    {
        .name          = MACHINE_TYPE_NAME("mylc3-v1"),
        .parent        = TYPE_MYLC3_MACHINE,
        .class_init    = mylc3_mega2560_class_init,
    }, {
        .name           = TYPE_MYLC3_MACHINE,
        .parent         = TYPE_MACHINE,
        .instance_size  = sizeof(MYLC3MachineState),
        .class_size     = sizeof(MYLC3MachineClass),
        .class_init     = mylc3_machine_class_init,
        .abstract       = true,
    }
};

DEFINE_TYPES(mylc3_machine_types)
