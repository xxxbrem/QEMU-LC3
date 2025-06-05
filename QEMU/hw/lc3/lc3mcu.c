/*
 * QEMU LC3 MCU
 *
 * Copyright (c) 2019-2020 Philippe Mathieu-Daudé
 *
 * This work is licensed under the terms of the GNU GPLv2 or later.
 * See the COPYING file in the top-level directory.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "qemu/module.h"
#include "qemu/units.h"
#include "qapi/error.h"
#include "exec/memory.h"
#include "exec/address-spaces.h"
#include "sysemu/sysemu.h"
#include "hw/qdev-properties.h"
#include "hw/sysbus.h"
#include "qom/object.h"
#include "hw/misc/unimp.h"
#include "lc3mcu.h"

enum LC3Peripheral {
    POWER0, POWER1,
    GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF,
    GPIOG, GPIOH, GPIOI, GPIOJ, GPIOK, GPIOL,
    USART0, USART1, USART2, USART3,
    TIMER0, TIMER1, TIMER2, TIMER3, TIMER4, TIMER5,
    PERIFMAX
};

#define GPIO(n)     (n + GPIOA)
#define USART(n)    (n + USART0)
#define TIMER(n)    (n + TIMER0)
#define POWER(n)    (n + POWER0)

typedef struct {
    uint16_t addr;
    enum LC3Peripheral power_index;
    uint8_t power_bit;
    /* timer specific */
    uint16_t intmask_addr;
    uint16_t intflag_addr;
    bool is_timer16;
} peripheral_cfg;

struct LC3McuClass {
    /*< private >*/
    SysBusDeviceClass parent_class;
    /*< public >*/
    const char *uc_name;
    const char *cpu_type;
    size_t memory_size;
    size_t eeprom_size;
    size_t cpu_ram_size;
    size_t io_size;
    size_t gpio_count;
    size_t adc_count;
    const uint8_t *irq;
    const peripheral_cfg *dev;
};
typedef struct LC3McuClass LC3McuClass;

DECLARE_CLASS_CHECKERS(LC3McuClass, LC3_MCU,
                       TYPE_LC3_MCU)

static const peripheral_cfg dev1280_2560[PERIFMAX] = {
    [USART3]        = { 0x130, POWER1, 2 },
    [TIMER5]        = { 0x120, POWER1, 5, 0x73, 0x3a, true },
    [GPIOL]         = { 0x109 },
    [GPIOK]         = { 0x106 },
    [GPIOJ]         = { 0x103 },
    [GPIOH]         = { 0x100 },
    [USART2]        = {  0xd0, POWER1, 1 },
    [USART1]        = {  0xc8, POWER1, 0 },
    [USART0]        = {  0xc0, POWER0, 1 },
    [TIMER2]        = {  0xb0, POWER0, 6, 0x70, 0x37, false }, /* TODO async */
    [TIMER4]        = {  0xa0, POWER1, 4, 0x72, 0x39, true },
    [TIMER3]        = {  0x90, POWER1, 3, 0x71, 0x38, true },
    [TIMER1]        = {  0x80, POWER0, 3, 0x6f, 0x36, true },
    [POWER1]        = {  0x65 },
    [POWER0]        = {  0x64 },
    [TIMER0]        = {  0x44, POWER0, 5, 0x6e, 0x35, false },
    [GPIOG]         = {  0x32 },
    [GPIOF]         = {  0x2f },
    [GPIOE]         = {  0x2c },
    [GPIOD]         = {  0x29 },
    [GPIOC]         = {  0x26 },
    [GPIOB]         = {  0x23 },
    [GPIOA]         = {  0x20 },
};

