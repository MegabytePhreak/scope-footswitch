


#include <stdio.h>
#include <string.h>

#include "ch.h"
#include "hal.h"

#include "chprintf.h"

#include "usbcfg.h"
#include "scpi/scpi.h"



#define SCPI_INPUT_BUFFER_LENGTH 256
#define SCPI_ERROR_QUEUE_SIZE 17
#define SCPI_IDN1 "PAUL ROUKEMA"
#define SCPI_IDN2 "TMCEMU"
#define SCPI_IDN3 NULL
#define SCPI_IDN4 "01-02"

const scpi_command_t scpi_commands[] = {
    /* IEEE Mandated Commands (SCPI std V1999.0 4.1.1) */
    { .pattern = "*CLS", .callback = SCPI_CoreCls,},
    { .pattern = "*ESE", .callback = SCPI_CoreEse,},
    { .pattern = "*ESE?", .callback = SCPI_CoreEseQ,},
    { .pattern = "*ESR?", .callback = SCPI_CoreEsrQ,},
    { .pattern = "*IDN?", .callback = SCPI_CoreIdnQ,},
    { .pattern = "*OPC", .callback = SCPI_CoreOpc,},
    { .pattern = "*OPC?", .callback = SCPI_CoreOpcQ,},
    { .pattern = "*RST", .callback = SCPI_CoreRst,},
    { .pattern = "*SRE", .callback = SCPI_CoreSre,},
    { .pattern = "*SRE?", .callback = SCPI_CoreSreQ,},
    { .pattern = "*STB?", .callback = SCPI_CoreStbQ,},
    //{ .pattern = "*TST?", .callback = My_CoreTstQ,},
    { .pattern = "*WAI", .callback = SCPI_CoreWai,},

    /* Required SCPI commands (SCPI std V1999.0 4.2.1) */
    {.pattern = "SYSTem:ERRor[:NEXT]?", .callback = SCPI_SystemErrorNextQ,},
    {.pattern = "SYSTem:ERRor:COUNt?", .callback = SCPI_SystemErrorCountQ,},
    {.pattern = "SYSTem:VERSion?", .callback = SCPI_SystemVersionQ,},

    /* {.pattern = "STATus:OPERation?", .callback = scpi_stub_callback,}, */
    /* {.pattern = "STATus:OPERation:EVENt?", .callback = scpi_stub_callback,}, */
    /* {.pattern = "STATus:OPERation:CONDition?", .callback = scpi_stub_callback,}, */
    /* {.pattern = "STATus:OPERation:ENABle", .callback = scpi_stub_callback,}, */
    /* {.pattern = "STATus:OPERation:ENABle?", .callback = scpi_stub_callback,}, */

    {.pattern = "STATus:QUEStionable[:EVENt]?", .callback = SCPI_StatusQuestionableEventQ,},
    /* {.pattern = "STATus:QUEStionable:CONDition?", .callback = scpi_stub_callback,}, */
    {.pattern = "STATus:QUEStionable:ENABle", .callback = SCPI_StatusQuestionableEnable,},
    {.pattern = "STATus:QUEStionable:ENABle?", .callback = SCPI_StatusQuestionableEnableQ,},

    {.pattern = "STATus:PRESet", .callback = SCPI_StatusPreset,},

    /* Scope Emulation: DSO9404a */
    {.pattern = "SINGle", .callback = SCPI_StubQ,},
    {.pattern = "RUN", .callback = SCPI_StubQ,},
    {.pattern = "STOP", .callback = SCPI_StubQ,},
    {.pattern = "RSTate?", .callback = SCPI_StubQ,},


    /* Scope Emulation: DPO3034 */
    {.pattern = "ACQuire:STATE", .callback = SCPI_StubQ,},
    {.pattern = "ACQuire:STATE?", .callback = SCPI_StubQ,},

    SCPI_CMD_LIST_END
};

size_t SCPI_Write(scpi_t * context, const char * data, size_t len) {
    (void) context;
    (void)streamWrite(&SD1, (uint8_t *)data, len);
    return obqWriteTimeout(&(TMC1.obqueue), (uint8_t *)data, len, TIME_INFINITE);
}

scpi_result_t SCPI_Flush(scpi_t * context) {
    (void) context;

    obqFlush(&(TMC1.obqueue));
    return SCPI_RES_OK;
}

int SCPI_Error(scpi_t * context, int_fast16_t err) {
    (void) context;

    chprintf((BaseSequentialStream *)&SD1, "**ERROR: %d, \"%s\"\r\n", (int16_t) err, SCPI_ErrorTranslate(err));
    return 0;
}

scpi_result_t SCPI_Control(scpi_t * context, scpi_ctrl_name_t ctrl, scpi_reg_val_t val) {
    (void) context;

    if (SCPI_CTRL_SRQ == ctrl) {
        chprintf((BaseSequentialStream *)&SD1, "**SRQ: 0x%X (%d)\r\n", val, val);
    } else {
        chprintf((BaseSequentialStream *)&SD1, "**CTRL %02x: 0x%X (%d)\r\n", ctrl, val, val);
    }
    return SCPI_RES_OK;
}

scpi_result_t SCPI_Reset(scpi_t * context) {
    (void) context;

    chprintf((BaseSequentialStream *)&SD1, "**Reset\r\n");
    return SCPI_RES_OK;
}







scpi_interface_t scpi_interface = {
    .error = SCPI_Error,
    .write = SCPI_Write,
    .control = SCPI_Control,
    .flush = SCPI_Flush,
    .reset = SCPI_Reset,
};

char scpi_input_buffer[SCPI_INPUT_BUFFER_LENGTH];
scpi_error_t scpi_error_queue_data[SCPI_ERROR_QUEUE_SIZE];

scpi_t scpi_context;




/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/


THD_FUNCTION(scpiThread, arg) {

  (void)arg;
  chRegSetThreadName("scpi");
  chprintf((BaseSequentialStream *)&SD1, "\r\nSCPI Parser Started\r\n");


  SCPI_Init(&scpi_context,
          scpi_commands,
          &scpi_interface,
          scpi_units_def,
          SCPI_IDN1, SCPI_IDN2, SCPI_IDN3, SCPI_IDN4,
          scpi_input_buffer, SCPI_INPUT_BUFFER_LENGTH,
          scpi_error_queue_data, SCPI_ERROR_QUEUE_SIZE);


  while (ibqGetFullBufferTimeout(&(TMC1.ibqueue), TIME_INFINITE) != Q_RESET) {
    uint8_t * buf = TMC1.ibqueue.ptr;
    size_t len = TMC1.ibqueue.top - TMC1.ibqueue.ptr;


  	chprintf((BaseSequentialStream *)&SD1, "Got USBTMC Command: \"");
    streamWrite(&SD1, buf, len);
  	chprintf((BaseSequentialStream *)&SD1, "\"\r\n");

    scpi_bool_t result = SCPI_Parse(&scpi_context, (char *)buf, len);

  	chprintf((BaseSequentialStream *)&SD1, "SCPI Parser Result: %d\r\n", result);
    ibqReleaseEmptyBuffer(&(TMC1.ibqueue));
  }
  chprintf((BaseSequentialStream *)&SD1, "\r\nSCPI Parser Stopped\r\n");
}

