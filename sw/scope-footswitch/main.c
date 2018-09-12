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
#include  "usbh_usbtmc.h"


static const char runcmd[] = "ACQuire:STATE RUN\r\n";
static const char stopcmd[] = "ACQuire:STATE STOP\r\n";
static const char setsinglecmd[] = "ACQuire:STOPAfter SEQUENCE\r\n";
static const char setrunstopcmd[] = "ACQuire:STOPAfter RUNSTOP\r\n";
static const char idncmd[] = "*IDN?\r\n";
static const char statecmd[] = "ACQuire:STATE?\r\n";
static const char stopaftercmd[] = "ACQuire:STOPAfter?\r\n";
static const char allstatecmd[] = "ACQuire?\r\n";

#if 0
static THD_WORKING_AREA(waTestTMC, 1024);

static void ThreadTestTMC(void *p) {
    (void)p;

    chRegSetThreadName("TMC");

    int j  = 0;
    for (;;) {
        for (uint8_t i = 0; i < USBH_TMC_MAX_INSTANCES; i++) {
            if (USBHTMCD[i].state == USBHTMC_STATE_ACTIVE) {
                usbDbgPrintf("TMC: Connected, TMC%d", i);
                usbhtmcStart(&USBHTMCD[i]);
                j = 0;
            } else if (USBHTMCD[i].state == USBHTMC_STATE_READY) {
                char buf[50+1];

                usbDbgPrintf("TMC test loop iteration %d", j++);
                if(usbhtmcIndicatorPulse(&USBHTMCD[i], NULL) != USBH_URBSTATUS_OK)
                {
                    usbDbgPrintf("TMC CTRL Error");
                }

                chThdSleepMilliseconds(2000);

                if(!usbhtmcWrite(&USBHTMCD[i], runcmd, strlen(runcmd), TIME_MS2I(1000) )){
                    usbDbgPrintf("TMC Write returned 0");
                }

                chThdSleepMilliseconds(2000);


                if(!usbhtmcWrite(&USBHTMCD[i], stopcmd, strlen(stopcmd), TIME_MS2I(1000) )){
                    usbDbgPrintf("TMC Write returned 0");
                }

                chThdSleepMilliseconds(2000);

                if(!usbhtmcWrite(&USBHTMCD[i], idncmd, strlen(idncmd), TIME_MS2I(1000) )){
                    usbDbgPrintf("TMC Write returned 0");
                }

                chThdSleepMilliseconds(2000);

                if(!usbhtmcRead(&USBHTMCD[i], buf, 50, TIME_MS2I(1000))){
                    usbDbgPrintf("TMC Read returned 0");
                } else {
                   usbDbgPrintf("TMC Read response: '%s'", buf);
                }

                chThdSleepMilliseconds(2000);

                if(!usbhtmcAsk(&USBHTMCD[i], statecmd, strlen(statecmd), buf, 50, TIME_MS2I(1000))){
                    usbDbgPrintf("TMC ASK returned 0");
                } else {
                    usbDbgPrintf("TMC Ask response: '%s'", buf);
                }

                chThdSleepMilliseconds(2000);
            }
        }
        chThdSleepMilliseconds(200);
    }

}
#endif

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
    {LED_MODE_REMAP, {WS2812_RED} , 3000, 500, 3000, 50},
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




enum {
    SCOPE_STATE_STOPPED,
    SCOPE_STATE_RUNNING,
    SCOPE_STATE_SINGLE
};

