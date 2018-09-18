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

#include "hal.h"
#include "usbh_usbtmc.h"
#include "usbh/internal.h"


#if HAL_USBH_USE_ADDITIONAL_CLASS_DRIVERS

#include <string.h>
#include "usbh_usbtmc.h"
#include "usbh/internal.h"

#if USBH_DEBUG_ENABLE_TRACE
#define udbgf(f, ...)  usbDbgPrintf(f, ##__VA_ARGS__)
#define udbg(f, ...)  usbDbgPuts(f, ##__VA_ARGS__)
#else
#define udbgf(f, ...)  do {} while(0)
#define udbg(f, ...)   do {} while(0)
#endif

#if USBH_DEBUG_ENABLE_INFO
#define uinfof(f, ...)  usbDbgPrintf(f, ##__VA_ARGS__)
#define uinfo(f, ...)  usbDbgPuts(f, ##__VA_ARGS__)
#else
#define uinfof(f, ...)  do {} while(0)
#define uinfo(f, ...)   do {} while(0)
#endif

#if USBH_DEBUG_ENABLE_WARNINGS
#define uwarnf(f, ...)  usbDbgPrintf(f, ##__VA_ARGS__)
#define uwarn(f, ...)  usbDbgPuts(f, ##__VA_ARGS__)
#else
#define uwarnf(f, ...)  do {} while(0)
#define uwarn(f, ...)   do {} while(0)
#endif

#if USBH_DEBUG_ENABLE_ERRORS
#define uerrf(f, ...)  usbDbgPrintf(f, ##__VA_ARGS__)
#define uerr(f, ...)  usbDbgPuts(f, ##__VA_ARGS__)
#else
#define uerrf(f, ...)  do {} while(0)
#define uerr(f, ...)   do {} while(0)
#endif


#define USBTMC_INTERFACE_CLASS    0xFE
#define USBTMC_INTERFACE_SUBCLASS 0x03
#define USBTMC_INTERFACE_PROTOCOL_USBTMC 0x0
#define USBTMC_INTERFACE_PROTOCOL_USB488 0x1

enum {
	USBH_TMC_REQ_GET_CAPABILITIES= 7,
	USBH_TMC_REQ_INDICATOR_PULSE = 64,
};

enum {
	USBH_TMC_MSGID_DEV_DEP_MSG_OUT = 1,
	USBH_TMC_MSGID_REQUEST_DEV_DEP_MSG_IN = 2,
	USBH_TMC_MSGID_DEV_DEP_MSG_IN = 2,
};

enum {
	USBH_TMC_ATTRIBUTE_EOM = 1,
};

struct dev_dep_msg_out_hdr {
	uint8_t bMsgId;
	uint8_t bTag;
	uint8_t bTagInverse;
	uint8_t bReserved0;
	uint32_t dwTransferSize;
	uint8_t bmTransferAttributes;
	uint8_t dwReserved1[3];
};

struct dev_dep_msg_in_hdr {
	uint8_t bMsgId;
	uint8_t bTag;
	uint8_t bTagInverse;
	uint8_t bReserved0;
	uint32_t dwTransferSize;
	uint8_t bmTransferAttributes;
	uint8_t dwReserved1[3];
};

struct dev_dep_request_msg_in_hdr {
	uint8_t bMsgId;
	uint8_t bTag;
	uint8_t bTagInverse;
	uint8_t bReserved0;
	uint32_t dwTransferSize;
	uint8_t bmTransferAttributes;
	uint8_t bTermChar;
	uint8_t dwReserved1[2];
};

/*===========================================================================*/
/* USB Class driver loader for Custom Class Example				 		 	 */
/*===========================================================================*/

static USBH_DEFINE_BUFFER(uint8_t inbuf[USBH_TMC_MAX_INSTANCES][USBH_TMC_BUF_SIZE]);
static USBH_DEFINE_BUFFER(uint8_t outbuf[USBH_TMC_MAX_INSTANCES][USBH_TMC_BUF_SIZE]);
USBHTmcDriver USBHTMCD[USBH_TMC_MAX_INSTANCES];

static void _tmc_init(void);
static usbh_baseclassdriver_t *_load(usbh_device_t *dev, const uint8_t *descriptor, uint16_t rem);
static void _unload(usbh_baseclassdriver_t *drv);
static void _stop_locked(USBHTmcDriver *tmcp);

static const usbh_classdriver_vmt_t class_driver_vmt = {
	_tmc_init,
	_load,
	_unload
};

const usbh_classdriverinfo_t usbhTmcClassDriverInfo = {
	"USBTMC", &class_driver_vmt
};

