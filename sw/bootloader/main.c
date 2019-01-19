/*
 * This file is part of the scope-footswitch project.
 *
 * Copyright (C) 2013 Gareth McMullin <gareth@blacksphere.co.nz>
 * Copyright (C) 2018 Paul Roukema <paul@paulroukema.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#define HID_MODE

#include <string.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/dwc/otg_fs.h>

#ifdef HID_MODE
#include "usbhid.h"
#else
#include "usbdfu.h"
#endif

uint32_t app_address = 0x08004200;

int activity_counter = 0;

#define LED_PORT GPIOA
#define LED_R GPIO6
#define LED_G GPIO7

#define BTN_PORT GPIOC
#define BTN GPIO13

#define EN_DEVICE_PORT GPIOB
#define EN_DEVICE GPIO5

void dfu_detach(void) {
    /* USB device must detach, we just reset... */
    scb_reset_system();
}

int main(void) {
    usbd_device *usbd_dev;

    rcc_periph_clock_enable(RCC_GPIOC);
    dfu_protect();

    // Force bootload if button pressed
    if (!gpio_get(BTN_PORT, BTN)) {
        /* Boot the application if it's valid. */
        dfu_jump_app_if_valid();
    }

    rcc_clock_setup_hse_3v3(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_48MHZ]);

    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);
    systick_set_reload(900000);

    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_OTGFS);

    // Drive EN_DEVICE high to switch to device mode
    gpio_mode_setup(EN_DEVICE_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                    EN_DEVICE);
    gpio_set(EN_DEVICE_PORT, EN_DEVICE);

    // Enable mode switch leds
    gpio_mode_setup(LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_R | LED_G);
    gpio_set(LED_PORT, LED_G);
    gpio_clear(LED_PORT, LED_R);

    systick_interrupt_enable();
    systick_counter_enable();

    // Enable USB AF
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO11 | GPIO12);
    gpio_set_af(GPIOA, GPIO_AF10, GPIO11 | GPIO12);

    // VBUS pin is not connected since the device is bus powered, so disable
    // sensing
    OTG_FS_GCCFG |= OTG_GCCFG_NOVBUSSENS;
#ifdef HID_MODE
    hid_init(&otgfs_usb_driver);

    hid_main();
#else
    dfu_init(&otgfs_usb_driver);

    dfu_main();
#endif
}

#ifdef HID_MODE
#define PRESCALE (64)
#else
#define PRESCALE (1)
#endif

static int prescaler = 0;
void dfu_event(void) {
    prescaler = (prescaler+1) % PRESCALE;
    if(prescaler == 0) {
        /* If the counter was at 0 before we should reset LED status. */
        if (activity_counter == 0) {
            gpio_clear(LED_PORT, LED_R | LED_G);
        }

        /* Prevent the sys_tick_handler from blinking leds for a bit. */
        activity_counter = 5;

        /* Toggle the DFU activity LED. */
        gpio_toggle(LED_PORT, LED_G);
    }
}

void sys_tick_handler(void) {
    #ifdef HID_MODE
        hid_tick();
    #endif

    /* Run the LED show only if there is no DFU activity. */
    if (activity_counter != 0) {
        activity_counter--;
        if (activity_counter == 0) {
            gpio_set(LED_PORT, LED_G);
        }
    } else {
        gpio_toggle(LED_PORT, LED_R | LED_G);
    }
}
