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

/**
 * @file    hal_serial_usb.c
 * @brief   Serial over USB Driver code.
 *
 * @addtogroup SERIAL_USB
 * @{
 */

#include "hal.h"
#include "usb_tmc.h"
#include <memory.h>

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/
#define MSGID_DEV_DEP_MSG_OUT 1
#define MSGID_DEV_DEP_MSG_IN  2
#define MSGID_REQUEST_DEV_DEP_MSG_IN 2
#define MSGID_VENDOR_SPECIFIC_OUT 126
#define MSGID_VENDOR_SPECIFIC_IN  127
#define MSGID_REQUEST_VENDOR_SPECIFIC_IN 127

#define TMC_INITIATE_ABORT_BULK_OUT 1
#define TMC_CHECK_ABORT_BULK_OUT_STATUS 2
#define TMC_INITIATE_ABORT_BULK_IN  3
#define TMC_CHECK_ABORT_BULK_IN_STATUS 4
#define TMC_INITIATE_CLEAR 5
#define TMC_CHECK_CLEAR_STATUS 6
#define TMC_GET_CAPABILITIES 7
#define TMC_INDICATOR_PULSE 64

#define TMC_STATUS_SUCCESS 1 
#define TMC_STATUS_PENDING 2 
#define TMC_STATUS_FAILED 0x80 
#define TMC_STATUS_TRANSFER_NOT_IN_PROGRESS 0x81
#define TMC_STATUS_SPLIT_NOT_IN_PROGRESS 0x82 
#define TMC_STATUS_SPLIT_IN_PROGRESS 0x83




/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static bool tmc_start_receive(USBTMCDriver *tmcp) {
  uint8_t *buf;

  /* If the USB driver is not in the appropriate state then transactions
     must not be started.*/
  if ((usbGetDriverStateI(tmcp->config->usbp) != USB_ACTIVE) ||
      (tmcp->state != TMC_READY)) {
    return true;
  }

  /* Checking if there is already a transaction ongoing on the endpoint.*/
  if (usbGetReceiveStatusI(tmcp->config->usbp, tmcp->config->bulk_in)) {
    return true;
  }

  /* Checking if there is a buffer ready for incoming data.*/
  buf = ibqGetEmptyBufferI(&tmcp->ibqueue);
  if (buf == NULL) {
    return true;
  }

  /* Buffer found, starting a new transaction.*/
  usbStartReceiveI(tmcp->config->usbp, tmcp->config->bulk_out,
                   buf, USB_TMC_BUFFERS_SIZE);

  return false;
}

/*
 * Interface implementation.
 */

static size_t _write(void *ip, const uint8_t *bp, size_t n) {

  return obqWriteTimeout(&((USBTMCDriver *)ip)->obqueue, bp,
                         n, TIME_INFINITE);
}

static size_t _read(void *ip, uint8_t *bp, size_t n) {

  return ibqReadTimeout(&((USBTMCDriver *)ip)->ibqueue, bp,
                        n, TIME_INFINITE);
}

static msg_t _put(void *ip, uint8_t b) {

  return obqPutTimeout(&((USBTMCDriver *)ip)->obqueue, b, TIME_INFINITE);
}

static msg_t _get(void *ip) {

  return ibqGetTimeout(&((USBTMCDriver *)ip)->ibqueue, TIME_INFINITE);
}

static msg_t _putt(void *ip, uint8_t b, sysinterval_t timeout) {

  return obqPutTimeout(&((USBTMCDriver *)ip)->obqueue, b, timeout);
}

static msg_t _gett(void *ip, sysinterval_t timeout) {

  return ibqGetTimeout(&((USBTMCDriver *)ip)->ibqueue, timeout);
}

static size_t _writet(void *ip, const uint8_t *bp, size_t n,
                      sysinterval_t timeout) {

  return obqWriteTimeout(&((USBTMCDriver *)ip)->obqueue, bp, n, timeout);
}

static size_t _readt(void *ip, uint8_t *bp, size_t n,
                     sysinterval_t timeout) {

  return ibqReadTimeout(&((USBTMCDriver *)ip)->ibqueue, bp, n, timeout);
}