static usbh_baseclassdriver_t *_load(usbh_device_t *dev, const uint8_t *descriptor, uint16_t rem) {
	int i;
	USBHTmcDriver *tmcp;

	if (_usbh_match_descriptor(descriptor, rem, USBH_DT_INTERFACE,
			USBTMC_INTERFACE_CLASS, USBTMC_INTERFACE_SUBCLASS, -1) != HAL_SUCCESS)
		return NULL;

	const usbh_interface_descriptor_t * const ifdesc = (const usbh_interface_descriptor_t *)descriptor;
	if(ifdesc->bInterfaceProtocol != USBTMC_INTERFACE_PROTOCOL_USBTMC &&
		ifdesc->bInterfaceProtocol != USBTMC_INTERFACE_PROTOCOL_USB488 )
		return NULL;

	/* alloc driver */
	for (i = 0; i < USBH_TMC_MAX_INSTANCES; i++) {
		if (USBHTMCD[i].dev == NULL) {
			tmcp = &USBHTMCD[i];
			goto alloc_ok;
		}
	}

	uwarn("Can't alloc USBTMC driver");

	/* can't alloc */
	return NULL;

alloc_ok:
	/* initialize the driver's variables */
	tmcp->epin.status = USBH_EPSTATUS_UNINITIALIZED;
	tmcp->epout.status = USBH_EPSTATUS_UNINITIALIZED;
	tmcp->epint.status = USBH_EPSTATUS_UNINITIALIZED;
	tmcp->ifnum = ifdesc->bInterfaceNumber;
	usbhEPSetName(&dev->ctrl, "TMC[CTRL]");


	/* parse the configuration descriptor */
	if_iterator_t iif;
	generic_iterator_t iep;
	iif.iad = 0;
	iif.curr = descriptor;
	iif.rem = rem;
	for (ep_iter_init(&iep, &iif); iep.valid; ep_iter_next(&iep)) {
		const usbh_endpoint_descriptor_t *const epdesc = ep_get(&iep);
		if ((epdesc->bEndpointAddress & 0x80) && (epdesc->bmAttributes == USBH_EPTYPE_BULK)) {
			uinfof("BULK IN endpoint found: bEndpointAddress=%02x", epdesc->bEndpointAddress);
			usbhEPObjectInit(&tmcp->epin, dev, epdesc);
			usbhEPSetName(&tmcp->epin, "TMC[BIN ]");
		} else if (((epdesc->bEndpointAddress & 0x80) == 0)
				&& (epdesc->bmAttributes == USBH_EPTYPE_BULK)) {
			uinfof("BULK OUT endpoint found: bEndpointAddress=%02x", epdesc->bEndpointAddress);
			usbhEPObjectInit(&tmcp->epout, dev, epdesc);
			usbhEPSetName(&tmcp->epout, "TMC[BOUT]");
		} else if ((epdesc->bEndpointAddress & 0x80) && (epdesc->bmAttributes == USBH_EPTYPE_INT)) {
			uinfof("INT IN endpoint found: bEndpointAddress=%02x", epdesc->bEndpointAddress);
			usbhEPObjectInit(&tmcp->epint, dev, epdesc);
			usbhEPSetName(&tmcp->epint, "TMC[INT ]");
		} else {
			uwarnf("unsupported endpoint found: bEndpointAddress=%02x, bmAttributes=%02x",
					epdesc->bEndpointAddress, epdesc->bmAttributes);
		}
	}

	if ((tmcp->epin.status != USBH_EPSTATUS_CLOSED) || (tmcp->epout.status != USBH_EPSTATUS_CLOSED)) {
		goto deinit;
	}

	if(ifdesc->bInterfaceProtocol == USBTMC_INTERFACE_PROTOCOL_USBTMC)
	{
		uinfo("TMC: Basic USBTMC protocol device fount");
	}else if(ifdesc->bInterfaceProtocol == USBTMC_INTERFACE_PROTOCOL_USB488)
	{
		uinfo("TMC: USB488 protocol device found");
	}

	tmcp->state = USBHTMC_STATE_ACTIVE;

	return (usbh_baseclassdriver_t *)tmcp;
deinit:
	/* Here, the enpoints are closed, and the driver is unlinked */
	return NULL;
}

static void _unload(usbh_baseclassdriver_t *drv) {
	USBHTmcDriver * tmcp = (USBHTmcDriver*) drv;
	chSemWait(&tmcp->sem);
	_stop_locked(tmcp);
	tmcp->state = USBHTMC_STATE_STOP;
	chSemSignal(&tmcp->sem);
}

static void _stop_locked(USBHTmcDriver *tmcp) {
	if (tmcp->state == USBHTMC_STATE_ACTIVE)
		return;

	osalDbgCheck(tmcp->state == USBHTMC_STATE_READY);

	usbhEPClose(&tmcp->epin);
	usbhEPClose(&tmcp->epout);
	if (tmcp->epint.status != USBH_EPSTATUS_UNINITIALIZED) {
		usbhEPClose(&tmcp->epint);
	}
	tmcp->state = USBHTMC_STATE_ACTIVE;
}


