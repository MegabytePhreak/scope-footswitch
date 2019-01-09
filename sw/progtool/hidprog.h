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
 
 
#ifndef HIDPROG_H
#define HIDPROG_H

#include <stdlib.h>
#include <stdint.h>

struct hid_device_;
typedef struct hid_device_ hid_device;

union hidprog_command;
union hidprog_response;
typedef union hidprog_command hidprog_command_t;
typedef union hidprog_response hidprog_response_t;

enum hidprog_block_flags {
    HIDPROG_BLOCK_FLAG_READONLY = 1,
};

typedef struct hidprog_info {
    uint8_t  version;
    uint32_t magic;
    uint32_t flash_base;
    uint32_t flash_size;
    struct {
        uint32_t size;
        uint32_t count;
        enum hidprog_block_flags flags;
    } blocks[8];
    uint8_t num_blocks;
} hidprog_info_t;


int hidprog_send_command(hid_device *handle, hidprog_command_t *cmd);
int hidprog_get_response(hid_device *handle, hidprog_response_t *rsp, int timeout );
int hidprog_run_command(hid_device *handle, hidprog_command_t *cmd, hidprog_response_t *rsp);

int hidprog_setaddr(hid_device *handle, uint32_t address);
int hidprog_getaddr(hid_device *handle, uint32_t* address);
int hidprog_getinfo(hid_device * handle, hidprog_info_t* info);
int hidprog_program(hid_device *handle, uint32_t base_address, uint8_t *data, size_t len);
int hidprog_read(hid_device *handle, uint32_t base_address, uint8_t *data, size_t len);
int hidprog_verify(hid_device *handle, uint32_t base_address, uint8_t *data, size_t len);
int hidprog_reset(hid_device *handle);
void hidprog_dump_response(hidprog_response_t *rsp);


#endif /* HIDPROG_H */