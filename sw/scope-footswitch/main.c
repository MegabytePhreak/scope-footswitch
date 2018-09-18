/*
    ChibiOS - Copyright (C) 2006..2015 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "ch.h"
#include "hal.h"
#include <string.h>
#include "usbh/debug.h"		/* for usbDbgPuts/usbDbgPrintf */
#include "chprintf.h"

#include "led_manager.h"
#include "ws2812.h"
#include "events.h"
#include "usbh_usbtmc.h"
#include "scope.h"


#define PWM_FREQ 2000
static PWMConfig tim3_pwmcfg = {
    STM32_SYSCLK,                           /* PWM clock frequency, PSC register. */
    (uint16_t) (STM32_SYSCLK / PWM_FREQ) + 1,   /* Initial PWM period. ARR register*/
    NULL,                                   /* Periodic callback pointer. */
    {                                       /* Channel configuration set dynamically below, */
        {PWM_OUTPUT_ACTIVE_HIGH | PWM_COMPLEMENTARY_OUTPUT_ACTIVE_LOW, NULL},
        {PWM_OUTPUT_ACTIVE_HIGH | PWM_COMPLEMENTARY_OUTPUT_ACTIVE_LOW, NULL},
        {PWM_OUTPUT_DISABLED, NULL},
        {PWM_OUTPUT_DISABLED, NULL}
    },
    0,                                       /* TIM CR2 register initialization data. */
#if STM32_PWM_USE_ADVANCED
    0,                                      /* TIM BDTR register initialization data. */
#endif
    0
};


static uint32_t ws2812_buffer[WS2812_BUFFER_SIZE(1)];

static WS2812Config ws2812_config = {
    STM32_SYSCLK,
    &PWMD5,
    1,
    STM32_DMA1_STREAM4,
    6,
    &ws2812_buffer[0],
    WS2812_TIMING_CONFIG(STM32_SYSCLK),
    {}
};


#define LED_MODE_GREEN 1
#define LED_MODE_RED 0

static LedManagerEntry g_leds[] = {
    {&PWMD3, 0, LED_MODE_REMAP, 0, 0, 5000, 1000},
    {&PWMD3, 1, LED_MODE_REMAP, 0, 0, 9000, 1000},
};

static LedManagerWS2812 g_rgb_leds[] = {
    {LED_MODE_REMAP, {WS2812_RED} , 3000, 500, 3000, 200},
};

static WS2812Pixel ws2812_pixel_buf[1];

static LedManagerConfig led_config = {
    g_leds,
    &logarithmicLedRemap,
    sizeof(g_leds)/sizeof(g_leds[0]),
    10,
    &ws2812_config,
    g_rgb_leds,
    sizeof(g_rgb_leds)/sizeof(g_rgb_leds[0]),
    ws2812_pixel_buf
};

static THD_WORKING_AREA(waThreadLed, 1024);
static THD_FUNCTION(ThreadLed, arg){
    (void)arg;


    pwmStart(&PWMD3, &tim3_pwmcfg);
    //pwmStart(&PWMD5, &tim5_pwmcfg);
    ws2812_init(&ws2812_config);
    pwmEnableChannel(&PWMD3, 0,
                PWM_PERCENTAGE_TO_WIDTH(&PWMD3, 5000));
    pwmEnableChannel(&PWMD3, 1,
                PWM_PERCENTAGE_TO_WIDTH(&PWMD3, 5000));

    runLedManager(&led_config);
}




static void update_leds(scope_state_t state)
{
    switch (state)
    {
        case SCOPE_STATE_RUNNING:
            setLedTarget(&led_config, FALSE, LED_MODE_GREEN, 10000);
            setLedTarget(&led_config, FALSE, LED_MODE_RED,   0);
            break;
        case SCOPE_STATE_SINGLE:
            setLedTarget(&led_config, FALSE, LED_MODE_GREEN, 10000);
            setLedTarget(&led_config, FALSE, LED_MODE_RED,   6000);
            break;
        case SCOPE_STATE_STOPPED:
            setLedTarget(&led_config, FALSE, LED_MODE_GREEN, 0);
            setLedTarget(&led_config, FALSE, LED_MODE_RED,   10000);
            break;
        default:
            setLedColor(&led_config, 0, WS2812_RED);
            setLedFlashing(&led_config, TRUE, 0);
            setLedTarget(&led_config, FALSE, LED_MODE_RED, 0);
            setLedTarget(&led_config, FALSE, LED_MODE_RED, 0);
            break;
    }
}




