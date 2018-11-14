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

#include "ws2812.h"
#include "hal.h"


static void dma_isr(void *p, uint32_t flags) {
    WS2812Config *cfg = (WS2812Config *)p;
    chSysLockFromISR();
    if (flags & STM32_DMA_ISR_TEIF) {
        chSysHalt("DMA Error");
    } else if (flags & STM32_DMA_ISR_TCIF) {
        chSemSignalI(&cfg->sem);
    }
    chSysUnlockFromISR();
}

static void timer_init(WS2812Config *cfg) {
    PWMConfig pwm_cfg                       = {0};
    pwm_cfg.frequency                       = cfg->pwm_frequency;
    pwm_cfg.period                          = cfg->pwm_period;
    pwm_cfg.channels[cfg->pwm_channel].mode = PWM_OUTPUT_ACTIVE_HIGH;

    pwmStart(cfg->pwm_driver, &pwm_cfg);
}
static void dma_init(WS2812Config *cfg) {
    const stm32_dma_stream_t *stream = cfg->dma_stream;
    dmaStreamAllocate(stream, 10, dma_isr, cfg);
}

void ws2812_init(WS2812Config *cfg) {
    chSemObjectInit(&cfg->sem, 0);
    dma_init(cfg);
    timer_init(cfg);
}

static void start_dma(WS2812Config *cfg, uint32_t len) {
    dmaStreamSetMemory0(cfg->dma_stream, cfg->buffer);
    dmaStreamSetPeripheral(cfg->dma_stream,
                           &(cfg->pwm_driver->tim->CCR[cfg->pwm_channel]));
    dmaStreamSetMode(
        cfg->dma_stream,
        STM32_DMA_CR_DIR_M2P          // Transfer from memory to peripheral
            | STM32_DMA_CR_MINC       // Increment the memory address
            | STM32_DMA_CR_PSIZE_WORD // Peripheral transfer is 16-bits
            | STM32_DMA_CR_MSIZE_WORD // Memory transfer is bytes
            | STM32_DMA_CR_PL(2)      // Priority is very high
            | STM32_DMA_CR_TEIE       // Enable error interrupt
            | STM32_DMA_CR_TCIE       // Enable completion interrupt
            | STM32_DMA_CR_CHSEL(cfg->dma_channel));
    dmaStreamSetFIFO(cfg->dma_stream, 0);
    dmaStreamSetTransactionSize(cfg->dma_stream, len);
    pwmEnableChannel(cfg->pwm_driver, cfg->pwm_channel, 0);

    dmaStreamEnable(cfg->dma_stream);
    cfg->pwm_driver->tim->DIER |= STM32_TIM_DIER_CC1DE << (cfg->pwm_channel);
}

static void stop_dma(WS2812Config *cfg) {
    cfg->pwm_driver->tim->DIER &= ~(STM32_TIM_DIER_CC1DE << (cfg->pwm_channel));
    dmaStreamDisable(cfg->dma_stream);
    pwmDisableChannel(cfg->pwm_driver, cfg->pwm_channel);
}

static void wait_dma(WS2812Config *cfg) {
    chSemWait(&(cfg->sem));
}

static uint32_t *encode_byte(WS2812Config *cfg, uint32_t *buf, uint8_t byte) {
    int i;
    for (i = 0x80; i != 0; i >>= 1) {
        *buf++ = (byte & i) ? cfg->pwm_one_duty : cfg->pwm_zero_duty;
    }
    return buf;
}

static uint32_t encode_pixels(WS2812Config *cfg, uint32_t count,
                              WS2812Pixel pixels[]) {
    uint32_t  i;
    uint32_t *buf = cfg->buffer;
    for (i = 0; i < count; i++) {
        buf = encode_byte(cfg, buf, pixels[i].comp.green);
        buf = encode_byte(cfg, buf, pixels[i].comp.red);
        buf = encode_byte(cfg, buf, pixels[i].comp.blue);
    }

    for (i = 0; i < WS2812_RESET_CYCLES; i++) {
        *buf++ = 0;
    }
    return (buf - cfg->buffer);
}

void ws2812_send_wait(WS2812Config *cfg, uint32_t count, WS2812Pixel pixels[]) {
    ws2812_send(cfg, count, pixels);
    ws2812_wait(cfg);
}

void ws2812_send(WS2812Config *cfg, uint32_t count, WS2812Pixel pixels[]) {
    uint32_t len = encode_pixels(cfg, count, pixels);
    start_dma(cfg, len);
}

void ws2812_wait(WS2812Config *cfg) {
    wait_dma(cfg);
    stop_dma(cfg);
}
