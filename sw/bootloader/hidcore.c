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

#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/hid.h>
#include <libopencm3/stm32/desig.h>

#include "usbhid.h"
#include "hidprog.h"

#define ARRAY_SIZE(x)  (sizeof(x)/sizeof(x[0]))

#define ADDR_IN USB_ENDPOINT_ADDR_IN(1)
#define ADDR_OUT USB_ENDPOINT_ADDR_OUT(1)
#define HID_SET_REPORT 0x09

static usbd_device *usbdev;
static uint8_t usbd_control_buffer[256];

static uint32_t max_address;
static uint32_t flash_size;

const struct usb_device_descriptor devdesc = {
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

static const uint8_t hid_report_descriptor[] = {
    0x06, 0x00, 0xff,   // USAGE_PAGE (Vendor Defined)
    0x09, 0x01,         // USAGE (Vendor Usage 1)
    0xa1, 0x01,         // COLLECTION (Application)
    0x19, 0x01,         //   USAGE_MINIMUM (0)
    0x29, 0x40,         //   USAGE_MAXIMIM (64)
    0x15, 0x01,         //   LOGICAL_MINIMUM (0)        
	0x26, 0xFF, 0x00,    //   LOGICAL_MAXIMUM (255)        
    0x75, 0x08,         //   REPORT_SIZE (8)
    0x85, 0x01,         //   REPORT_ID (1)
    0x95, 0x40,         //   REPORT_COUNT (64)
    0x81, 0x00,         //   Input (Data, Array, Abs): Instantiates input packet fields based on the above report size, count, logical min/max, and usage.
    0x19, 0x01,         //   USAGE_MINIMUM (0)
    0x29, 0x40,         //   USAGE_MAXIMIM (64)
    0x91, 0x00,         //   Output (Data, Array, Abs): Instantiates output packet fields.  Uses same report size and count as "Input" fields, since nothing new/different was specified to the parser since the "Input" item.
    0xC0                // End Collection
};   


static const struct {
    struct usb_hid_descriptor hid_descriptor;
    struct {
        uint8_t bReportDescriptorType;
        uint16_t wDescriptorLength;
    } __attribute__((packed)) hid_report;
} __attribute__((packed)) hid_function = {
    .hid_descriptor = {
        .bLength = sizeof(hid_function),
        .bDescriptorType = USB_DT_HID,
        .bcdHID = 0x0100,
        .bCountryCode = 0,
        .bNumDescriptors = 1,
    },
    .hid_report = {
        .bReportDescriptorType = USB_DT_REPORT,
        .wDescriptorLength = sizeof(hid_report_descriptor),
    }
};

static const struct usb_endpoint_descriptor hid_endpoints[] = {
    {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = ADDR_IN,
        .bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
        .wMaxPacketSize = 64,
        .bInterval = 1,
    },{
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = ADDR_OUT,
        .bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
        .wMaxPacketSize = 64,
        .bInterval = 1,
    }
};

static const struct usb_interface_descriptor iface = {
    .bLength            = USB_DT_INTERFACE_SIZE,
    .bDescriptorType    = USB_DT_INTERFACE,
    .bInterfaceNumber   = 0,
    .bAlternateSetting  = 0,
    .bNumEndpoints      = ARRAY_SIZE(hid_endpoints),
    .bInterfaceClass    = USB_CLASS_HID,
    .bInterfaceSubClass = 0x00,
    .bInterfaceProtocol = 0x00,
    .iInterface         = 0,

    .endpoint           = &hid_endpoints[0],

    .extra              = &hid_function,
    .extralen           = sizeof(hid_function),
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

static const char *usb_strings[] = {
    "www.paulroukema.com",
    "scope-footswitch Bootloader",
    serial_no,
};


static struct {
    uint32_t addr;
} state;



static uint32_t get_le32(uint8_t buf[4])
{
	return buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
}

static void put_le32(uint8_t buf[4], uint32_t val)
{
    buf[0] = (val >> 0) & 0xFF;
    buf[1] = (val >> 8) & 0xFF;
    buf[2] = (val >> 16) & 0xFF;
    buf[3] = (val >> 24) & 0xFF;
}



static void hid_get_flash_size(void) {
#define FLASH_SIZE_R 0x1fff7A22


    /* Calculated the upper flash limit from the exported data
       in theparameter block*/
    flash_size = *(uint32_t *)FLASH_SIZE_R & 0xfff;
    flash_size <<= 10;
    max_address = 0x8000000 + (flash_size);
}

typedef void (hidprog_cmd_callback)(hidprog_command_t* cmd, hidprog_response_t* rsp);

static void process_ping(hidprog_command_t* cmd, hidprog_response_t* rsp)
{
    static int i = 0;;
    rsp->ping.echo = cmd->ping.echo;
    rsp->ping.count = i++;
}

static void process_getinfo(hidprog_command_t* cmd, hidprog_response_t* rsp)
{
    (void) cmd;
    rsp->getinfo.version = 0;
    rsp->getinfo.magic[3] = 0x76;
    rsp->getinfo.magic[2] = 0x54;
    rsp->getinfo.magic[1] = 0x32;
    rsp->getinfo.magic[0] = 0x10;
    rsp->getinfo.flash_size[0] = (flash_size >> 0) & 0xFF; 
    rsp->getinfo.flash_size[1] = (flash_size >> 8) & 0xFF; 
    rsp->getinfo.flash_size[2] = (flash_size >> 16) & 0xFF; 
    rsp->getinfo.flash_size[3] = (flash_size >> 24) & 0xFF; 
    rsp->getinfo.block_count[0] = 1;
    rsp->getinfo.block_count[1] = 3;
    rsp->getinfo.block_count[2] = 1;
    rsp->getinfo.block_size[0] = 14 | HIDPROG_GETINFO_BLOCKSIZE_READONY;
    rsp->getinfo.block_size[1] = 14;
    rsp->getinfo.block_size[2] = 16;
}

static void process_setaddr(hidprog_command_t* cmd, hidprog_response_t* rsp)
{
    (void)rsp;
    // Align to 32-bits
    state.addr = get_le32(cmd->setaddr.addr) & 0xFFFFFFFC;
}

static void process_getaddr(hidprog_command_t* cmd, hidprog_response_t* rsp)
{
    (void)cmd;
    
    put_le32(rsp->getaddr.addr, state.addr);
}

static void process_writedata(hidprog_command_t* cmd, hidprog_response_t* rsp)
{
    (void)rsp;
    int len = 4*(cmd->writedata.len & HIDPROG_WRITEDATA_LEN_LEN);
    dfu_get_sector_num(state.addr);
    flash_unlock();
    if(cmd->writedata.len & HIDPROG_WRITEDATA_LEN_MAY_ERASE)
        dfu_check_and_do_sector_erase(state.addr);
    dfu_flash_program_buffer(state.addr, cmd->writedata.data, len);
    flash_lock();
    state.addr += len;
}

static void process_readdata(hidprog_command_t* cmd, hidprog_response_t* rsp)
{
    (void)rsp;
    int len = 4*(cmd->readdata.len & HIDPROG_WRITEDATA_LEN_LEN);
    memcpy(rsp->readdata.data, (void*)state.addr, len);
    rsp->readdata.len =  len/4;
    state.addr += len;
}

static int do_reboot = 0;
static void process_finish(hidprog_command_t* cmd, hidprog_response_t* rsp)
{
    (void)rsp;
    if(cmd->finish.flags & HIDPROG_FINISH_FLAG_REBOOT)
    {
        do_reboot = 1;
    }
}


static hidprog_cmd_callback* cmd_callbacks[] = {
    [HIDPROG_ID_PING] = process_ping,
    [HIDPROG_ID_SETADDR] = process_setaddr,
    [HIDPROG_ID_GETADDR] = process_getaddr,
    [HIDPROG_ID_WRITEDATA] = process_writedata,
    [HIDPROG_ID_READDATA] = process_readdata,
    [HIDPROG_ID_FINISH] = process_finish,
    [HIDPROG_ID_GETINFO] = process_getinfo,
};

static void process_cmd(usbd_device *dev, hidprog_command_t* cmd) {
    hidprog_response_t rsp = {0};
    rsp.id = cmd->id;
    if(cmd->id < ARRAY_SIZE(cmd_callbacks) && cmd_callbacks[cmd->id])
        cmd_callbacks[cmd->id](cmd, &rsp);
    else
        rsp.id = HIDPROG_ID_UNKNOWN;

    usbd_ep_write_packet(dev, ADDR_IN, &(rsp.bytes), sizeof(rsp.bytes));
}

static void hid_rx_cb(usbd_device *dev, uint8_t ep)
{
	(void)ep;

	char buf[64];

    hidprog_command_t cmd;

	int len = usbd_ep_read_packet(dev, ADDR_OUT, &(cmd.bytes), sizeof(cmd.bytes));

    process_cmd(dev, &cmd);

}

static enum usbd_request_return_codes hid_descriptor_request(usbd_device *dev, struct usb_setup_data *req, uint8_t **buf, uint16_t *len,
			void (**complete)(usbd_device *dev, struct usb_setup_data *req))
{
	(void)complete;
	(void)dev;

	if((req->bmRequestType != 0x81) ||
	   (req->bRequest != USB_REQ_GET_DESCRIPTOR) ||
	   (req->wValue != 0x2200))
		return USBD_REQ_NOTSUPP;

	/* Handle the HID report descriptor. */
	*buf = (uint8_t *)hid_report_descriptor;
	*len = sizeof(hid_report_descriptor);

	return USBD_REQ_HANDLED;
}

static void hid_set_config(usbd_device *dev, uint16_t wValue)
{
	(void)wValue;

    usbd_ep_setup(dev, ADDR_OUT, USB_ENDPOINT_ATTR_INTERRUPT, 64, hid_rx_cb);
	usbd_ep_setup(dev, ADDR_IN , USB_ENDPOINT_ATTR_INTERRUPT, 64, NULL);

	usbd_register_control_callback( dev,
				USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_INTERFACE,
				USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
				hid_descriptor_request);
}

void hid_init(const usbd_driver *driver) {
    desig_get_unique_id_as_dfu(serial_no);
    hid_get_flash_size();

    usbdev = usbd_init(driver, &devdesc, &config, usb_strings, ARRAY_SIZE(usb_strings),
                       usbd_control_buffer, sizeof(usbd_control_buffer));
	usbd_register_set_config_callback(usbdev, hid_set_config);

}

void hid_main(void) {
    while (1){
        usbd_poll(usbdev);
        if(do_reboot)
            dfu_detach();
    }
}



