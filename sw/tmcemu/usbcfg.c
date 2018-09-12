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

#include "hal.h"
#include "usb_tmc.h"
#include "usbcfg.h"
#include <memory.h>

USBTMCDriver TMC1;
enum emu_device_t emu_device = EMU_DEVICE_TEK_DPO3034;

/*
 * Endpoints to be used for USBD1.
 */
#define TMC_BULK_OUT_EP      1
#define TMC_BULK_IN_EP       1
#define TMC_INT_IN_EP        2

/*
 * USB Device Descriptor.
 */
static const uint8_t tmcemu_device_descriptor_data[18] = {
  USB_DESC_DEVICE       (0x0110,        /* bcdUSB (1.1).                    */
                         0x00,          /* bDeviceClass                     */
                         0x00,          /* bDeviceSubClass.                 */
                         0x00,          /* bDeviceProtocol.                 */
                         0x40,          /* bMaxPacketSize.                  */
                         0x0483,        /* idVendor (ST).                   */
                         0x5740,        /* idProduct.                       */
                         0x0200,        /* bcdDevice.                       */
                         1,             /* iManufacturer.                   */
                         2,             /* iProduct.                        */
                         3,             /* iSerialNumber.                   */
                         1)             /* bNumConfigurations.              */
};


static const uint8_t dpo3034_device_descriptor_data[18] = {
  USB_DESC_DEVICE       (0x0200,        /* bcdUSB (1.1).                    */
                         0x00,          /* bDeviceClass                     */
                         0x00,          /* bDeviceSubClass.                 */
                         0x00,          /* bDeviceProtocol.                 */
                         0x40,          /* bMaxPacketSize.                  */
                         0x0699,        /* idVendor (ST).                   */
                         0x0415,        /* idProduct.                       */
                         0x0200,        /* bcdDevice.                       */
                         1,             /* iManufacturer.                   */
                         2,             /* iProduct.                        */
                         3,             /* iSerialNumber.                   */
                         1)             /* bNumConfigurations.              */
};

static const uint8_t dso9404a_device_descriptor_data[18] = {
    USB_DESC_DEVICE       (0x0200,        /* bcdUSB (1.1).                    */
                         0x00,          /* bDeviceClass                     */
                         0x00,          /* bDeviceSubClass.                 */
                         0x00,          /* bDeviceProtocol.                 */
                         0x40,          /* bMaxPacketSize.                  */
                         0x2a8d,        /* idVendor (ST).                   */
                         0x9009,        /* idProduct.                       */
                         0x0200,        /* bcdDevice.                       */
                         1,             /* iManufacturer.                   */
                         2,             /* iProduct.                        */
                         3,             /* iSerialNumber.                   */
                         1)             /* bNumConfigurations.              */
};

/*
 * Device Descriptor wrapper.
 */
static const USBDescriptor tmcemu_device_descriptor = {
  sizeof tmcemu_device_descriptor_data,
  tmcemu_device_descriptor_data
};

static const USBDescriptor dpo3034_device_descriptor = {
  sizeof dpo3034_device_descriptor_data,
  dpo3034_device_descriptor_data
};


static const USBDescriptor dso9404a_device_descriptor = {
  sizeof dso9404a_device_descriptor_data,
  dso9404a_device_descriptor_data
};


/* Configuration Descriptor tree for a CDC.*/
static const uint8_t tmc_configuration_descriptor_data[39] = {
  /* Configuration Descriptor.*/
  USB_DESC_CONFIGURATION(39,            /* wTotalLength.                    */
                         0x01,          /* bNumInterfaces.                  */
                         0x01,          /* bConfigurationValue.             */
                         0,             /* iConfiguration.                  */
                         0xC0,          /* bmAttributes (self powered).     */
                         50),           /* bMaxPower (100mA).               */
  /* Interface Descriptor.*/
  USB_DESC_INTERFACE    (0x00,          /* bInterfaceNumber.                */
                         0x00,          /* bAlternateSetting.               */
                         0x03,          /* bNumEndpoints.                   */
                         0xFE,          /* bInterfaceClass (Application Specific) */
                         0x03,          /* bInterfaceSubClass (USBTMC).     */
                         0x00,          /* bInterfaceProtocol (Basic).      */
                         0),            /* iInterface.                      */
  /* Endpoint 1 Descriptor.*/
  USB_DESC_ENDPOINT     (1,       /* bEndpointAddress.*/
                         0x02,          /* bmAttributes (Bulk).             */
                         0x0040,        /* wMaxPacketSize.                  */
                         0x00),         /* bInterval.                       */
  /* Endpoint 2 Descriptor.*/
  USB_DESC_ENDPOINT     (1|0x80,    /* bEndpointAddress.*/
                         0x02,          /* bmAttributes (Bulk).             */
                         0x0040,        /* wMaxPacketSize.                  */
                         0x00),          /* bInterval.                       */
  /* Endpoint 3 Descriptor.*/
  USB_DESC_ENDPOINT     (2|0x80,
                         0x03,          /* bmAttributes (Interrupt).        */
                         0x0008,        /* wMaxPacketSize.                  */
                         0xFF)          /* bInterval.                       */

};

