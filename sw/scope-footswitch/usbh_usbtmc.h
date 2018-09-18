/*
    ChibiOS - Copyright (C) 2006..2017 Giovanni Di Sirio
              Copyright (C) 2015..2017 Diego Ismirlian, (dismirlian (at) google's mail)

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

#ifndef USBH_CUSTOM_H_
#define USBH_CUSTOM_H_

#include "hal_usbh.h"

#if HAL_USE_USBH && HAL_USBH_USE_ADDITIONAL_CLASS_DRIVERS

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/
#define USBH_TMC_MAX_INSTANCES      1
#define USBH_TMC_BUF_SIZE     	(1<<8)

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

typedef enum {
    USBH_TMC_STATUS_SUCCESS = 0x01,
    USBH_TMC_STATUS_PENDING = 0x02,
    USBH_TMC_STATUS_FAILED = 0x80,
    USBH_TMC_STATUS_TRANSFER_NOT_IN_PROGRESS = 0x81,
    USBH_TMC_STATUS_SPLIT_NOT_IN_PROGRESS = 0x82,
    USBH_TMC_STATUS_SPLIT_IN_PROGRESS = 0x83,
} USBH_TMC_STATUS;


/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

typedef enum {
	USBHTMC_STATE_UNINIT = 0,
	USBHTMC_STATE_STOP = 1,
	USBHTMC_STATE_ACTIVE = 2,
	USBHTMC_STATE_READY = 3
} usbhtmc_state_t;

typedef struct USBHTmcDriver USBHTmcDriver;


struct USBHTmcDriver {
	/* inherited from abstract class driver */
	_usbh_base_classdriver_data

    usbh_ep_t epin;
    usbh_ep_t epout;
    usbh_ep_t epint;

    usbh_urb_t in_urb;
    usbh_urb_t out_urb;

    usbhtmc_state_t state;

    semaphore_t sem;

    uint8_t ifnum;
    uint8_t index;
    uint8_t last_btag;
};


typedef struct USBHTmcCapabilities USBHTmcCapabilities;

struct USBHTmcCapabilities {
    uint8_t bStatus;
    uint8_t bReserved0;
    uint16_t bcdUSBTMC;
    uint8_t bInterfaceCapabilities;
    uint8_t bDeviceCapabilities;
    uint32_t dwReserved1;
    uint32_t dwReserved2;
};


/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/


/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

extern USBHTmcDriver USBHTMCD[USBH_TMC_MAX_INSTANCES];

#ifdef __cplusplus
extern "C" {
#endif
	/* API goes here */
    void usbhtmcStart(USBHTmcDriver * tmcp);
    void usbhtmcStop(USBHTmcDriver * tmcp);
    size_t usbhtmcWrite(USBHTmcDriver * tmcp, const char *data, size_t n, systime_t timeout);
    size_t usbhtmcRead(USBHTmcDriver * tmcp, char *data, size_t n, systime_t timeout);
    size_t usbhtmcAsk(USBHTmcDriver * tmcp, const char *query, size_t querylen, char* answer, size_t answerlen, systime_t timeout);
    usbh_urbstatus_t usbhtmcClear(USBHTmcDriver * tmcp);
    usbh_urbstatus_t usbhtmcIndicatorPulse(USBHTmcDriver * tmcp, uint8_t* status);
    usbh_urbstatus_t usbhtmcGetCapabilities(USBHTmcDriver * tmcp, USBHTmcCapabilities* capp);

#ifdef __cplusplus
}
#endif

#endif

#endif /* USBH_CUSTOM_H_ */
