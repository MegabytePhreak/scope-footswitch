/*
 * This file is part of the scope-footswitch project.
 *
 * Copyright (C) 2019 Paul Roukema <paul@paulroukema.com>
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

#ifndef IMAGE_HEADER_H
#define IMAGE_HEADER_H

#include <stdint.h>

struct image_header {
    uint8_t  magic[4];
    uint8_t  version_major;
    uint8_t  version_minor;
    uint8_t  version_patch;
    uint16_t vid;
    uint16_t pid;
    uint8_t  bootloader_magic[4];
};

#define IMAGE_HEADER_MAGIC_STR "IHDR"
#define IMAGE_HEADER_MAGIC_INIT                                                \
    { 'I', 'H', 'D', 'R' }

#endif
