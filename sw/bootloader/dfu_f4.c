/*
 * This file is part of the scope-footswitch project.
 *
 * Copyright (C) 2013 Gareth McMullin <gareth@blacksphere.co.nz>
 * Copyright (C) 2018 Paul Roukema <paul@paulroukema.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
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
#include "usbdfu.h"

#if defined(STM32F2)
#include <libopencm3/stm32/f2/flash.h>
#elif defined(STM32F4)
#include <libopencm3/stm32/f4/flash.h>
#endif
#include <libopencm3/cm3/scb.h>

static uint32_t sector_addr[] = {
    0x8000000, 0x8004000, 0x8008000, 0x800c000, 0x8010000, 0x8020000, 0x8040000,
    0x8060000, 0x8080000, 0x80a0000, 0x80c0000, 0x80e0000, 0x8100000, 0};
static uint16_t sector_erase_time[12] = {500,  500,  500,  500,  1100, 2600,
                                         2600, 2600, 2600, 2600, 2600, 2600};
static uint8_t  sector_num            = 0xff;

/* Find the sector number for a given address*/
void dfu_get_sector_num(uint32_t addr) {
    int i = 0;
    while (sector_addr[i + 1]) {
        if (addr < sector_addr[i + 1])
            break;
        i++;
    }
    if (!sector_addr[i])
        return;
    sector_num = i;
}

void dfu_check_and_do_sector_erase(uint32_t addr) {
    if (addr == sector_addr[sector_num]) {
        flash_erase_sector(sector_num, FLASH_CR_PROGRAM_X32);
    }
}

void dfu_flash_program_buffer(uint32_t baseaddr, void *buf, int len) {
    for (int i = 0; i < len; i += 4)
        flash_program_word(baseaddr + i, *(uint32_t *)(buf + i));

    dfu_event();
}

uint32_t dfu_poll_timeout(uint8_t cmd, uint32_t addr, uint16_t blocknum) {
    /* Erase for big pages on STM2/4 needs "long" time
       Try not to hit USB timeouts*/
    if ((blocknum == 0) && (cmd == CMD_ERASE)) {
        dfu_get_sector_num(addr);
        if (addr == sector_addr[sector_num])
            return sector_erase_time[sector_num];
    }

    /* Programming 256 word with 100 us(max) per word*/
    return 26;
}

void dfu_jump_app_if_valid(void) {

    /* Boot the application if it's valid */
    /* Vector table may be anywhere in 128 kByte RAM
       CCM not handled*/
    if ((*(volatile uint32_t *)app_address & 0x2FFC0000) == 0x20000000) {
        /* Set vector table base address */
        SCB_VTOR = app_address & 0x1FFFFF; /* Max 2 MByte Flash*/
        /* Initialise master stack pointer */
        __asm volatile("msr msp, %0" ::"g"(*(volatile uint32_t *)app_address));
        /* Jump to application */
        void (*app_entry)(void) = *((void **)(app_address + 4));
        app_entry();
    }
}

void dfu_protect(void) {
    // Enable write protection for the bootloader sector
    FLASH_OPTCR &= ~(1 << 16);
}
