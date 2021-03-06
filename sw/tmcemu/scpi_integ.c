/*
 * This file is part of the scope-footswitch project.
 *
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



#include <stdio.h>
#include <string.h>

#include "ch.h"
#include "hal.h"

#include "chprintf.h"

#include "usbcfg.h"
#include "scpi/scpi.h"

static scpi_result_t SCPI_Flush(scpi_t *context);
static size_t        SCPI_Write(scpi_t *context, const char *data, size_t len);

#define SCPI_INPUT_BUFFER_LENGTH 256
#define SCPI_ERROR_QUEUE_SIZE 17
#define SCPI_IDN1 "PAUL ROUKEMA"
#define SCPI_IDN2 "TMCEMU"
#define SCPI_IDN3 NULL
#define SCPI_IDN4 "01-02"

enum scope_state { STATE_STOPPED, STATE_RUNNING, STATE_SINGLE };

enum stop_after { STOPAFTER_RUNSTOP, STOPAFTER_SEQUENCE };

static enum scope_state scope_state       = STATE_STOPPED;
static enum stop_after  dpo3034_stopafter = STOPAFTER_RUNSTOP;
static virtual_timer_t  single_vt;

static void dso9404a_single_timeout(void *);

static void dso9404a_set_state(enum scope_state state) {
    palClearLine(LINE_LED_ORANGE);
    palClearLine(LINE_LED_RED);
    palClearLine(LINE_LED_GREEN);

    scope_state = state;
    switch (state) {
        case STATE_RUNNING:
            palSetLine(LINE_LED_GREEN);
            break;
        case STATE_SINGLE:
            palSetLine(LINE_LED_ORANGE);
            chVTSet(&single_vt, TIME_MS2I(5000), dso9404a_single_timeout, NULL);
            break;
        default:
            palSetLine(LINE_LED_RED);
    }
}

static void dso9404a_single_timeout(void *arg) {
    (void)arg;
    dso9404a_set_state(STATE_STOPPED);
}

static scpi_result_t dso9404a_single(scpi_t *context) {
    (void)context;
    dso9404a_set_state(STATE_SINGLE);

    return SCPI_RES_OK;
}

static scpi_result_t dso9404a_run(scpi_t *context) {
    (void)context;
    dso9404a_set_state(STATE_RUNNING);

    return SCPI_RES_OK;
}

static scpi_result_t dso9404a_stop(scpi_t *context) {
    (void)context;
    dso9404a_set_state(STATE_STOPPED);

    return SCPI_RES_OK;
}

static scpi_result_t dso9404a_rstateQ(scpi_t *context) {

    switch (scope_state) {
        case STATE_RUNNING:
            SCPI_ResultMnemonic(context, "RUN");
            break;
        case STATE_SINGLE:
            SCPI_ResultMnemonic(context, "SING");
            break;
        default:
            SCPI_ResultMnemonic(context, "STOP");
    }
    return SCPI_RES_OK;
}

static scpi_choice_def_t dpo3034_state_options[] = {
    {"OFF", 0}, {"STOP", 0}, {"ON", 1}, {"RUN", 1}, SCPI_CHOICE_LIST_END};

static scpi_result_t dpo3034_acquire_state(scpi_t *context) {
    scpi_parameter_t param;
    int32_t          value = 0;
    if (SCPI_Parameter(context, &param, TRUE)) {
        if (param.type == SCPI_TOKEN_PROGRAM_MNEMONIC) {
            if (!SCPI_ParamToChoice(context, &param, dpo3034_state_options,
                                    &value))
                return SCPI_RES_ERR;
        } else if (SCPI_ParamIsNumber(&param, FALSE)) {
            if (!SCPI_ParamToInt32(context, &param, &value))
                return SCPI_RES_ERR;
        } else {
            return SCPI_RES_ERR;
        }
        if (value == 0) {
            dso9404a_set_state(STATE_STOPPED);
        } else {
            if (dpo3034_stopafter == STOPAFTER_RUNSTOP) {
                dso9404a_set_state(STATE_RUNNING);
            } else {
                dso9404a_set_state(STATE_SINGLE);
            }
        }
        return SCPI_RES_OK;
    }
    return SCPI_RES_ERR;
}

static scpi_result_t dpo3034_acquire_stateQ(scpi_t *context) {
    SCPI_ResultInt32(context, scope_state == STATE_STOPPED ? 0 : 1);

    return SCPI_RES_OK;
}

static scpi_choice_def_t dpo3034_stopafter_options[] = {
    {"RUNSTop", STOPAFTER_RUNSTOP},
    {"SEQuence", STOPAFTER_SEQUENCE},
    SCPI_CHOICE_LIST_END};

static scpi_result_t dpo3034_acquire_stopafter(scpi_t *context) {
    int32_t value = 0;
    if (!SCPI_ParamChoice(context, dpo3034_stopafter_options, &value, TRUE)) {
        return SCPI_RES_ERR;
    }
    dpo3034_stopafter = value;

    return SCPI_RES_OK;
}

static scpi_result_t dpo3034_acquire_stopafterQ(scpi_t *context) {
    SCPI_ResultMnemonic(context, dpo3034_stopafter == STOPAFTER_RUNSTOP
                                     ? "RUNSTOP"
                                     : "SEQUENCE");

    return SCPI_RES_OK;
}

// Hacky emulation of dso3034 behaviour. Response returned in multiple
// transactions Also the dso3034 seems to separate responses with semicolons and
// not commas
static scpi_result_t dpo3034_acquire_acquireQ(scpi_t *context) {
    SCPI_ResultMnemonic(context, dpo3034_stopafter == STOPAFTER_RUNSTOP
                                     ? "RUNSTOP"
                                     : "SEQUENCE");
    SCPI_Write(context, ";", 1);
    SCPI_Flush(context);
    context->output_count = 0;
    SCPI_ResultInt32(context, scope_state == STATE_STOPPED ? 0 : 1);
    SCPI_Write(context, ";", 1);
    context->output_count = 0;
    SCPI_ResultMnemonic(context, "SAMPLE");
    SCPI_Write(context, ";", 1);
    SCPI_Flush(context);
    context->output_count = 0;
    SCPI_ResultMnemonic(context, "INFINITE");
    SCPI_Write(context, ";", 1);
    context->output_count = 0;
    SCPI_ResultInt32(context, 16);
    SCPI_Write(context, ";", 1);
    SCPI_Flush(context);
    context->output_count = 0;
    SCPI_ResultMnemonic(context, "2.5000E+9");

    return SCPI_RES_OK;
}

const scpi_command_t scpi_commands[] = {
    /* IEEE Mandated Commands (SCPI std V1999.0 4.1.1) */
    {
        .pattern  = "*CLS",
        .callback = SCPI_CoreCls,
    },
    {
        .pattern  = "*ESE",
        .callback = SCPI_CoreEse,
    },
    {
        .pattern  = "*ESE?",
        .callback = SCPI_CoreEseQ,
    },
    {
        .pattern  = "*ESR?",
        .callback = SCPI_CoreEsrQ,
    },
    {
        .pattern  = "*IDN?",
        .callback = SCPI_CoreIdnQ,
    },
    {
        .pattern  = "*OPC",
        .callback = SCPI_CoreOpc,
    },
    {
        .pattern  = "*OPC?",
        .callback = SCPI_CoreOpcQ,
    },
    {
        .pattern  = "*RST",
        .callback = SCPI_CoreRst,
    },
    {
        .pattern  = "*SRE",
        .callback = SCPI_CoreSre,
    },
    {
        .pattern  = "*SRE?",
        .callback = SCPI_CoreSreQ,
    },
    {
        .pattern  = "*STB?",
        .callback = SCPI_CoreStbQ,
    },
    //{ .pattern = "*TST?", .callback = My_CoreTstQ,},
    {
        .pattern  = "*WAI",
        .callback = SCPI_CoreWai,
    },

    /* Required SCPI commands (SCPI std V1999.0 4.2.1) */
    {
        .pattern  = "SYSTem:ERRor[:NEXT]?",
        .callback = SCPI_SystemErrorNextQ,
    },
    {
        .pattern  = "SYSTem:ERRor:COUNt?",
        .callback = SCPI_SystemErrorCountQ,
    },
    {
        .pattern  = "SYSTem:VERSion?",
        .callback = SCPI_SystemVersionQ,
    },

    /* {.pattern = "STATus:OPERation?", .callback = scpi_stub_callback,}, */
    /* {.pattern = "STATus:OPERation:EVENt?", .callback = scpi_stub_callback,},
     */
    /* {.pattern = "STATus:OPERation:CONDition?", .callback =
       scpi_stub_callback,}, */
    /* {.pattern = "STATus:OPERation:ENABle", .callback = scpi_stub_callback,},
     */
    /* {.pattern = "STATus:OPERation:ENABle?", .callback = scpi_stub_callback,},
     */

    {
        .pattern  = "STATus:QUEStionable[:EVENt]?",
        .callback = SCPI_StatusQuestionableEventQ,
    },
    /* {.pattern = "STATus:QUEStionable:CONDition?", .callback =
       scpi_stub_callback,}, */
    {
        .pattern  = "STATus:QUEStionable:ENABle",
        .callback = SCPI_StatusQuestionableEnable,
    },
    {
        .pattern  = "STATus:QUEStionable:ENABle?",
        .callback = SCPI_StatusQuestionableEnableQ,
    },

    {
        .pattern  = "STATus:PRESet",
        .callback = SCPI_StatusPreset,
    },

    /* Scope Emulation: DSO9404a */
    {
        .pattern  = "SINGle",
        .callback = dso9404a_single,
    },
    {
        .pattern  = "RUN",
        .callback = dso9404a_run,
    },
    {
        .pattern  = "STOP",
        .callback = dso9404a_stop,
    },
    {
        .pattern  = "RSTate?",
        .callback = dso9404a_rstateQ,
    },

    /* Scope Emulation: DPO3034 */
    {
        .pattern  = "ACQuire:STATE",
        .callback = dpo3034_acquire_state,
    },
    {
        .pattern  = "ACQuire:STATE?",
        .callback = dpo3034_acquire_stateQ,
    },
    {
        .pattern  = "ACQuire:STOPAfter",
        .callback = dpo3034_acquire_stopafter,
    },
    {
        .pattern  = "ACQuire:STOPAfter?",
        .callback = dpo3034_acquire_stopafterQ,
    },
    {
        .pattern  = "ACQuire?",
        .callback = dpo3034_acquire_acquireQ,
    },
    SCPI_CMD_LIST_END};