static msg_t _ctl(void *ip, unsigned int operation, void *arg) {
  USBTMCDriver *tmcp = (USBTMCDriver *)ip;

  osalDbgCheck(tmcp != NULL);

  switch (operation) {
  case CHN_CTL_NOP:
    osalDbgCheck(arg == NULL);
    break;
  case CHN_CTL_INVALID:
    osalDbgAssert(false, "invalid CTL operation");
    break;
  default:
#if defined(TMC_IMPLEMENTS_CTL)
    /* The TMC driver does not have a LLD but the application can use this
       hook to implement extra controls by supplying this function.*/ 
    extern msg_t tmc_lld_control(USBTMCDriver *tmcp,
                                 unsigned int operation,
                                 void *arg);
    return tmc_lld_control(tmcp, operation, arg);
#else
    break;
#endif
  }
  return MSG_OK;
}

static const struct USBTMCDriverVMT vmt = {
  (size_t)0,
  _write, _read, _put, _get,
  _putt, _gett, _writet, _readt,
  _ctl
};

/**
 * @brief   Notification of empty buffer released into the input buffers queue.
 *
 * @param[in] bqp       the buffers queue pointer.
 */
static void ibnotify(io_buffers_queue_t *bqp) {
  USBTMCDriver *tmcp = bqGetLinkX(bqp);
  (void) tmc_start_receive(tmcp);
}

/**
 * @brief   Notification of filled buffer inserted into the output buffers queue.
 *
 * @param[in] bqp       the buffers queue pointer.
 */