enum LC3Irq {
    USART0_RXC_IRQ, USART0_DRE_IRQ, USART0_TXC_IRQ,
    USART1_RXC_IRQ, USART1_DRE_IRQ, USART1_TXC_IRQ,
    USART2_RXC_IRQ, USART2_DRE_IRQ, USART2_TXC_IRQ,
    USART3_RXC_IRQ, USART3_DRE_IRQ, USART3_TXC_IRQ,
    TIMER0_CAPT_IRQ, TIMER0_COMPA_IRQ, TIMER0_COMPB_IRQ,
        TIMER0_COMPC_IRQ, TIMER0_OVF_IRQ,
    TIMER1_CAPT_IRQ, TIMER1_COMPA_IRQ, TIMER1_COMPB_IRQ,
        TIMER1_COMPC_IRQ, TIMER1_OVF_IRQ,
    TIMER2_CAPT_IRQ, TIMER2_COMPA_IRQ, TIMER2_COMPB_IRQ,
        TIMER2_COMPC_IRQ, TIMER2_OVF_IRQ,
    TIMER3_CAPT_IRQ, TIMER3_COMPA_IRQ, TIMER3_COMPB_IRQ,
        TIMER3_COMPC_IRQ, TIMER3_OVF_IRQ,
    TIMER4_CAPT_IRQ, TIMER4_COMPA_IRQ, TIMER4_COMPB_IRQ,
        TIMER4_COMPC_IRQ, TIMER4_OVF_IRQ,
    TIMER5_CAPT_IRQ, TIMER5_COMPA_IRQ, TIMER5_COMPB_IRQ,
        TIMER5_COMPC_IRQ, TIMER5_OVF_IRQ,
    IRQ_COUNT
};

#define USART_IRQ_COUNT     3
#define USART_RXC_IRQ(n)    (n * USART_IRQ_COUNT + USART0_RXC_IRQ)
#define USART_DRE_IRQ(n)    (n * USART_IRQ_COUNT + USART0_DRE_IRQ)
#define USART_TXC_IRQ(n)    (n * USART_IRQ_COUNT + USART0_TXC_IRQ)
#define TIMER_IRQ_COUNT     5
#define TIMER_CAPT_IRQ(n)   (n * TIMER_IRQ_COUNT + TIMER0_CAPT_IRQ)
#define TIMER_COMPA_IRQ(n)  (n * TIMER_IRQ_COUNT + TIMER0_COMPA_IRQ)
#define TIMER_COMPB_IRQ(n)  (n * TIMER_IRQ_COUNT + TIMER0_COMPB_IRQ)
#define TIMER_COMPC_IRQ(n)  (n * TIMER_IRQ_COUNT + TIMER0_COMPC_IRQ)
#define TIMER_OVF_IRQ(n)    (n * TIMER_IRQ_COUNT + TIMER0_OVF_IRQ)

static const uint8_t irq1280_2560[IRQ_COUNT] = {
    [TIMER2_COMPA_IRQ]      = 14,
    [TIMER2_COMPB_IRQ]      = 15,
    [TIMER2_OVF_IRQ]        = 16,
    [TIMER1_CAPT_IRQ]       = 17,
    [TIMER1_COMPA_IRQ]      = 18,
    [TIMER1_COMPB_IRQ]      = 19,
    [TIMER1_COMPC_IRQ]      = 20,
    [TIMER1_OVF_IRQ]        = 21,
    [TIMER0_COMPA_IRQ]      = 22,
    [TIMER0_COMPB_IRQ]      = 23,
    [TIMER0_OVF_IRQ]        = 24,
    [USART0_RXC_IRQ]        = 26,
    [USART0_DRE_IRQ]        = 27,
    [USART0_TXC_IRQ]        = 28,
    [TIMER3_CAPT_IRQ]       = 32,
    [TIMER3_COMPA_IRQ]      = 33,
    [TIMER3_COMPB_IRQ]      = 34,
    [TIMER3_COMPC_IRQ]      = 35,
    [TIMER3_OVF_IRQ]        = 36,
    [USART1_RXC_IRQ]        = 37,
    [USART1_DRE_IRQ]        = 38,
    [USART1_TXC_IRQ]        = 39,
    [TIMER4_CAPT_IRQ]       = 42,
    [TIMER4_COMPA_IRQ]      = 43,
    [TIMER4_COMPB_IRQ]      = 44,
    [TIMER4_COMPC_IRQ]      = 45,
    [TIMER4_OVF_IRQ]        = 46,
    [TIMER5_CAPT_IRQ]       = 47,
    [TIMER5_COMPA_IRQ]      = 48,
    [TIMER5_COMPB_IRQ]      = 49,
    [TIMER5_COMPC_IRQ]      = 50,
    [TIMER5_OVF_IRQ]        = 51,
    [USART2_RXC_IRQ]        = 52,
    [USART2_DRE_IRQ]        = 53,
    [USART2_TXC_IRQ]        = 54,
    [USART3_RXC_IRQ]        = 55,
    [USART3_DRE_IRQ]        = 56,
    [USART3_TXC_IRQ]        = 57,
};

