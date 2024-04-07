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

#include "qemu/osdep.h"
#include "cpu.h"
#include "migration/cpu.h"

// static int get_sreg(QEMUFile *f, void *opaque, size_t size,
//                     const VMStateField *field)
// {
//     // CPULC3State *env = opaque;
//     // uint8_t sreg;

//     // sreg = qemu_get_byte(f);
//     // cpu_set_sreg(env, sreg);
//     return 0;
// }

// static int put_sreg(QEMUFile *f, void *opaque, size_t size,
//                     const VMStateField *field, JSONWriter *vmdesc)
// {
//     // CPULC3State *env = opaque;
//     // uint8_t sreg = cpu_get_sreg(env);

//     // qemu_put_byte(f, sreg);
//     return 0;
// }

// static const VMStateInfo vms_sreg = {
//     .name = "sreg",
//     .get = get_sreg,
//     .put = put_sreg,
// };

// static int get_segment(QEMUFile *f, void *opaque, size_t size,
//                        const VMStateField *field)
// {
//     uint32_t *ramp = opaque;
//     uint8_t temp;

//     temp = qemu_get_byte(f);
//     *ramp = ((uint32_t)temp) << 16;
//     return 0;
// }

// static int put_segment(QEMUFile *f, void *opaque, size_t size,
//                        const VMStateField *field, JSONWriter *vmdesc)
// {
//     uint32_t *ramp = opaque;
//     uint8_t temp = *ramp >> 16;

//     qemu_put_byte(f, temp);
//     return 0;
// }

// static const VMStateInfo vms_rampD = {
//     .name = "rampD",
//     .get = get_segment,
//     .put = put_segment,
// };
// static const VMStateInfo vms_rampX = {
//     .name = "rampX",
//     .get = get_segment,
//     .put = put_segment,
// };
// static const VMStateInfo vms_rampY = {
//     .name = "rampY",
//     .get = get_segment,
//     .put = put_segment,
// };
// static const VMStateInfo vms_rampZ = {
//     .name = "rampZ",
//     .get = get_segment,
//     .put = put_segment,
// };
// static const VMStateInfo vms_eind = {
//     .name = "eind",
//     .get = get_segment,
//     .put = put_segment,
// };

const VMStateDescription vms_avr_cpu = {
    .name = "cpu",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        // VMSTATE_UINT32(env.pc_w, LC3CPU),
        // VMSTATE_UINT32(env.sp, LC3CPU),
        // VMSTATE_UINT32(env.skip, LC3CPU),

        // VMSTATE_UINT32_ARRAY(env.r, LC3CPU, NUMBER_OF_CPU_REGISTERS),

        // VMSTATE_SINGLE(env, LC3CPU, 0, vms_sreg, CPULC3State),
        // VMSTATE_SINGLE(env.rampD, LC3CPU, 0, vms_rampD, uint32_t),
        // VMSTATE_SINGLE(env.rampX, LC3CPU, 0, vms_rampX, uint32_t),
        // VMSTATE_SINGLE(env.rampY, LC3CPU, 0, vms_rampY, uint32_t),
        // VMSTATE_SINGLE(env.rampZ, LC3CPU, 0, vms_rampZ, uint32_t),
        // VMSTATE_SINGLE(env.eind, LC3CPU, 0, vms_eind, uint32_t),

        // VMSTATE_END_OF_LIST()
    }
};