static THD_WORKING_AREA(waThreadMain, 1024);
static THD_FUNCTION(ThreadMain, arg) {

    (void)arg;

    events_init();
    scope_state_t scope_state = SCOPE_STATE_STOPPED;
    const scope_config_t* cfg = NULL;
    systime_t last_update_time = chVTGetSystemTimeX();

    while (true) {
       // chSemWait(&sem);
        msg_t evt = iqGetTimeout(&event_queue, TIME_MS2I(100));

        if ((chVTGetSystemTimeX() - last_update_time) > TIME_MS2I(200))
        {
            last_update_time = chVTGetSystemTimeX();
            if (USBHTMCD[0].state == USBHTMC_STATE_ACTIVE) {
                usbDbgPrintf("TMC: Connected, TMC%d", 0);
                usbhtmcStart(&USBHTMCD[0]);
                cfg = detect_scope(&USBHTMCD[0]);
                if(!cfg)
                {
                    setLedColor(&led_config, 0, WS2812_RED);
                    setLedFlashing(&led_config, TRUE, 0);
                } else {
                    setLedColor(&led_config, 0, WS2812_BLUE);
                    setLedTarget(&led_config, TRUE, 0, 5000);
                }
                if(evt == MSG_TIMEOUT)
                {
                    evt = EVT_NOP;
                }
            } else if(USBHTMCD[0].state == USBHTMC_STATE_READY) {
                if(cfg)
                {
                    scope_state_t newstate;
                    if(!cfg->get_state(&USBHTMCD[0], &newstate))
                    {
                        setLedColor(&led_config, 0, WS2812_RED);
                        setLedFlashing(&led_config, TRUE, 0);
                    } else {
                        if(scope_state != newstate)
                        {
                            scope_state = newstate;
                            if(evt == MSG_TIMEOUT)
                            {
                                evt = EVT_NOP;
                            }
                        }
                    }
                }
            } else {
                setLedColor(&led_config, 0, WS2812_RED);
                setLedTarget(&led_config, TRUE, 0, 5000);
            }
        }
        if( evt == MSG_TIMEOUT)
        {
            continue;
        }

        chprintf((BaseSequentialStream*)&SD2, "evt = %d\r\n", evt);
        switch(evt)
        {
            case EVT_FOOTSW1_PRESS:
            case EVT_FOOTSW2_PRESS:
            case EVT_BTN_CLICK:
                if(USBHTMCD[0].state == USBHTMC_STATE_READY && cfg)
                {
                    scope_state_t newstate = SCOPE_STATE_STOPPED;
                    if(scope_state == SCOPE_STATE_STOPPED)
                    {
                        if(palReadLine(LINE_MODE))
                        {
                            newstate = SCOPE_STATE_RUNNING;
                        }else
                        {
                            newstate = SCOPE_STATE_SINGLE;
                        }
                    }
                    if(!cfg->set_state(&USBHTMCD[0], newstate))
                    {
                        setLedFlashing(&led_config, TRUE, 0);
                    } else {
                        scope_state = newstate;
                    }
                }
                break;
            case EVT_BTN_HOLD:
                usbhtmcIndicatorPulse(&USBHTMCD[0], NULL);
                break;

            default:
                break;
        }
        update_leds(scope_state);
    }
}


int main(void) {

    //IWDG->KR = 0x5555;
    //IWDG->PR = 7;

    halInit();
    chSysInit();

    //PA2(TX) and PA3(RX) are routed to USART2
    sdStart(&SD2, NULL);

    chThdCreateStatic(waThreadLed, sizeof(waThreadLed), NORMALPRIO, ThreadLed, 0);
    chThdCreateStatic(waThreadMain, sizeof(waThreadMain), NORMALPRIO, ThreadMain, 0);

    bool en_device = palReadLine(LINE_EN_DEVICE);
    if(!en_device)
    {
        //chThdCreateStatic(waTestTMC, sizeof(waTestTMC), NORMALPRIO, ThreadTestTMC, 0);


        //turn on USB power
        palSetPad(GPIOA, GPIOA_HOST_VBUS_EN);
        chThdSleepMilliseconds(100);

        usbhStart(&USBHD1);
    }
    for(;;) {
        if(!en_device)
            usbhMainLoop(&USBHD1);

        chThdSleepMilliseconds(100);
    }
}