// static void connect_peripheral_irq(const LC3McuClass *k,
//                                    SysBusDevice *dev, int dev_irqn,
//                                    DeviceState *cpu,
//                                    unsigned peripheral_index)
// {
//     int cpu_irq = k->irq[peripheral_index];

//     if (!cpu_irq) {
//         return;
//     }
//     /* FIXME move that to avr_cpu_set_int() once 'sample' board is removed */
//     assert(cpu_irq >= 2);
//     cpu_irq -= 2;

//     sysbus_connect_irq(dev, dev_irqn, qdev_get_gpio_in(cpu, cpu_irq));
// }

// static void connect_power_reduction_gpio(LC3MCUState *s,
//                                          const LC3McuClass *k,
//                                          DeviceState *cpu,
//                                          unsigned peripheral_index)
// {
//     unsigned power_index = k->dev[peripheral_index].power_index;
//     assert(k->dev[power_index].addr);
//     sysbus_connect_irq(SYS_BUS_DEVICE(&s->pwr[power_index - POWER0]),
//                        k->dev[peripheral_index].power_bit,
//                        qdev_get_gpio_in(cpu, 0));
// }

static void lc3_realize(DeviceState *dev, Error **errp)
{
    LC3MCUState *s = LC3_MCU(dev);
    const LC3McuClass *mc = LC3_MCU_GET_CLASS(dev);
    // DeviceState *cpudev;
    // SysBusDevice *sbd;
    // char *devname;
    // size_t i;

    assert(mc->io_size <= 0x200);

    if (!s->xtal_freq_hz) {
        error_setg(errp, "\"xtal-frequency-hz\" property must be provided.");
        return;
    }

    /* CPU */
    object_initialize_child(OBJECT(dev), "cpu", &s->cpu, mc->cpu_type);
    qdev_realize(DEVICE(&s->cpu), NULL, &error_abort);
    // cpudev = DEVICE(&s->cpu);

    /* SRAM */
    memory_region_init_ram(&s->cpu_ram, OBJECT(dev), "cpu_ram", mc->cpu_ram_size,
                           &error_abort);
    memory_region_add_subregion(get_system_memory(), 0x6000, &s->cpu_ram);

    // /* Flash */
    // memory_region_init_ram(&s->memory, OBJECT(dev),
    //                        "memory", mc->memory_size, &error_fatal);
    // memory_region_add_subregion(get_system_memory(), 131072, &s->memory);
}

static Property lc3_props[] = {
    DEFINE_PROP_UINT64("xtal-frequency-hz", LC3MCUState,
                       xtal_freq_hz, 0),
    DEFINE_PROP_END_OF_LIST()
};

static void lc3_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    dc->realize = lc3_realize;
    device_class_set_props(dc, lc3_props);
    /* Reason: Mapped at fixed location on the system bus */
    dc->user_creatable = false;
}

static void lc32560_class_init(ObjectClass *oc, void *data)
{
    LC3McuClass *amc = LC3_MCU_CLASS(oc);

    amc->cpu_type = LC3_CPU_TYPE_NAME("lc3");
    amc->memory_size = 256 * KiB;
    amc->eeprom_size = 4 * KiB;
    // amc->cpu_ram_size = 8 * KiB;
    amc->cpu_ram_size = 1 << 17;
    amc->io_size = 512;
    amc->gpio_count = 54;
    amc->adc_count = 16;
    amc->irq = irq1280_2560;
    amc->dev = dev1280_2560;
};

static const TypeInfo lc3_mcu_types[] = {
    {
        .name           = TYPE_LC32560_MCU,
        .parent         = TYPE_LC3_MCU,
        .class_init     = lc32560_class_init,
    }, {
        .name           = TYPE_LC3_MCU,
        .parent         = TYPE_SYS_BUS_DEVICE,
        .instance_size  = sizeof(LC3MCUState),
        .class_size     = sizeof(LC3McuClass),
        .class_init     = lc3_class_init,
        .abstract       = true,
    }
};

DEFINE_TYPES(lc3_mcu_types)