/*
 * Configuration Descriptor wrapper.
 */
static const USBDescriptor tmc_configuration_descriptor = {
  sizeof tmc_configuration_descriptor_data,
  tmc_configuration_descriptor_data
};

/*
 * U.S. English language identifier.
 */
static const uint8_t tmc_string0[] = {
  USB_DESC_BYTE(4),                     /* bLength.                         */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  USB_DESC_WORD(0x0409)                 /* wLANGID (U.S. English).          */
};

/*
 * Vendor string.
 */
static const uint8_t tmcemu_string1[] = {
  USB_DESC_BYTE(26),                    /* bLength.                         */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  'P', 0, 'a', 0, 'u', 0, 'l', 0, ' ', 0, 'R', 0, 'o', 0, 'u', 0,
  'k', 0, 'e', 0, 'm', 0, 'a', 0
};

/*
 * Device Description string.
 */
static const uint8_t tmcemu_string2[] = {
  USB_DESC_BYTE(38),                    /* bLength.                         */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  'U', 0, 'S', 0, 'B', 0, 'T', 0, 'M', 0, 'C', 0, ' ', 0, 'T', 0,
  'e', 0, 's', 0, 't', 0, ' ', 0, 'D', 0, 'e', 0, 'v', 0, 'i', 0,
  'c', 0, 'e', 0
};

/*
 * Vendor string.
 */
static const uint8_t dpo3034_string1[] = {
  USB_DESC_BYTE(20),                    /* bLength.                         */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  'T', 0, 'e', 0, 'k', 0, 't', 0, 'r', 0, 'o', 0, 'n', 0, 'i', 0,
  'x', 0,
};

/*
 * Device Description string.
 */
static const uint8_t dpo3034_string2[] = {
  USB_DESC_BYTE(16),                    /* bLength.                         */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  'D', 0, 'P', 0, 'O', 0, '3', 0, '0', 0, '3', 0, '4', 0,
};


/*
 * Vendor string.
 */
static const uint8_t dso9404a_string1[] = {
  USB_DESC_BYTE(38),                    /* bLength.                         */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  'K', 0, 'e', 0, 'y', 0, 's', 0, 'i', 0, 'g', 0, 'h', 0, 't', 0,
  'T', 0, 'e', 0, 'c', 0, 'h', 0, 'n', 0, 'o', 0, 'g', 0, 'i', 0,
  'e', 0, 's', 0,
};

/*
 * Device Description string.
 */
static const uint8_t dso9404a_string2[] = {
  USB_DESC_BYTE(18),                    /* bLength.                         */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  'D', 0, 'S', 0, 'O', 0, '9', 0, '4', 0, '0', 0, '4', 0, 'A', 0,
};

/*
 * Serial Number string.
 */
static const uint8_t tmc_string3[] = {
  USB_DESC_BYTE(8),                     /* bLength.                         */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* bDescriptorType.                 */
  '0' + CH_KERNEL_MAJOR, 0,
  '0' + CH_KERNEL_MINOR, 0,
  '0' + CH_KERNEL_PATCH, 0
};

/*
 * Strings wrappers array.
 */
static const USBDescriptor tmcemu_strings[] = {
  {sizeof tmc_string0, tmc_string0},
  {sizeof tmcemu_string1, tmcemu_string1},
  {sizeof tmcemu_string2, tmcemu_string2},
  {sizeof tmc_string3, tmc_string3}
};

static const USBDescriptor  dpo3034_strings[] = {
  {sizeof tmc_string0, tmc_string0},
  {sizeof dpo3034_string1, dpo3034_string1},
  {sizeof dpo3034_string2, dpo3034_string2},
  {sizeof tmc_string3, tmc_string3}
};


static const USBDescriptor dso9404a_strings[] = {
  {sizeof tmc_string0, tmc_string0},
  {sizeof dso9404a_string1, dso9404a_string1},
  {sizeof dso9404a_string2, dso9404a_string2},
  {sizeof tmc_string3, tmc_string3}
};


/*
 * Handles the GET_DESCRIPTOR callback. All required descriptors must be
 * handled here.
 */
static const USBDescriptor *get_descriptor(USBDriver *usbp,
                                           uint8_t dtype,
                                           uint8_t dindex,
                                           uint16_t lang) {

  (void)usbp;
  (void)lang;
  switch (dtype) {
  case USB_DESCRIPTOR_DEVICE:
    switch(emu_device)
    {
      case EMU_DEVICE_TEK_DPO3034:
        return &dpo3034_device_descriptor;
      case EMU_DEVICE_KEYSIGHT_DSO9404A:
        return &dso9404a_device_descriptor;
      default:
        return &tmcemu_device_descriptor;
    }
  case USB_DESCRIPTOR_CONFIGURATION:
    return &tmc_configuration_descriptor;
  case USB_DESCRIPTOR_STRING:
    if (dindex < 4)
      switch(emu_device)
      {
        case EMU_DEVICE_TEK_DPO3034:
          return &dpo3034_strings[dindex];
        case EMU_DEVICE_KEYSIGHT_DSO9404A:
          return &dso9404a_strings[dindex];
        default:
          return &tmcemu_strings[dindex];
      }
  }
  return NULL;
}

