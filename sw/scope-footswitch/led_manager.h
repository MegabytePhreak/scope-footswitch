#ifndef _LED_MANAGER_H_
#define _LED_MANAGER_H_

#include "hal.h"
#include "ws2812.h"

typedef uint16_t LedMode_t;

enum {
    LED_MODE_CYCLE = 1,
    LED_MODE_REMAP = 2,
    LED_MODE_INVERT = 4
};

typedef uint16_t (*LedManagerRemap)(uint16_t);

typedef struct {
    PWMDriver *     pwmp;
    pwmchannel_t    channel;

    LedMode_t       mode;
    uint16_t        level;
    uint16_t        min;
    uint16_t        max;
    int16_t         step;
} LedManagerEntry;

typedef struct {
    LedMode_t       mode;
    WS2812Pixel     color;
    uint16_t        level;
    uint16_t        min;
    uint16_t        max;
    int16_t         step;
} LedManagerWS2812;

typedef struct {
    LedManagerEntry *   leds;
    LedManagerRemap     remap;
    size_t              num_leds;
    systime_t           step_ms;
    WS2812Config*       ws2812_config;
    LedManagerWS2812 *  ws2812s;
    size_t              num_ws2812;
    WS2812Pixel *       ws2812_pixels;
} LedManagerConfig;

void __attribute__((noreturn)) runLedManager(LedManagerConfig* cfg);
void updateLedManager(LedManagerConfig* cfg);
void setLedTarget(LedManagerConfig* cfg, uint8_t is_ws2812, uint8_t index, uint16_t target);
void setLedFlashing(LedManagerConfig* cfg, uint8_t is_ws2812, uint8_t index);
void setLedColor(LedManagerConfig* cfg, uint8_t index, uint32_t color);

uint16_t logarithmicLedRemap(uint16_t level);

#endif /* _LED_MANAGER_H_ */


