/*
 * This file is part of the scope-footswitch project.
 *
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



#ifndef USBCFG_H
#define USBCFG_H

#include "usb_tmc.h"

extern const USBConfig    usbcfg;
extern const USBTMCConfig usbtmccfg;
extern USBTMCDriver       TMC1;

enum emu_device_t {
    EMU_DEVICE_TMCEMU,
    EMU_DEVICE_TEK_DPO3034,
    EMU_DEVICE_KEYSIGHT_DSO9404A
};

extern enum emu_device_t emu_device;

void usb_init(void);

#endif /* USBCFG_H */

/** @} */
