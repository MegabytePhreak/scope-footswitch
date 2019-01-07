/*
 * This file is part of the scope-footswitch project.
 *
 * Copyright (C) 2013 Gareth McMullin <gareth@blacksphere.co.nz>
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

#include <string.h>
#include <libopencm3/stm32/f4/flash.h>
#define DFU_IFACE_STRING                                                       \
    "@Internal Flash  /0x08000000/1*016Ka,3*016Kg,1*064Kg"

#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/dfu.h>
#include <libopencm3/stm32/desig.h>

#include "usbdfu.h"

static usbd_device *usbdev;
/* We need a special large control buffer for this device: */
static uint8_t usbd_control_buffer[1024];

static uint32_t max_address;

static enum dfu_state usbdfu_state = STATE_DFU_IDLE;

static struct {
    uint8_t  buf[sizeof(usbd_control_buffer)];
    uint16_t len;
    uint32_t addr;
    uint16_t blocknum;
} prog;
static uint8_t current_error;

static const struct usb_device_descriptor dev = {
    .bLength            = USB_DT_DEVICE_SIZE,
    .bDescriptorType    = USB_DT_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0,
    .bDeviceSubClass    = 0,
    .bDeviceProtocol    = 0,
    .bMaxPacketSize0    = 64,
    .idVendor           = 0x1d50,
    .idProduct          = 0x613b,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 1,
    .iProduct           = 2,
    .iSerialNumber      = 3,
    .bNumConfigurations = 1,
};

static const struct usb_dfu_descriptor dfu_function = {
    .bLength         = sizeof(struct usb_dfu_descriptor),
    .bDescriptorType = DFU_FUNCTIONAL,
    .bmAttributes =
        USB_DFU_CAN_DOWNLOAD | USB_DFU_CAN_UPLOAD | USB_DFU_WILL_DETACH,
    .wDetachTimeout = 255,
    .wTransferSize  = 1024,
    .bcdDFUVersion  = 0x011A,
};

static const struct usb_interface_descriptor iface = {
    .bLength            = USB_DT_INTERFACE_SIZE,
    .bDescriptorType    = USB_DT_INTERFACE,
    .bInterfaceNumber   = 0,
    .bAlternateSetting  = 0,
    .bNumEndpoints      = 0,
    .bInterfaceClass    = 0xFE, /* Device Firmware Upgrade */
    .bInterfaceSubClass = 1,
    .bInterfaceProtocol = 2,

    /* The ST Microelectronics DfuSe application needs this string.
     * The format isn't documented... */
    .iInterface = 4,

    .extra    = &dfu_function,
    .extralen = sizeof(dfu_function),
};

static const struct usb_interface ifaces[] = {{
    .num_altsetting = 1,
    .altsetting     = &iface,
}};

static const struct usb_config_descriptor config = {
    .bLength             = USB_DT_CONFIGURATION_SIZE,
    .bDescriptorType     = USB_DT_CONFIGURATION,
    .wTotalLength        = 0,
    .bNumInterfaces      = 1,
    .bConfigurationValue = 1,
    .iConfiguration      = 0,
    .bmAttributes        = 0xC0,
    .bMaxPower           = 0x32,

    .interface = ifaces,
};

static char serial_no[9];
static char if_string[] = DFU_IFACE_STRING;

static const char *usb_strings[] = {
    "www.paulroukema.com",
    "scope-footswitch Bootloader",
    serial_no,
    /* This string is used by ST Microelectronics' DfuSe utility */
    if_string,
};

static uint32_t get_le32(const void *vp) {
    const uint8_t *p = vp;
    return ((uint32_t)p[3] << 24) + ((uint32_t)p[2] << 16) + (p[1] << 8) + p[0];
}

static uint8_t usbdfu_getstatus(uint32_t *bwPollTimeout) {
    switch (usbdfu_state) {
        case STATE_DFU_DNLOAD_SYNC:
            usbdfu_state   = STATE_DFU_DNBUSY;
            *bwPollTimeout = dfu_poll_timeout(
                prog.buf[0], get_le32(prog.buf + 1), prog.blocknum);
            return DFU_STATUS_OK;

        case STATE_DFU_MANIFEST_SYNC:
            /* Device will reset when read is complete */
            usbdfu_state = STATE_DFU_MANIFEST;
            return DFU_STATUS_OK;
        case STATE_DFU_ERROR:
            return current_error;
        default:
            return DFU_STATUS_OK;
    }
}

static void usbdfu_getstatus_complete(usbd_device *          mydev,
                                      struct usb_setup_data *req) {
    (void)req;
    (void)mydev;

    switch (usbdfu_state) {
        case STATE_DFU_DNBUSY:

            flash_unlock();
            if (prog.blocknum == 0) {
                int32_t addr = get_le32(prog.buf + 1);
                switch (prog.buf[0]) {
                    case CMD_ERASE:
                        dfu_check_and_do_sector_erase(addr);
                }
            } else {
                uint32_t baseaddr = prog.addr + ((prog.blocknum - 2) *
                                                 dfu_function.wTransferSize);
                dfu_flash_program_buffer(baseaddr, prog.buf, prog.len);
            }
            flash_lock();

            /* We jump straight to dfuDNLOAD-IDLE,
             * skipping dfuDNLOAD-SYNC
             */
            usbdfu_state = STATE_DFU_DNLOAD_IDLE;
            return;

        case STATE_DFU_MANIFEST:
            dfu_detach();
            return; /* Will never return */
        default:
            return;
    }
}