/**
 * @brief   IN EP1 state.
 */
static USBInEndpointState ep1instate;

/**
 * @brief   OUT EP1 state.
 */
static USBOutEndpointState ep1outstate;

/**
 * @brief   EP1 initialization structure (both IN and OUT).
 */
static const USBEndpointConfig ep1config = {
  USB_EP_MODE_TYPE_BULK,
  NULL,
  tmcDataTransmitted,
  tmcDataReceived,
  USB_TMC_MAX_PKT_SIZE,
  USB_TMC_MAX_PKT_SIZE,
  &ep1instate,
  &ep1outstate,
  1,
  NULL
};

/**
 * @brief   IN EP2 state.
 */
static USBInEndpointState ep2instate;

/**
 * @brief   EP2 initialization structure (IN only).
 */
static const USBEndpointConfig ep2config = {
  USB_EP_MODE_TYPE_INTR,
  NULL,
  tmcInterruptTransmitted,
  NULL,
  0x0010,
  0x0000,
  &ep2instate,
  NULL,
  1,
  NULL
};

/*
 * Handles the USB driver global events.
 */
static void usb_event(USBDriver *usbp, usbevent_t event) {

  switch (event) {
  case USB_EVENT_ADDRESS:
    return;
  case USB_EVENT_CONFIGURED:
    chSysLockFromISR();

    /* Enables the endpoints specified into the configuration.
       Note, this callback is invoked from an ISR so I-Class functions
       must be used.*/
    usbInitEndpointI(usbp, TMC_BULK_IN_EP, &ep1config);
    usbInitEndpointI(usbp, TMC_INT_IN_EP, &ep2config);

    /* Resetting the state of the CDC subsystem.*/
    tmcConfigureHookI(&TMC1);


    chSysUnlockFromISR();
    return;
  case USB_EVENT_RESET:
    /* Falls into.*/
  case USB_EVENT_UNCONFIGURED:
    /* Falls into.*/
  case USB_EVENT_SUSPEND:
    chSysLockFromISR();

    /* Disconnection event on suspend.*/
    tmcSuspendHookI(&TMC1);

    chSysUnlockFromISR();
    return;
  case USB_EVENT_WAKEUP:
    chSysLockFromISR();
    /* Disconnection event on suspend.*/
    tmcWakeupHookI(&TMC1);

    chSysUnlockFromISR();
    return;
  case USB_EVENT_STALLED:
    return;
  }
  return;
}




/*
 * Handles the USB driver global events.
 */
static void sof_handler(USBDriver *usbp) {

  (void)usbp;

  osalSysLockFromISR();
  tmcSOFHookI(&TMC1);
  osalSysUnlockFromISR();
}

virtual_timer_t indicator_vt;

void usb_init(void)
{
  palClearLine(LINE_LED_ORANGE);
  palClearLine(LINE_LED_RED);
  palClearLine(LINE_LED_GREEN);
  chVTObjectInit(&indicator_vt);
}

static uint8_t orange_state = 0;
static uint8_t green_state = 0;
static uint8_t red_state = 0;
static void indicator_step(void * arg)
{
  int step = (int) arg;
  switch(step)
  {
    case 0:
      orange_state = palReadLine(LINE_LED_ORANGE);
      green_state  = palReadLine(LINE_LED_GREEN);
      red_state    = palReadLine(LINE_LED_RED);
      palClearLine(LINE_LED_ORANGE);
      palClearLine(LINE_LED_GREEN);
      palClearLine(LINE_LED_RED);
      palSetLine(LINE_LED_ORANGE);
      break;
    case 1:
      palClearLine(LINE_LED_ORANGE);
      palSetLine(LINE_LED_RED);
      break;
    case 2:
      palClearLine(LINE_LED_RED);
      palSetLine(LINE_LED_GREEN);
      break;
    case 3:
      palClearLine(LINE_LED_GREEN);
      break;
    default:
      palWriteLine(LINE_LED_ORANGE, orange_state);
      palWriteLine(LINE_LED_GREEN, green_state);
      palWriteLine(LINE_LED_RED, red_state);
  }
  if(step < 4)
  {
    osalSysLockFromISR();
    chVTSetI(&indicator_vt, TIME_MS2I(250), indicator_step, (void*)step+1);
    osalSysUnlockFromISR();
  }
}

static void indicator_handler(USBTMCDriver *tmcp)
{
  (void)tmcp;
  indicator_step(0);
}

/*
 * USB driver configuration.
 */
const USBConfig usbcfg = {
  usb_event,
  get_descriptor,
  tmcRequestsHook,
  sof_handler
};

/*
 * Serial over USB driver configuration.
 */
const USBTMCConfig usbtmccfg = {
  &USBD1,
  TMC_BULK_IN_EP,
  TMC_BULK_OUT_EP,
  TMC_INT_IN_EP,
  indicator_handler
};


