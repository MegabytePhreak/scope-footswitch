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

#ifndef WS2812_H_
#define WS2812_H_

#include "hal.h"

#define WS2812_WHITE 0x00FFFFFF
#define WS2812_GREEN 0x0000FF00
#define WS2812_YELLOW 0x00FFFF00
#define WS2812_RED 0x00FF0000
#define WS2812_MAGENTA 0x00FF00FF
#define WS2812_BLUE 0x000000FF
#define WS2812_CYAN 0x0000FFFF

typedef struct {
    uint32_t                  pwm_frequency;
    PWMDriver *               pwm_driver;
    pwmchannel_t              pwm_channel;
    const stm32_dma_stream_t *dma_stream;
    uint8_t                   dma_channel;
    uint32_t *                buffer;
    uint32_t                  pwm_period;
    uint32_t                  pwm_zero_duty;
    uint32_t                  pwm_one_duty;
    semaphore_t               sem;
} WS2812Config;

typedef union {
    uint32_t raw;
    struct {
        uint8_t blue;
        uint8_t green;
        uint8_t red;
    } comp;
} WS2812Pixel;

/* 	NEOPIXEL-MINI Timing Specs (SK6812)
 *	Period 1.25 uS
 *  High time for zero 0.3 uS
 *  High time for one  0.6 uS
 *	Reset time between tranfers 80 uS
 */

#define WS2812_PERIOD_NS 1250
#define WS2812_ZERO_DUTY_NS 300
#define WS2812_ONE_DUTY_NS 600
#define WS2812_RESET_CYCLES (80000 / WS2812_PERIOD_NS)

#define WS2812_TIMING_CONFIG_PERIOD(pwm_frequency)                             \
    ((WS2812_PERIOD_NS * (pwm_frequency / 1000)) / 1000000)
#define WS2812_TIMING_CONFIG_ZERO_DUTY(pwm_frequency)                          \
    ((WS2812_ZERO_DUTY_NS * (pwm_frequency / 1000)) / 1000000)
#define WS2812_TIMING_CONFIG_ONE_DUTY(pwm_frequency)                           \
    ((WS2812_ONE_DUTY_NS * (pwm_frequency / 1000)) / 1000000)

#define WS2812_TIMING_CONFIG(pwm_frequency)                                    \
    WS2812_TIMING_CONFIG_PERIOD(pwm_frequency),                                \
        WS2812_TIMING_CONFIG_ZERO_DUTY(pwm_frequency),                         \
        WS2812_TIMING_CONFIG_ONE_DUTY(pwm_frequency)

#define WS2812_BUFFER_SIZE(pixel_cnt) ((pixel_cnt)*24 + WS2812_RESET_CYCLES)

extern void ws2812_init(WS2812Config *cfg);
extern void ws2812_send_wait(WS2812Config *cfg, uint32_t count,
                             WS2812Pixel pixels[]);
extern void ws2812_send(WS2812Config *cfg, uint32_t count,
                        WS2812Pixel pixels[]);
extern void ws2812_wait(WS2812Config *cfg);

#endif