static enum usbd_request_return_codes usbdfu_control_request(
    usbd_device *mydev, struct usb_setup_data *req, uint8_t **buf,
    uint16_t *len,
    void (**complete)(usbd_device *dev, struct usb_setup_data *req)) {
    (void)mydev;

    if ((req->bmRequestType & 0x7F) != 0x21)
        return 0; /* Only accept class request */

    switch (req->bRequest) {
        case DFU_DNLOAD:
            if ((len == NULL) || (*len == 0)) {
                usbdfu_state = STATE_DFU_MANIFEST_SYNC;
                return 1;
            } else {
                /* Copy download data for use on GET_STATUS */
                prog.blocknum = req->wValue;
                prog.len      = *len;
                memcpy(prog.buf, *buf, *len);
                if ((req->wValue == 0) && (prog.buf[0] == CMD_SETADDR)) {
                    uint32_t addr = get_le32(prog.buf + 1);
                    if ((addr < app_address) || (addr >= max_address)) {
                        current_error = DFU_STATUS_ERR_TARGET;
                        usbdfu_state  = STATE_DFU_ERROR;
                        return 1;
                    } else
                        prog.addr = addr;
                }
                usbdfu_state = STATE_DFU_DNLOAD_SYNC;
                return 1;
            }
        case DFU_CLRSTATUS:
            /* Clear error and return to dfuIDLE */
            if (usbdfu_state == STATE_DFU_ERROR)
                usbdfu_state = STATE_DFU_IDLE;
            return 1;
        case DFU_ABORT:
            /* Abort returns to dfuIDLE state */
            usbdfu_state = STATE_DFU_IDLE;
            return 1;
        case DFU_UPLOAD:
            if ((usbdfu_state == STATE_DFU_IDLE) ||
                (usbdfu_state == STATE_DFU_DNLOAD_IDLE) ||
                (usbdfu_state == STATE_DFU_UPLOAD_IDLE)) {
                prog.blocknum = req->wValue;
                usbdfu_state  = STATE_DFU_UPLOAD_IDLE;
                if (prog.blocknum > 1) {
                    uint32_t baseaddr =
                        prog.addr +
                        ((prog.blocknum - 2) * dfu_function.wTransferSize);
                    memcpy(*buf, (void *)baseaddr, *len);
                }
                return 1;
            } else {
                usbd_ep_stall_set(mydev, 0, 1);
                return 0;
            }
        case DFU_GETSTATUS: {
            uint32_t bwPollTimeout = 0; /* 24-bit integer in DFU class spec */

            (*buf)[0] = usbdfu_getstatus(&bwPollTimeout);
            (*buf)[1] = bwPollTimeout & 0xFF;
            (*buf)[2] = (bwPollTimeout >> 8) & 0xFF;
            (*buf)[3] = (bwPollTimeout >> 16) & 0xFF;
            (*buf)[4] = usbdfu_state;
            (*buf)[5] = 0; /* iString not used here */
            *len      = 6;

            *complete = usbdfu_getstatus_complete;

            return 1;
        }
        case DFU_GETSTATE:
            /* Return state with no state transision */
            *buf[0] = usbdfu_state;
            *len    = 1;
            return 1;
    }

    return 0;
}

static void dfu_get_flash_size(void) {
#define FLASH_SIZE_R 0x1fff7A22

    uint32_t fuse_flash_size;

    /* Calculated the upper flash limit from the exported data
       in theparameter block*/
    fuse_flash_size = *(uint32_t *)FLASH_SIZE_R & 0xfff;
    if (fuse_flash_size == 0x40) /* Handle F103x8 as F103xC! */
        fuse_flash_size = 0x80;
    max_address = 0x8000000 + (fuse_flash_size << 10);
}

void dfu_init(const usbd_driver *driver) {
    desig_get_unique_id_as_dfu(serial_no);
    dfu_get_flash_size();

    usbdev = usbd_init(driver, &dev, &config, usb_strings, 4,
                       usbd_control_buffer, sizeof(usbd_control_buffer));

    usbd_register_control_callback(
        usbdev, USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
        USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT, usbdfu_control_request);
}

void dfu_main(void) {
    while (1)
        usbd_poll(usbdev);
}

#if defined(DFU_IFACE_STRING_OFFSET)
static void set_dfu_iface_string(uint32_t size) {
    uint32_t res;
    char *   p = if_string + DFU_IFACE_STRING_OFFSET;
    /* We do not want the whole printf library in the bootloader.
     * Fill the size digits by hand.
     */
    res = size / 100;
    if (res > 9) {
        *p++ = '9';
        *p++ = '9';
        *p++ = '9';
        return;
    } else {
        *p++ = res + '0';
        size -= res * 100;
    }
    res  = size / 10;
    *p++ = res + '0';
    size -= res * 10;
    *p++ = size + '0';
}
#else
#define set_dfu_iface_string()
#endif
