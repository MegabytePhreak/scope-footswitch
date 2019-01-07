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

#include <stdint.h>

enum {
    HIDPROG_ID_PING = 0,
    HIDPROG_ID_SETADDR = 1,
    HIDPROG_ID_GETADDR = 2,
    HIDPROG_ID_WRITEDATA = 3,
    HIDPROG_ID_READDATA = 4,
    HIDPROG_ID_FINISH = 5,
    HIDPROG_ID_GETINFO = 6,
    HIDPROG_ID_UNKNOWN = 0xFF
};


#define HIDPROG_COMMAND_LEN 64 
#define HIDPROG_DATA_LEN (HIDPROG_COMMAND_LEN-4)

typedef union hid_command {
    uint8_t bytes[HIDPROG_COMMAND_LEN];

    struct {
        uint8_t id;
    };

    struct {
        uint8_t id;
        uint8_t echo;
    } ping;

    struct {
        uint8_t id;
        uint8_t addr[4];
    } setaddr;

    struct {
        uint8_t id;
    } getaddr;

    struct {
        uint8_t id;
        uint8_t len;
        uint8_t data[HIDPROG_DATA_LEN-4];
    } writedata;

    struct {
        uint8_t id;
        uint8_t len;
    } readdata;

    struct {
        uint8_t id;
        uint8_t flags;
    } finish;

    struct { 
        uint8_t id;
    } getinfo;
} hidprog_command_t;

typedef union hid_hid_response {
    uint8_t bytes[HIDPROG_COMMAND_LEN];

    struct {
        uint8_t id;
    };

    struct {
        uint8_t id;
        uint8_t echo;
        uint8_t count;
    } ping;

    struct {
        uint8_t id;
    } setaddr;

    struct {
        uint8_t id;
        uint8_t addr[4];
    } getaddr;

    struct {
        uint8_t id;
    } writedata;

    struct {
        uint8_t id;
    } finish;

    struct {
        uint8_t id;
        uint8_t len;
        uint8_t data[HIDPROG_DATA_LEN-4];
    } readdata;

    struct { 
        uint8_t id;
        uint8_t version;
        uint8_t magic [4];
        uint8_t flash_size [4];
        uint8_t block_count [8];
        uint8_t block_size  [8];
    } getinfo;

} hidprog_response_t;

#define HIDPROG_WRITEDATA_LEN_MAY_ERASE 0x80
#define HIDPROG_WRITEDATA_LEN_LEN 0x7F
#define HIDPROG_GETINFO_BLOCKSIZE_READONY 0x80
#define HIDPROG_GETINFO_BLOCKSIZE_SIZE 0x7F
#define HIDPROG_FINISH_FLAG_REBOOT 0x1

#endif /* HIDPROG_H */