void usbhtmcStart(USBHTmcDriver *tmcp) {
	osalDbgCheck(tmcp);

	chSemWait(&tmcp->sem);
	if (tmcp->state == USBHTMC_STATE_READY) {
		chSemSignal(&tmcp->sem);
		return;
	}
	osalDbgCheck(tmcp->state == USBHTMC_STATE_ACTIVE);

	/* open the int IN/OUT endpoints */
	usbhEPOpen(&tmcp->epin);
	usbhEPOpen(&tmcp->epout);
	if (tmcp->epint.status == USBH_EPSTATUS_CLOSED) {
		usbhEPOpen(&tmcp->epint);
	}

	tmcp->state = USBHTMC_STATE_READY;
	chSemSignal(&tmcp->sem);
}

void usbhtmcStop(USBHTmcDriver *tmcp) {
	chSemWait(&tmcp->sem);
	_stop_locked(tmcp);
	chSemSignal(&tmcp->sem);
}

static uint8_t _get_next_tag(USBHTmcDriver * tmcp)
{
	return (tmcp->last_btag = (tmcp->last_btag % 255) + 1);
}

static size_t _write_locked(USBHTmcDriver * tmcp, const char *data, size_t n, systime_t timeout)
{
	if(n+sizeof(struct dev_dep_msg_out_hdr) > tmcp->epout.wMaxPacketSize)
	{
		uerrf("Message size %u exceeds, max packet size %u. Multi-transfer packets not yet implemented",
			n, tmcp->epout.wMaxPacketSize);
		return 0;
	}
	if(tmcp->state != USBHTMC_STATE_READY)
	{
		uinfo("[TMC] Aborted write due to driver not ready");
		return 0;
	}


	uint8_t * buf = outbuf[tmcp->index];
	struct dev_dep_msg_out_hdr hdr = {};
	hdr.bMsgId = USBH_TMC_MSGID_DEV_DEP_MSG_OUT;
	hdr.bTag = _get_next_tag(tmcp);
	hdr.bTagInverse = ~hdr.bTag & 0xFF;
	hdr.dwTransferSize = n;
	hdr.bmTransferAttributes = USBH_TMC_ATTRIBUTE_EOM;

	memcpy(buf, &hdr, sizeof(hdr));
	memcpy(buf+sizeof(hdr), data, n);

	size_t len = sizeof(hdr) + ((n+3)/4) * 4;

	usbh_urbstatus_t status = usbhBulkTransfer(&tmcp->epout, buf, len, NULL, timeout);

	if(status == USBH_URBSTATUS_TIMEOUT)
	{
		uinfo("[TMC] Write timeout");
	}

	if (status != USBH_URBSTATUS_OK) {
		uerrf("[TMC] Write status = %d (!= OK)", status);
		return 0;
	}
	return n;
}


static usbh_urbstatus_t  _dev_dep_request_msg_in(USBHTmcDriver * tmcp, size_t len, systime_t timeout)
{

	uint8_t * buf = outbuf[tmcp->index];
	struct dev_dep_request_msg_in_hdr req_hdr = {};
	req_hdr.bMsgId = USBH_TMC_MSGID_REQUEST_DEV_DEP_MSG_IN;
	req_hdr.bTag = _get_next_tag(tmcp);
	req_hdr.bTagInverse = ~req_hdr.bTag & 0xFF;
	req_hdr.dwTransferSize = len;
	req_hdr.bmTransferAttributes = 0;

	memcpy(buf, &req_hdr, sizeof(req_hdr));
	return usbhBulkTransfer(&tmcp->epout, buf, sizeof(req_hdr), NULL, timeout);
}

