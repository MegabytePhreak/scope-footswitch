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

#ifndef _SCOPE_H
#define _SCOPE_H

typedef struct USBHTmcDriver USBHTmcDriver;

// typedef struct USBHTmcDriver;

typedef enum {
    SCOPE_STATE_STOPPED,
    SCOPE_STATE_RUNNING,
    SCOPE_STATE_SINGLE
} scope_state_t;

typedef struct {
    int (*set_state)(USBHTmcDriver *tmcp, scope_state_t state);
    int (*get_state)(USBHTmcDriver *tmcp, scope_state_t *state);
} scope_config_t;

const scope_config_t *detect_scope(USBHTmcDriver *tmcp);

#endif /* _SCOPE_H */
