

#ifndef USB_TMC_H
#define USB_TMC_H

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/
/**
 * @brief   USBTMC max packet size.
 * @details Configuration parameter, the USB data endpoint maximum packet size.
 * @note    The default is 64 bytes for both the transmission and receive
 *          buffers.
 */
#if !defined(USB_TMC_MAX_PKT_SIZE) || defined(__DOXYGEN__)
#define USB_TMC_MAX_PKT_SIZE 64
#endif

/**
 * @brief   USBTMCbuffers size.
 * @details Configuration parameter, the buffer size must be a multiple of
 *          the USB data endpoint maximum packet size.
 * @note    The default is 256 bytes for both the transmission and receive
 *          buffers.
 */
#if !defined(USB_TMC_BUFFERS_SIZE) || defined(__DOXYGEN__)
#define USB_TMC_BUFFERS_SIZE 256
#endif

/**
 * @brief   Serial over USB number of buffers.
 * @note    The default is 2 buffers.
 */
#if !defined(USB_TMC_BUFFERS_NUMBER) || defined(__DOXYGEN__)
#define USB_TMC_BUFFERS_NUMBER 2
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if HAL_USE_USB == FALSE
#error "USB TMC Driver requires HAL_USE_USB"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief Driver state machine possible states.
 */
typedef enum {
    TMC_UNINIT = 0, /**< Not initialized.                   */
    TMC_STOP   = 1, /**< Stopped.                           */
    TMC_READY  = 2  /**< Ready.                             */
} tmcstate_t;

/**
 * @brief   Structure representing a serial over USB driver.
 */
typedef struct USBTMCDriver USBTMCDriver;

typedef void (*tmccallback_t)(USBTMCDriver *usbp);

/**
 * @brief   Serial over USB Driver configuration structure.
 * @details An instance of this structure must be passed to @p sduStart()
 *          in order to configure and start the driver operations.
 */
typedef struct {
    /**
     * @brief   USB driver to use.
     */
    USBDriver *usbp;
    /**
     * @brief   Bulk IN endpoint used for outgoing data transfer.
     */
    usbep_t bulk_in;
    /**
     * @brief   Bulk OUT endpoint used for incoming data transfer.
     */
    usbep_t bulk_out;
    /**
     * @brief   Interrupt IN endpoint used for notifications.
     * @note    If set to zero then the INT endpoint is assumed to be not
     *          present, USB descriptors must be changed accordingly.
     */
    usbep_t int_in;

    tmccallback_t indicator_cb;
} USBTMCConfig;

/**
 * @brief   @p SerialDriver specific data.
 */
#define _usb_tmc_driver_data                                                   \
    _base_asynchronous_channel_data /* Driver state.*/                         \
        tmcstate_t state;                                                      \
    /* Input buffers queue.*/                                                  \
    input_buffers_queue_t ibqueue;                                             \
    /* Output queue.*/                                                         \
    output_buffers_queue_t obqueue;                                            \
    /* Input buffer.*/                                                         \
    uint8_t ib[BQ_BUFFER_SIZE(USB_TMC_BUFFERS_NUMBER, USB_TMC_BUFFERS_SIZE)];  \
    /* Output buffer.*/                                                        \
    uint8_t ob[BQ_BUFFER_SIZE(USB_TMC_BUFFERS_NUMBER, USB_TMC_BUFFERS_SIZE)];  \
    uint8_t next_btag;                                                         \
    size_t  in_size;                                                           \
    /* End of the mandatory fields.*/                                          \
    /* Current configuration data.*/                                           \
    const USBTMCConfig *config;

/**
 * @brief   @p USBTMCDriver specific methods.
 */
#define _usb_tmc_driver_methods _base_asynchronous_channel_methods

/**
 * @extends BaseAsynchronousChannelVMT
 *
 * @brief   @p SerialDriver virtual methods table.
 */
struct USBTMCDriverVMT {
    _usb_tmc_driver_methods
};

/**
 * @extends BaseAsynchronousChannel
 *
 * @brief   Full duplex serial driver class.
 * @details This class extends @p BaseAsynchronousChannel by adding physical
 *          I/O queues.
 */
struct USBTMCDriver {
    /** @brief Virtual Methods Table.*/
    const struct USBTMCDriverVMT *vmt;
    _usb_tmc_driver_data
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
void  tmcInit(void);
void  tmcObjectInit(USBTMCDriver *tmcp);
void  tmcStart(USBTMCDriver *tmcp, const USBTMCConfig *config);
void  tmcStop(USBTMCDriver *tmcp);
void  tmcSuspendHookI(USBTMCDriver *tmcp);
void  tmcWakeupHookI(USBTMCDriver *tmcp);
void  tmcConfigureHookI(USBTMCDriver *tmcp);
bool  tmcRequestsHook(USBDriver *usbp);
void  tmcSOFHookI(USBTMCDriver *tmcp);
void  tmcDataTransmitted(USBDriver *usbp, usbep_t ep);
void  tmcDataReceived(USBDriver *usbp, usbep_t ep);
void  tmcInterruptTransmitted(USBDriver *usbp, usbep_t ep);
msg_t tmcControl(USBDriver *usbp, unsigned int operation, void *arg);
#ifdef __cplusplus
}
#endif

#endif /* HAL_SERIAL_USB_H */

/** @} */