static size_t _read_locked(USBHTmcDriver * tmcp, char *data, size_t n, systime_t timeout)
{
	if(tmcp->state != USBHTMC_STATE_READY)
	{
		uinfo("[TMC] Aborted read due to driver not ready");
		return 0;
	}

	size_t read_len = USBH_TMC_BUF_SIZE-sizeof(struct dev_dep_msg_in_hdr);

	bool eom = FALSE;
	size_t bytes_received = 0;

	while(!eom)
	{
		if(n-bytes_received < read_len)
			read_len = n-bytes_received;

		usbh_urbstatus_t status = _dev_dep_request_msg_in(tmcp, read_len, timeout);
		if(status == USBH_URBSTATUS_TIMEOUT)
		{
			uinfo("[TMC] Read request timeout");
		}
		if (status != USBH_URBSTATUS_OK) {
			uerrf("[TMC] Read request status = %d (!= OK)", status);
			return 0;
		}


		uint32_t len = 0;
		status =  usbhBulkTransfer(&tmcp->epin, inbuf[tmcp->index], sizeof(struct dev_dep_msg_in_hdr) + ((read_len+3)/4)*4,
			 &len, timeout);

		if(status == USBH_URBSTATUS_TIMEOUT)
		{
			uinfo("[TMC] Read in timeout");
		}
		if (status != USBH_URBSTATUS_OK) {
			uerrf("[TMC] Read in status = %d (!= OK)", status);
			return 0;
		}

		struct dev_dep_msg_in_hdr hdr;
		memcpy(&hdr, inbuf[tmcp->index], sizeof(hdr));

		if(hdr.bMsgId != USBH_TMC_MSGID_DEV_DEP_MSG_IN){
			uerrf("Unexpected read MsgId %u, expected %u", hdr.bMsgId, USBH_TMC_MSGID_DEV_DEP_MSG_IN);
			return 0;
		}
		if(/*hdr.bTag != tmcp->last_btag ||*/ hdr.bTagInverse != (uint8_t)(~hdr.bTag) )
		{
			uerrf("Bad read tag %02x, inverse %02x. Expected %02x", hdr.bTag, hdr.bTagInverse, tmcp->last_btag);
			return 0;
		}
		eom = (hdr.bmTransferAttributes & USBH_TMC_ATTRIBUTE_EOM);
		if(hdr.dwTransferSize > n)
		{
			uerrf("Read size %u larger than request %u", hdr.dwTransferSize, n);
			return 0;
		}
		if(len < hdr.dwTransferSize + sizeof(hdr))
		{
			uerrf("Received data length %u less than indicated %u", len, hdr.dwTransferSize + sizeof(hdr));
			return 0;
		}
		memcpy(data+bytes_received, inbuf[tmcp->index] + sizeof(hdr), hdr.dwTransferSize);
		bytes_received += hdr.dwTransferSize;
		data[bytes_received] = 0;
		if(n-bytes_received <= 0)
			break;
	}

	return bytes_received;
}

size_t usbhtmcWrite(USBHTmcDriver * tmcp, const char *data, size_t n, systime_t timeout)
{
	osalDbgCheck(tmcp);
	chSemWait(&tmcp->sem);

	size_t len = _write_locked(tmcp, data, n, timeout);

	chSemSignal(&tmcp->sem);
	return len;
}

size_t usbhtmcRead(USBHTmcDriver * tmcp, char *data, size_t n, systime_t timeout)
{
	osalDbgCheck(tmcp);
	chSemWait(&tmcp->sem);

	size_t len = _read_locked(tmcp, data, n, timeout);

	chSemSignal(&tmcp->sem);
	return len;
}


size_t usbhtmcAsk(USBHTmcDriver * tmcp, const char *query, size_t querylen, char* answer, size_t answerlen, systime_t timeout)
{
	osalDbgCheck(tmcp);
	chSemWait(&tmcp->sem);

	size_t len = _write_locked(tmcp, query, querylen, timeout);
	if(len == querylen)
	{
		len = _read_locked(tmcp, answer, answerlen, timeout);
	} else {
		len = 0;
	}
	chSemSignal(&tmcp->sem);
	return len;
}




usbh_urbstatus_t usbhtmcIndicatorPulse(USBHTmcDriver * tmcp, uint8_t* status) {
	osalDbgCheck(tmcp);
    USBH_DEFINE_BUFFER(uint8_t buf);

	uint8_t* bufp = status;
	if(!status)
		bufp = &buf;
	return usbhControlRequest(tmcp->dev,
			USBH_REQTYPE_CLASSIN(USBH_REQTYPE_RECIP_INTERFACE), USBH_TMC_REQ_INDICATOR_PULSE,
			0, tmcp->ifnum, 1, bufp);
}

usbh_urbstatus_t usbhtmcGetCapabilities(USBHTmcDriver * tmcp, USBHTmcCapabilities* capp) {
	osalDbgCheck(tmcp && capp);
	return usbhControlRequest(tmcp->dev,
			USBH_REQTYPE_CLASSIN(USBH_REQTYPE_RECIP_INTERFACE), USBH_TMC_REQ_GET_CAPABILITIES,
			0, tmcp->ifnum, sizeof(*capp), (uint8_t *)capp);
}

static void _tmc_object_init(USBHTmcDriver *tmcp, size_t index) {
	osalDbgCheck(tmcp != NULL);
	memset(tmcp, 0, sizeof(*tmcp));
	tmcp->info = &usbhTmcClassDriverInfo;
	tmcp->index = index;
	chSemObjectInit(&tmcp->sem, 1);
}

static void _tmc_init(void) {
	uint8_t i;
	for (i = 0; i < USBH_TMC_MAX_INSTANCES; i++) {
		_tmc_object_init(&USBHTMCD[i], i);
	}
}

#endif
