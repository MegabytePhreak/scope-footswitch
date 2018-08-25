/*
    ChibiOS - Copyright (C) 2006..2018 Giovanni Di Sirio

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

#include <stdio.h>
#include <string.h>

#include "ch.h"
#include "hal.h"

#include "chprintf.h"

#include "usbcfg.h"

/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/


static THD_FUNCTION(traceThread, arg) {

  (void)arg;
  chRegSetThreadName("trace");
  chprintf((BaseSequentialStream *)&SD1, "USB connected");
  msg_t msg;
  while (ibqGetFullBufferTimeout(&(TMC1.ibqueue), TIME_INFINITE) != Q_RESET) {
    uint8_t * buf = TMC1.ibqueue.ptr;
    size_t len = TMC1.ibqueue.top - TMC1.ibqueue.ptr;
    size_t offset = 0;


    while(len){
      if(offset % 16 == 0)
        chprintf((BaseSequentialStream *)&SD1, "%04X:", offset);

      chprintf((BaseSequentialStream *)&SD1, " %02X", buf[offset]);

      offset ++;
      len--;

      if(offset % 16 == 0 || len == 0)
      chprintf((BaseSequentialStream *)&SD1, "\r\n");
    }
    ibqReleaseEmptyBuffer(&(TMC1.ibqueue));

  }
  chprintf((BaseSequentialStream *)&SD1, "USB disconnected");
}


/*===========================================================================*/
/* Generic code.                                                             */
/*===========================================================================*/

/*
 * Red LED blinker thread, times are in milliseconds.
 */
static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg) {

  (void)arg;
  chRegSetThreadName("blinker");
  while (true) {
    systime_t time;

    time = usbtmccfg.usbp->state == USB_ACTIVE ? 250 : 500;
    palClearLine(LINE_LED_RED);
    chThdSleepMilliseconds(time);
    palSetLine(LINE_LED_RED);
    chThdSleepMilliseconds(time);
  }
}

/*
 * Application entry point.
 */
int main(void) {

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();
  usb_init();

  /*
   * Initializes a serial-over-USB CDC driver.
   */
  tmcObjectInit(&TMC1);
  tmcStart(&TMC1, &usbtmccfg);

  /*
   * Activates the serial driver 1 using the driver default configuration.
   * PA9 and PA10 are routed to USART1.
   */
  sdStart(&SD1, NULL);
  palSetPadMode(GPIOA, 9, PAL_MODE_ALTERNATE(1));       /* USART1 TX.       */
  palSetPadMode(GPIOA, 10, PAL_MODE_ALTERNATE(1));      /* USART1 RX.       */


  /*
   * Activates the USB driver and then the USB bus pull-up on D+.
   * Note, a delay is inserted in order to not have to disconnect the cable
   * after a reset.
   */
  usbDisconnectBus(usbtmccfg.usbp);
  chThdSleepMilliseconds(1500);
  usbStart(usbtmccfg.usbp, &usbcfg);
  usbConnectBus(usbtmccfg.usbp);



  /*
   * Creates the blinker thread.
   */
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);

  /*
   * Normal main() thread activity, spawning shells.
   */
  while (true) {
    if (TMC1.config->usbp->state == USB_ACTIVE) {
      thread_t *tracetp = chThdCreateFromHeap(NULL, 4096,
                                             "trace", NORMALPRIO + 1,
                                             traceThread, NULL);
      chThdWait(tracetp);               /* Waiting termination.             */
    }
    chThdSleepMilliseconds(1000);
    chprintf((BaseSequentialStream *)&SD1, ".");
  }
}