static size_t SCPI_Write(scpi_t *context, const char *data, size_t len) {
    (void)context;
    (void)streamWrite(&SD1, (uint8_t *)data, len);
    return obqWriteTimeout(&(TMC1.obqueue), (uint8_t *)data, len,
                           TIME_INFINITE);
}

static scpi_result_t SCPI_Flush(scpi_t *context) {
    (void)context;

    obqFlush(&(TMC1.obqueue));
    return SCPI_RES_OK;
}

static int SCPI_Error(scpi_t *context, int_fast16_t err) {
    (void)context;

    chprintf((BaseSequentialStream *)&SD1, "**ERROR: %d, \"%s\"\r\n",
             (int16_t)err, SCPI_ErrorTranslate(err));
    return 0;
}

static scpi_result_t SCPI_Control(scpi_t *context, scpi_ctrl_name_t ctrl,
                                  scpi_reg_val_t val) {
    (void)context;

    if (SCPI_CTRL_SRQ == ctrl) {
        chprintf((BaseSequentialStream *)&SD1, "**SRQ: 0x%X (%d)\r\n", val,
                 val);
    } else {
        chprintf((BaseSequentialStream *)&SD1, "**CTRL %02x: 0x%X (%d)\r\n",
                 ctrl, val, val);
    }
    return SCPI_RES_OK;
}