static void update_leds(uint8_t state)
{
    if(USBHTMCD[0].state == USBHTMC_STATE_READY)
    {
        setLedColor(&led_config, 0, WS2812_BLUE);
        setLedTarget(&led_config, TRUE, 0, 5000);
    } else {
        setLedColor(&led_config, 0, WS2812_RED);
        setLedTarget(&led_config, TRUE, 0, 5000);
    }

    switch (state)
    {
        case SCOPE_STATE_RUNNING:
            setLedTarget(&led_config, FALSE, LED_MODE_GREEN, 10000);
            setLedTarget(&led_config, FALSE, LED_MODE_RED,   0);
            break;
        case SCOPE_STATE_SINGLE:
            setLedTarget(&led_config, FALSE, LED_MODE_GREEN, 10000);
            setLedTarget(&led_config, FALSE, LED_MODE_RED,   7000);
            break;
        case SCOPE_STATE_STOPPED:
            setLedTarget(&led_config, FALSE, LED_MODE_GREEN, 0);
            setLedTarget(&led_config, FALSE, LED_MODE_RED,   10000);
            break;
        default:
            setLedColor(&led_config, 0, WS2812_RED);
            setLedFlashing(&led_config, TRUE, 0);
            setLedTarget(&led_config, FALSE, LED_MODE_RED,   0);
            setLedTarget(&led_config, FALSE, LED_MODE_RED,   0);
            break;
    }
}


static uint8_t scope_state;


bool tektronixParseState(const char * buf)
{
    uint8_t newstate = SCOPE_STATE_STOPPED;

    int field = 0;

    if(*buf == 'R')
    {
        newstate = SCOPE_STATE_RUNNING;
    } else if(*buf == 'S')
    {
        newstate = SCOPE_STATE_SINGLE;
    }

    while(*buf)
    {
        if(*buf == ',' || *buf == ';')
        {
            field++;
        } else if(field == 1)
        {
            if(*buf == '0')
            {
                newstate = SCOPE_STATE_STOPPED;
            }
            break;
        }
        buf++;
    }

    if(scope_state != newstate)
    {
        scope_state = newstate;
        return TRUE;
    }
    return FALSE;
}

static THD_WORKING_AREA(waThreadMain, 256);
static THD_FUNCTION(ThreadMain, arg) {

    (void)arg;
    char buf[50+1];

    events_init();
    scope_state = SCOPE_STATE_STOPPED;
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
                if(!usbhtmcAsk(&USBHTMCD[0], idncmd, strlen(idncmd), buf, sizeof(buf)-1, TIME_MS2I(1000))){
                    usbDbgPrintf("TMC ASK IDN returned 0");
                } else {
                    usbDbgPrintf("TMC ASK IDN response: '%s'", buf);

                }

                if(evt == MSG_TIMEOUT)
                {
                    evt = EVT_NOP;
                }
            } else if(USBHTMCD[0].state == USBHTMC_STATE_READY) {
                if(!usbhtmcAsk(&USBHTMCD[0], allstatecmd, strlen(allstatecmd), buf, sizeof(buf)-1, TIME_MS2I(1000))){
                    usbDbgPrintf("TMC ASK returned 0");
                } else {
                    usbDbgPrintf("TMC ASK ACQUIRE? response: '%s'", buf);
                    if(tektronixParseState(buf))
                    {
                        if(evt == MSG_TIMEOUT)
                        {
                            evt = EVT_NOP;
                        }
                    }
                }
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
                if(USBHTMCD[0].state == USBHTMC_STATE_READY)
                {
                    if(scope_state == SCOPE_STATE_STOPPED)
                    {
                        if(palReadLine(LINE_MODE))
                        {
                            if(!usbhtmcWrite(&USBHTMCD[0], setrunstopcmd, strlen(setrunstopcmd), TIME_MS2I(1000) ))
                            {
                                usbDbgPrintf("TMC Write returned 0");
                            }
                            if(!usbhtmcWrite(&USBHTMCD[0], runcmd, strlen(runcmd), TIME_MS2I(1000) ))
                            {
                                usbDbgPrintf("TMC Write returned 0");
                            }
                        }else
                        {
                            if(!usbhtmcWrite(&USBHTMCD[0], setsinglecmd, strlen(setsinglecmd), TIME_MS2I(1000) ))
                            {
                                usbDbgPrintf("TMC Write returned 0");
                            }
                            if(!usbhtmcWrite(&USBHTMCD[0], runcmd, strlen(runcmd), TIME_MS2I(1000) ))
                            {
                                usbDbgPrintf("TMC Write returned 0");
                            }
                        }
                    } else {
                        if(!usbhtmcWrite(&USBHTMCD[0], stopcmd, strlen(stopcmd), TIME_MS2I(1000) ))
                        {
                            usbDbgPrintf("TMC Write returned 0");
                        }
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
