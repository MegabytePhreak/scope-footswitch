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


#ifndef EVENTS_H
#define EVENTS_H

#include "hal.h"

enum events {
    EVT_NOP = 0,
    EVT_STARTUP,
    EVT_BTN_CLICK,
    EVT_BTN_HOLD,
    EVT_FOOTSW1_PRESS,
    EVT_FOOTSW2_PRESS,
    EVT_MODE_CHANGE,
    EVT_VBUS_FAULT,
    EVT_STATE_CHANGE
};

extern input_queue_t event_queue;
void                 events_init(void);

#endif