static scpi_result_t SCPI_Reset(scpi_t *context) {
    (void)context;

    chprintf((BaseSequentialStream *)&SD1, "**Reset\r\n");
    return SCPI_RES_OK;
}

scpi_interface_t scpi_interface = {
    .error   = SCPI_Error,
    .write   = SCPI_Write,
    .control = SCPI_Control,
    .flush   = SCPI_Flush,
    .reset   = SCPI_Reset,
};

char         scpi_input_buffer[SCPI_INPUT_BUFFER_LENGTH];
scpi_error_t scpi_error_queue_data[SCPI_ERROR_QUEUE_SIZE];

scpi_t scpi_context;

/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/

THD_FUNCTION(scpiThread, arg) {

    (void)arg;
    chRegSetThreadName("scpi");
    chprintf((BaseSequentialStream *)&SD1, "\r\nSCPI Parser Started\r\n");

    dso9404a_set_state(STATE_STOPPED);

    char *idn1 = SCPI_IDN1, *idn2 = SCPI_IDN2, *idn3 = SCPI_IDN4,
         *idn4 = SCPI_IDN4;
    switch (emu_device) {
        case EMU_DEVICE_TEK_DPO3034:
            idn1 = "TEKTRONIX";
            idn2 = "dpo3034";
            break;

        case EMU_DEVICE_KEYSIGHT_DSO9404A:
            idn1 = "KEYSIGHT TECHNOLOGIES";
            idn2 = "DSO9404A";
            idn3 = "MY53020105";
            idn4 = "06.20.01002";
            break;

        default:
            break;
    }

    SCPI_Init(&scpi_context, scpi_commands, &scpi_interface, scpi_units_def,
              idn1, idn2, idn3, idn4, scpi_input_buffer,
              SCPI_INPUT_BUFFER_LENGTH, scpi_error_queue_data,
              SCPI_ERROR_QUEUE_SIZE);

    while (ibqGetFullBufferTimeout(&(TMC1.ibqueue), TIME_INFINITE) != Q_RESET) {
        uint8_t *buf = TMC1.ibqueue.ptr;
        size_t   len = TMC1.ibqueue.top - TMC1.ibqueue.ptr;

        chprintf((BaseSequentialStream *)&SD1, "Got USBTMC Command: \"");
        streamWrite(&SD1, buf, len);
        chprintf((BaseSequentialStream *)&SD1, "\"\r\n");

        scpi_bool_t result = SCPI_Parse(&scpi_context, (char *)buf, len);

        chprintf((BaseSequentialStream *)&SD1, "SCPI Parser Result: %d\r\n",
                 result);
        ibqReleaseEmptyBuffer(&(TMC1.ibqueue));
    }
    chprintf((BaseSequentialStream *)&SD1, "\r\nSCPI Parser Stopped\r\n");
}
