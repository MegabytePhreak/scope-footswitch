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

#include "led_manager.h"
#include "ws2812.h"
#include "events.h"
#include  "usbh_usbtmc.h"

#if HAL_USBH_USE_HID
#include "usbh/dev/hid.h"
#include "chprintf.h"



static THD_WORKING_AREA(waTestHID, 1024);

static void _hid_report_callback(USBHHIDDriver *hidp, uint16_t len) {
    uint8_t *report = (uint8_t *)hidp->config->report_buffer;

    if (hidp->type == USBHHID_DEVTYPE_BOOT_MOUSE) {
        usbDbgPrintf("Mouse report: buttons=%02x, Dx=%d, Dy=%d",
                report[0],
                (int8_t)report[1],
                (int8_t)report[2]);
    } else if (hidp->type == USBHHID_DEVTYPE_BOOT_KEYBOARD) {
        usbDbgPrintf("Keyboard report: modifier=%02x, keys=%02x %02x %02x %02x %02x %02x",
                report[0],
                report[2],
                report[3],
                report[4],
                report[5],
                report[6],
                report[7]);
    } else {
        usbDbgPrintf("Generic report, %d bytes", len);
    }
}

static USBH_DEFINE_BUFFER(uint8_t report[HAL_USBHHID_MAX_INSTANCES][8]);
static USBHHIDConfig hidcfg[HAL_USBHHID_MAX_INSTANCES];

static void ThreadTestHID(void *p) {
    (void)p;
    uint8_t i;
    static uint8_t kbd_led_states[HAL_USBHHID_MAX_INSTANCES];

    chRegSetThreadName("HID");

    for (i = 0; i < HAL_USBHHID_MAX_INSTANCES; i++) {
        hidcfg[i].cb_report = _hid_report_callback;
        hidcfg[i].protocol = USBHHID_PROTOCOL_BOOT;
        hidcfg[i].report_buffer = report[i];
        hidcfg[i].report_len = 8;
    }

    for (;;) {
        for (i = 0; i < HAL_USBHHID_MAX_INSTANCES; i++) {
            if (usbhhidGetState(&USBHHIDD[i]) == USBHHID_STATE_ACTIVE) {
                usbDbgPrintf("HID: Connected, HID%d", i);
                usbhhidStart(&USBHHIDD[i], &hidcfg[i]);
                if (usbhhidGetType(&USBHHIDD[i]) != USBHHID_DEVTYPE_GENERIC) {
                    usbhhidSetIdle(&USBHHIDD[i], 0, 0);
                }
                kbd_led_states[i] = 1;
            } else if (usbhhidGetState(&USBHHIDD[i]) == USBHHID_STATE_READY) {
                if (usbhhidGetType(&USBHHIDD[i]) == USBHHID_DEVTYPE_BOOT_KEYBOARD) {
                    USBH_DEFINE_BUFFER(uint8_t val);
                    val = kbd_led_states[i] << 1;
                    if (val == 0x08) {
                        val = 1;
                    }
                    usbhhidSetReport(&USBHHIDD[i], 0, USBHHID_REPORTTYPE_OUTPUT, &val, 1);
                    kbd_led_states[i] = val;
                }
            }
        }
        chThdSleepMilliseconds(200);
    }

}
#endif


static THD_WORKING_AREA(waTestTMC, 1024);
static const char runcmd[] = "ACQuire:STATE RUN\r\n";
static const char stopcmd[] = "ACQuire:STATE STOP\r\n";
static const char idncmd[] = "*IDN?\r\n";
static const char statecmd[] = "ACQuire:STATE?\r\n";

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




static LedManagerEntry g_leds[] = {
    {&PWMD3, 0, LED_MODE_REMAP, 2500, 0, 5000, 50},
    {&PWMD3, 1, LED_MODE_REMAP, 0, 0, 9000, 50},
};

static LedManagerWS2812 g_rgb_leds[] = {
    {LED_MODE_REMAP, WS2812_RED, 3000, 500, 3000, 50},
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



static THD_WORKING_AREA(waThreadMain, 256);
static THD_FUNCTION(ThreadMain, arg) {

    (void)arg;

    events_init();

    while (true) {
       // chSemWait(&sem);
        msg_t evt = iqGetTimeout(&event_queue, TIME_MS2I(10));

        if (evt == Q_TIMEOUT)
        {
            continue;
        }

        switch(evt)
        {
            default:
                chprintf((BaseSequentialStream*)&SD2, "evt = %d\r\n", evt);
                break;
        }
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
    #if HAL_USBH_USE_HID
        chThdCreateStatic(waTestHID, sizeof(waTestHID), NORMALPRIO, ThreadTestHID, 0);
    #endif
        chThdCreateStatic(waTestTMC, sizeof(waTestTMC), NORMALPRIO, ThreadTestTMC, 0);


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