static void obnotify(io_buffers_queue_t *bqp) {
  //size_t n;
  USBTMCDriver *tmcp = bqGetLinkX(bqp);

  /* If the USB driver is not in the appropriate state then transactions
     must not be started.*/
  if ((usbGetDriverStateI(tmcp->config->usbp) != USB_ACTIVE) ||
      (tmcp->state != TMC_READY)) {
    return;
  }

  /* Checking if there is already a transaction ongoing on the endpoint.*/
  //if (!usbGetTransmitStatusI(tmcp->config->usbp, tmcp->config->bulk_in)) {
  //  /* Trying to get a full buffer.*/
  //  uint8_t *buf = obqGetFullBufferI(&tmcp->obqueue, &n);
  //  if (buf != NULL) {
  //    /* Buffer found, starting a new transaction.*/
  //    usbStartTransmitI(tmcp->config->usbp, tmcp->config->bulk_in, buf, n);
  //  }
  //}
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Serial Driver initialization.
 * @note    This function is implicitly invoked by @p halInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void tmcInit(void) {
}

/**
 * @brief   Initializes a generic full duplex driver object.
 * @details The HW dependent part of the initialization has to be performed
 *          outside, usually in the hardware initialization code.
 *
 * @param[out] tmcp     pointer to a @p USBTMCDriver structure
 *
 * @init
 */
void tmcObjectInit(USBTMCDriver *tmcp) {

  tmcp->vmt = &vmt;
  osalEventObjectInit(&tmcp->event);
  tmcp->state = TMC_STOP;
  ibqObjectInit(&tmcp->ibqueue, true, tmcp->ib,
                SERIAL_USB_BUFFERS_SIZE, SERIAL_USB_BUFFERS_NUMBER,
                ibnotify, tmcp);
  obqObjectInit(&tmcp->obqueue, true, tmcp->ob,
                SERIAL_USB_BUFFERS_SIZE, SERIAL_USB_BUFFERS_NUMBER,
                obnotify, tmcp);
}

/**
 * @brief   Configures and starts the driver.
 *
 * @param[in] tmcp      pointer to a @p USBTMCDriver object
 * @param[in] config    the serial over USB driver configuration
 *
 * @api
 */
void tmcStart(USBTMCDriver *tmcp, const USBTMCConfig *config) {
  USBDriver *usbp = config->usbp;

  osalDbgCheck(tmcp != NULL);

  osalSysLock();
  osalDbgAssert((tmcp->state == TMC_STOP) || (tmcp->state == TMC_READY),
                "invalid state");
  usbp->in_params[config->bulk_in - 1U]   = tmcp;
  usbp->out_params[config->bulk_out - 1U] = tmcp;
  if (config->int_in > 0U) {
    usbp->in_params[config->int_in - 1U]  = tmcp;
  }
  tmcp->config = config;
  tmcp->state = TMC_READY;
  osalSysUnlock();
}

/**
 * @brief   Stops the driver.
 * @details Any thread waiting on the driver's queues will be awakened with
 *          the message @p MSG_RESET.
 *
 * @param[in] tmcp      pointer to a @p USBTMCDriver object
 *
 * @api
 */
void tmcStop(USBTMCDriver *tmcp) {
  USBDriver *usbp = tmcp->config->usbp;

  osalDbgCheck(tmcp != NULL);

  osalSysLock();

  osalDbgAssert((tmcp->state == TMC_STOP) || (tmcp->state == TMC_READY),
                "invalid state");

  /* Driver in stopped state.*/
  usbp->in_params[tmcp->config->bulk_in - 1U]   = NULL;
  usbp->out_params[tmcp->config->bulk_out - 1U] = NULL;
  if (tmcp->config->int_in > 0U) {
    usbp->in_params[tmcp->config->int_in - 1U]  = NULL;
  }
  tmcp->config = NULL;
  tmcp->state  = TMC_STOP;

  /* Enforces a disconnection.*/
  chnAddFlagsI(tmcp, CHN_DISCONNECTED);
  ibqResetI(&tmcp->ibqueue);
  obqResetI(&tmcp->obqueue);
  osalOsRescheduleS();

  osalSysUnlock();
}

/**
 * @brief   USB device suspend handler.
 * @details Generates a @p CHN_DISCONNECT event and puts queues in
 *          non-blocking mode, this way the application cannot get stuck
 *          in the middle of an I/O operations.
 * @note    If this function is not called from an ISR then an explicit call
 *          to @p osalOsRescheduleS() in necessary afterward.
 *
 * @param[in] tmcp      pointer to a @p USBTMCDriver object
 *
 * @iclass
 */
void tmcSuspendHookI(USBTMCDriver *tmcp) {

  chnAddFlagsI(tmcp, CHN_DISCONNECTED);
  bqSuspendI(&tmcp->ibqueue);
  bqSuspendI(&tmcp->obqueue);
}

/**
 * @brief   USB device wakeup handler.
 * @details Generates a @p CHN_CONNECT event and resumes normal queues
 *          operations.
 *
 * @note    If this function is not called from an ISR then an explicit call
 *          to @p osalOsRescheduleS() in necessary afterward.
 *
 * @param[in] tmcp      pointer to a @p USBTMCDriver object
 *
 * @iclass
 */
void tmcWakeupHookI(USBTMCDriver *tmcp) {

  chnAddFlagsI(tmcp, CHN_CONNECTED);
  bqResumeX(&tmcp->ibqueue);
  bqResumeX(&tmcp->obqueue);
}

/**
 * @brief   USB device configured handler.
 *
 * @param[in] tmcp      pointer to a @p USBTMCDriver object
 *
 * @iclass
 */
void tmcConfigureHookI(USBTMCDriver *tmcp) {

  ibqResetI(&tmcp->ibqueue);
  bqResumeX(&tmcp->ibqueue);
  obqResetI(&tmcp->obqueue);
  bqResumeX(&tmcp->obqueue);
  chnAddFlagsI(tmcp, CHN_CONNECTED);
  (void) tmc_start_receive(tmcp);
}


static uint8_t resp[12] = {};

/**
 * @param[in] usbp      pointer to the @p USBDriver object
 * @return              The hook status.
 * @retval true         Message handled internally.
 * @retval false        Message not handled.
 */
bool tmcRequestsHook(USBDriver *usbp) {

  USBTMCDriver *tmcp = usbp->in_params[0];
  if(tmcp == NULL)
  { 
    osalDbgAssert(tmcp != NULL, "Control request tmcp NULL");

    return false;
  }  

  if ((usbp->setup[0] & USB_RTYPE_TYPE_MASK) == USB_RTYPE_TYPE_CLASS) {
    switch (usbp->setup[1]) {
    case TMC_INITIATE_ABORT_BULK_OUT:
    case TMC_INITIATE_ABORT_BULK_IN:
      resp[0] = TMC_STATUS_TRANSFER_NOT_IN_PROGRESS;
      resp[1] = 0;
      usbSetupTransfer(usbp, resp, 2, NULL);
      return true;
    case TMC_CHECK_ABORT_BULK_OUT_STATUS:
      resp[0] = TMC_STATUS_SUCCESS;
      resp[1] = 0;
      resp[2] = 0;
      resp[3] = 0;
      resp[4] = 0;
      usbSetupTransfer(usbp, resp, 5, NULL);
      return true;
    case TMC_CHECK_ABORT_BULK_IN_STATUS:
      resp[0] = TMC_STATUS_SUCCESS;
      resp[1] = 0;
      resp[2] = 0;
      resp[3] = 0;
      resp[4] = 0;
      resp[5] = 0;
      resp[1] = 0;
      resp[7] = 0;
      usbSetupTransfer(usbp, resp, 8, NULL);
      return true;
    case TMC_INITIATE_CLEAR:
      resp[0] = TMC_STATUS_SUCCESS;
      usbSetupTransfer(usbp, resp, 1, NULL);
      return true;
    case TMC_CHECK_CLEAR_STATUS:
      resp[0] = TMC_STATUS_SUCCESS;
      resp[1] = 0;
      usbSetupTransfer(usbp, resp, 2, NULL);
      return true;
    case TMC_GET_CAPABILITIES:
      memset(resp, 0, sizeof resp);
      resp[0] = TMC_STATUS_SUCCESS;
      resp[2] = 0x00; /* bcdUSBTMC */
      resp[3] = 0x10; /* bcdUSBTMC */
      resp[4] = 0x04; /* bmInterfaceCap */
      resp[5] = 0x00; /* bmDeviceCap */
      usbSetupTransfer(usbp, resp, 12, NULL);
      return true;
    case TMC_INDICATOR_PULSE:
      resp[0] = TMC_STATUS_SUCCESS;
      if(tmcp->config->indicator_cb)
      {
          tmcp->config->indicator_cb(tmcp);
      }
      usbSetupTransfer(usbp, resp, 1, NULL);
      return true;
    default:
      return false;
    }
  }
  return false;
}

/**
 * @brief   SOF handler.
 * @details The SOF interrupt is used for automatic flushing of incomplete
 *          buffers pending in the output queue.
 *
 * @param[in] tmcp      pointer to a @p USBTMCDriver object
 *
 * @iclass
 */
void tmcSOFHookI(USBTMCDriver *tmcp) {

  /* If the USB driver is not in the appropriate state then transactions
     must not be started.*/
  if ((usbGetDriverStateI(tmcp->config->usbp) != USB_ACTIVE) ||
      (tmcp->state != TMC_READY)) {
    return;
  }

  /* If there is already a transaction ongoing then another one cannot be
     started.*/
  if (usbGetTransmitStatusI(tmcp->config->usbp, tmcp->config->bulk_in)) {
    return;
  }
  return;
}

/**
 * @brief   Default data transmitted callback.
 * @details The application must use this function as callback for the IN
 *          data endpoint.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        IN endpoint number
 */
void tmcDataTransmitted(USBDriver *usbp, usbep_t ep) {
  USBTMCDriver *tmcp = usbp->in_params[ep - 1U];

  if (tmcp == NULL) {
    return;
  }

  osalSysLockFromISR();

  /* Signaling that space is available in the output queue.*/
  chnAddFlagsI(tmcp, CHN_OUTPUT_EMPTY);

  /* Freeing the buffer just transmitted, if it was not a zero size packet.*/
  if (usbp->epc[ep]->in_state->txsize > 0U) {
    obqReleaseEmptyBufferI(&tmcp->obqueue);
  }

  osalSysUnlockFromISR();
}


static int handleMsgOut(USBTMCDriver* tmcp, uint8_t* buf,size_t size)
{
  //uint8_t * buf = ibqGetEmptyBufferI(&(tmcp->ibqueue));

  uint8_t bTag = buf[1];
  uint8_t bTagInverse = buf[2];
  uint32_t TransferSize = buf[4] | buf[5] << 8 | buf[6] << 16 | buf[7] << 24;
  uint8_t bmTransferAttributes = buf[8];

  if(bTag != (uint8_t)(~bTagInverse))
  {
    return -1;
  }
  if(TransferSize > (SERIAL_USB_BUFFERS_SIZE-12-1) || bmTransferAttributes != 1 
    || size != ((TransferSize+3)/4)*4 + 12) 
  {
    return -1;
  }
  memmove(buf, buf+12, TransferSize);
  /* Signaling that data is available in the input queue.*/
  chnAddFlagsI(tmcp, CHN_INPUT_AVAILABLE);

  /* Posting the filled buffer in the queue.*/
  ibqPostFullBufferI(&tmcp->ibqueue, TransferSize);

  return 0;
}

static int handleMsgIn(USBTMCDriver* tmcp, uint8_t* buf, size_t size)
{
  //uint8_t * buf = ibqGetEmptyBufferI(&(tmcp->ibqueue));

  uint8_t bTag = buf[1];
  uint8_t bTagInverse = buf[2];
  uint32_t TransferSize = buf[4] | buf[5] << 8 | buf[6] << 16 | buf[7] << 24;
  uint8_t bmTransferAttributes = buf[8];

  if(bTag != (uint8_t)(~bTagInverse) || size != 12)
  {
    return -1;
  }
  if(bmTransferAttributes != 0)
  {
    return -1;
  }
  /* Checking if there is a buffer ready for transmission.*/
  size_t txsize = 0;
  uint8_t* txbuf = obqGetFullBufferI(&tmcp->obqueue, &txsize);
  if(!txbuf || txsize > (SERIAL_USB_BUFFERS_SIZE-12))
  {
    return -1;
  } 

  memmove(txbuf+12, txbuf, txsize);
  txbuf[0]  = MSGID_DEV_DEP_MSG_IN;
  txbuf[1]  = bTag;
  txbuf[2]  = ~bTag;
  txbuf[3]  = 0;
  txbuf[4]  = txsize & 0xFF;
  txbuf[5]  = (txsize >> 8) & 0xFF;
  txbuf[6]  = (txsize >> 16) & 0xFF;
  txbuf[7]  = (txsize >> 24) & 0xFF;
  txbuf[8]  = 1;
  txbuf[9]  = 0;
  txbuf[10]  = 0;
  txbuf[11]  = 0;

  usbStartTransmitI(tmcp->config->usbp, tmcp->config->bulk_in, txbuf, (txsize+3)/4*4+12);
  return 0;
}

/**
 * @brief   Default data received callback.
 * @details The application must use this function as callback for the OUT
 *          data endpoint.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        OUT endpoint number
 */
void tmcDataReceived(USBDriver *usbp, usbep_t ep) {
  size_t size;
  int    status = -1;
  USBTMCDriver *tmcp = usbp->out_params[ep - 1U];

  if (tmcp == NULL) {
    return;
  }

  osalSysLockFromISR();

  /* Checking for zero-size transactions.*/
  size = usbGetReceiveTransactionSizeX(tmcp->config->usbp,
                                       tmcp->config->bulk_out);
  if (size >= (size_t)12) {
    uint8_t * buf = ibqGetEmptyBufferI(&tmcp->ibqueue);
    switch (buf[0])
    {
      case MSGID_DEV_DEP_MSG_OUT:
        status = handleMsgOut(tmcp, buf, size);
        break;
      case MSGID_DEV_DEP_MSG_IN:

        status = handleMsgIn(tmcp, buf, size);
        break;
      default: 
        break;
    }
  }
  if(status == 0)
  {
    /* The endpoint cannot be busy, we are in the context of the callback,
      so a packet is in the buffer for sure. Trying to get a free buffer
      for the next transaction.*/
    (void) tmc_start_receive(tmcp);
  } 
  else {
    usbStallReceiveI(usbp, ep);
  }


  osalSysUnlockFromISR();
}

/**
 * @brief   Default data received callback.
 * @details The application must use this function as callback for the IN
 *          interrupt endpoint.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 */
void tmcInterruptTransmitted(USBDriver *usbp, usbep_t ep) {

  (void)usbp;
  (void)ep;
}

/**
 * @brief   Control operation on a serial USB port.
 *
 * @param[in] usbp       pointer to a @p USBDriver object
 * @param[in] operation control operation code
 * @param[in,out] arg   operation argument
 *
 * @return              The control operation status.
 * @retval MSG_OK       in case of success.
 * @retval MSG_TIMEOUT  in case of operation timeout.
 * @retval MSG_RESET    in case of operation reset.
 *
 * @api
 */
msg_t tmcControl(USBDriver *usbp, unsigned int operation, void *arg) {

  return _ctl((void *)usbp, operation, arg);
}

/** @} */
