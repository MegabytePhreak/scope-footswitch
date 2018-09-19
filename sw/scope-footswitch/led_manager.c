#include "led_manager.h"
#include "chprintf.h"

void runLedManager(LedManagerConfig *cfg) {
    while (1) {
        updateLedManager(cfg);

        if (cfg->num_ws2812 > 0) {
            ws2812_send(cfg->ws2812_config, cfg->num_ws2812,
                        cfg->ws2812_pixels);
        }
        chThdSleepMilliseconds(cfg->step_ms);
        if (cfg->num_ws2812 > 0) {
            ws2812_wait(cfg->ws2812_config);
        }
    }
}

void updateLedManager(LedManagerConfig *cfg) {
    int i;
    for (i = 0; i < (int)cfg->num_leds; i++) {
        LedManagerEntry *led = &cfg->leds[i];

        int32_t level = led->level;
        level += led->step;
        if (level < led->min) {
            level = led->min;
            if (led->mode & LED_MODE_CYCLE) {
                led->step = -led->step;
            }
        }
        if (level > led->max) {
            level = led->max;
            if (led->mode & LED_MODE_CYCLE) {
                led->step = -led->step;
            }
        }
        led->level = level;

        if (led->mode & LED_MODE_REMAP) {
            level = cfg->remap(level);
        }
        pwmEnableChannel(led->pwmp, led->channel,
                         PWM_PERCENTAGE_TO_WIDTH(led->pwmp, level));
    }
    for (i = 0; i < (int)cfg->num_ws2812; i++) {
        LedManagerWS2812 *led = &cfg->ws2812s[i];

        int32_t level = led->level;
        level += led->step;
        if (level < led->min) {
            level = led->min;
            if (led->mode & LED_MODE_CYCLE) {
                led->step = -led->step;
            }
        }
        if (level > led->max) {
            level = led->max;
            if (led->mode & LED_MODE_CYCLE) {
                led->step = -led->step;
            }
        }
        led->level = level;

        if (led->mode & LED_MODE_REMAP) {
            level = cfg->remap(level);
        }
        level = (255 * level) / 10000;

        cfg->ws2812_pixels[i].comp.red   = (level * led->color.comp.red) >> 8;
        cfg->ws2812_pixels[i].comp.green = (level * led->color.comp.green) >> 8;
        cfg->ws2812_pixels[i].comp.blue  = (level * led->color.comp.blue) >> 8;
    }
}

static const uint16_t log_lut[65] = {
    0,     28,    55,    83,    113,   148,   191,   241,   299,   366,   442,
    527,   624,   731,   850,   981,   1125,  1283,  1454,  1640,  1842,  2059,
    2293,  2544,  2812,  3099,  3405,  3730,  4075,  4441,  4828,  5237,  5668,
    6123,  6601,  7103,  7630,  8183,  8762,  9367,  10000, 10000, 10000, 10000,
    10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
    10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000};

uint16_t logarithmicLedRemap(uint16_t level) {
    int index = (level & 0x3FFF) >> 8;

    uint32_t lut1 = log_lut[index];
    uint32_t lut2 = log_lut[index + 1];

    uint32_t mix = level & 0xFF;

    return ((lut1 * (255 - mix)) + (lut2 * mix)) / 256;
}

void setLedTarget(LedManagerConfig *cfg, uint8_t is_ws2812, uint8_t index,
                  uint16_t target) {
    osalDbgCheck(cfg);
    if (is_ws2812 && index < cfg->num_ws2812) {
        LedManagerWS2812 *led = &cfg->ws2812s[index];
        led->mode &= ~LED_MODE_CYCLE;
        if (target < led->level) {
            led->min = target;
            if (led->step > 0) {
                led->step = -led->step;
            }
        } else {
            led->max = target;
            if (led->step < 0) {
                led->step = -led->step;
            }
        }
    } else if (!is_ws2812 && index < cfg->num_leds) {
        LedManagerEntry *led = &cfg->leds[index];
        led->mode &= ~LED_MODE_CYCLE;
        if (target < led->level) {
            led->min = target;
            if (led->step > 0) {
                led->step = -led->step;
            }
        } else {
            led->max = target;
            if (led->step < 0) {
                led->step = -led->step;
            }
        }
    }
}

void setLedFlashing(LedManagerConfig *cfg, uint8_t is_ws2812, uint8_t index) {
    osalDbgCheck(cfg);
    if (is_ws2812 && index < cfg->num_ws2812) {
        LedManagerWS2812 *led = &cfg->ws2812s[index];
        led->mode |= LED_MODE_CYCLE;
        led->min = 0;
        led->max = 10000;
    } else if (!is_ws2812 && index < cfg->num_leds) {
        LedManagerEntry *led = &cfg->leds[index];
        led->mode |= LED_MODE_CYCLE;
        led->min = 2000;
        led->max = 10000;
    }
}

void setLedColor(LedManagerConfig *cfg, uint8_t index, uint32_t color) {
    osalDbgCheck(cfg);
    if (index < cfg->num_ws2812) {
        cfg->ws2812s[index].color.raw = color;
    }
}
