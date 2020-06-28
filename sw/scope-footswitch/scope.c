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

#include "scope.h"

#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "usbh_usbtmc.h"

#include <string.h>

#define SCOPE_DEBUG_ENABLE_TRACE 0
#define SCOPE_DEBUG_ENABLE_INFO 1
#define SCOPE_DEBUG_ENABLE_WARNINGS 1
#define SCOPE_DEBUG_ENABLE_ERRORS 1

#if SCOPE_DEBUG_ENABLE_TRACE
#define sdbgf(f, ...) chprintf(CON, f, ##__VA_ARGS__)
#else
#define sdbgf(f, ...)                                                          \
    do {                                                                       \
    } while (0)
#endif

#if SCOPE_DEBUG_ENABLE_INFO
#define sinfof(f, ...) chprintf(CON, f, ##__VA_ARGS__)
#else
#define sinfof(f, ...)                                                         \
    do {                                                                       \
    } while (0)
#endif

#if SCOPE_DEBUG_ENABLE_WARNINGS
#define swarnf(f, ...) chprintf(CON, f, ##__VA_ARGS__)
#else
#define swarnf(f, ...)                                                         \
    do {                                                                       \
    } while (0)
#endif

#if SCOPE_DEBUG_ENABLE_ERRORS
#define serrf(f, ...) chprintf(CON, f, ##__VA_ARGS__)
#else
#define serrf(f, ...)                                                          \
    do {                                                                       \
    } while (0)
#endif

#define CMD_TIMEOUT TIME_MS2I(1000)
const char *strip_chars = " \r\n\t";

/* remove characters from the ends of string*/
static char *strstrip(char *str, const char *remove) {
    /* Trim front */
    while (strchr(remove, *str) && *str) {
        str++;
    }
    char *end = str + strlen(str) - 1;
    while (strchr(remove, *end) && end >= str) {
        *end = 0;
        end--;
    }
    return str;
}

static size_t tokenize(char *resp, char *elems[], size_t num_elems) {
    size_t found = 1;
    elems[0]     = resp;
    for (size_t i = 1; i < num_elems; i++) {
        elems[i] = elems[i - 1];
        while (*elems[i]) {
            if (*elems[i] == ',' || *elems[i] == ';' || *elems[i] == '\n') {
                found++;
                *elems[i] = 0;
            }
            elems[i]++;
            if (found > i) {
                break;
            }
        }
        strstrip(elems[i - 1], strip_chars);
    }
    strstrip(elems[found - 1], strip_chars);
    return found;
}

static int run_cmd(USBHTmcDriver *tmcp, const char *cmd) {
    sdbgf("Running scope command '%s'\r\n", cmd);
    if (!usbhtmcWrite(tmcp, cmd, strlen(cmd), CMD_TIMEOUT)) {
        serrf("Scope command '%s' failed\r\n", cmd);
        return 0;
    }
    return 1;
}

static int run_ask(USBHTmcDriver *tmcp, const char *cmd, char *buf,
                   size_t buf_len) {
    chDbgAssert(tmcp, "tmcp");
    chDbgAssert(cmd, "tmcp");
    chDbgAssert(buf, "buf");
    chDbgAssert(buf_len > 0, "buf_len");
    size_t len;

    sdbgf("Asking '%s'\r\n", cmd);
    if (!(len = usbhtmcAsk(tmcp, cmd, strlen(cmd), buf, buf_len - 1,
                           CMD_TIMEOUT))) {
        serrf("State query failed ask '%s'\r\n", cmd);
        return 0;
    }
    buf[len] = 0;

    return 1;
}

static int tektronix_set_state(USBHTmcDriver *tmcp, scope_state_t state) {
    static const char stopcmd[]      = "ACQuire:STATE STOP";
    static const char runsinglecmd[] = "ACQuire:STOPAfter SEQUENCE; STATE RUN";
    static const char runstopcmd[]   = "ACQuire:STOPAfter RUNSTOP; STATE RUN";
    const char *      cmd            = 0;
    switch (state) {
        case SCOPE_STATE_SINGLE:
            cmd = runsinglecmd;
            break;
        case SCOPE_STATE_RUNNING:
            cmd = runstopcmd;
            break;
        case SCOPE_STATE_STOPPED:
        default:
            cmd = stopcmd;
            break;
    }

    return run_cmd(tmcp, cmd);
}

static int tektronix_get_state(USBHTmcDriver *tmcp, scope_state_t *state) {
    static const char allstatecmd[] = "ACQuire:STOPAfter?; STATE?";
    enum { ELEM_STOPAFTER, ELEM_STATE };

    scope_state_t newstate;
    char          buf[65];

    if (!run_ask(tmcp, allstatecmd, buf, sizeof(buf))) {
        return 0;
    }

    size_t count;
    char * elems[6];
    if ((count = tokenize(buf, elems, 2)) < 2) {
        serrf("State query failed tokenize %u\r\n", count);
        return 0;
    }

    // First check the mode
    if (!strcasecmp(elems[ELEM_STOPAFTER], "RUNSTOP")) {
        newstate = SCOPE_STATE_RUNNING;
    } else if (!strcasecmp(elems[ELEM_STOPAFTER], "SEQUENCE")) {
        newstate = SCOPE_STATE_SINGLE;
    } else {
        serrf("State query failed to parse STOPAfter\r\n");
        return 0;
    }

    // Then check the state
    if (!strcasecmp(elems[ELEM_STATE], "0")) {
        newstate = SCOPE_STATE_STOPPED;
    } else if (!strcasecmp(elems[ELEM_STATE], "1")) {
        /* running*/
    } else {
        serrf("State query failed to parse STATE\r\n");
        return 0;
    }
    *state = newstate;
    return 1;
}

static int keysight_set_state(USBHTmcDriver *tmcp, scope_state_t state) {
    static const char stopcmd[]      = "STOP";
    static const char runsinglecmd[] = "SINGle";
    static const char runstopcmd[]   = "RUN";
    const char *      cmd            = 0;
    switch (state) {
        case SCOPE_STATE_SINGLE:
            cmd = runsinglecmd;
            break;
        case SCOPE_STATE_RUNNING:
            cmd = runstopcmd;
            break;
        case SCOPE_STATE_STOPPED:
        default:
            cmd = stopcmd;
            break;
    }

    return run_cmd(tmcp, cmd);
}

static int keysight_get_state(USBHTmcDriver *tmcp, scope_state_t *state) {
    static const char rstatecmd[] = "RSTate?";
    char              buf[65];

    if (!run_ask(tmcp, rstatecmd, buf, sizeof(buf))) {
        return 0;
    }

    char *resp = strstrip(buf, strip_chars);

    // First check the mode
    if (!strcasecmp(resp, "RUN")) {
        *state = SCOPE_STATE_RUNNING;
    } else if (!strcasecmp(resp, "SING")) {
        *state = SCOPE_STATE_SINGLE;
    } else if (!strcasecmp(resp, "STOP")) {
        *state = SCOPE_STATE_STOPPED;
    } else {
        chprintf(CON, "State query failed to parse RSTate");
        return 0;
    }

    return 1;
}

static const scope_config_t tektronix_cfg = {tektronix_set_state,
                                             tektronix_get_state};

static const scope_config_t keysight_cfg = {keysight_set_state,
                                            keysight_get_state};

static const scope_config_t tmcemu_cfg = {tektronix_set_state,
                                          keysight_get_state};

static const struct {
    const char *          vendor;
    const char *          model;
    const scope_config_t *config;
} id_table[] = {{"Tektronix", NULL, &tektronix_cfg},
                {"KEYSIGHT TECHNOLOGIES", NULL, &keysight_cfg},
                {"Agilent", NULL, &keysight_cfg},
                {NULL, "TMCEMU", &tmcemu_cfg}};

const scope_config_t *detect_scope(USBHTmcDriver *tmcp) {
    static const char idncmd[] = "*IDN?";

    enum { ELEM_VENDOR = 0, ELEM_MODEL = 1 };

    char  buf[256];
    char *elems[4];

    if (!run_ask(tmcp, idncmd, buf, sizeof(buf))) {
        return 0;
    }

    sinfof("Scope *IDN? returns '%s'", buf);

    if (tokenize(buf, elems, 4) != 4) {
        serrf("Failed to tokenize IDN");
        return NULL;
    }

    for (size_t i = 0; i < sizeof(id_table) / sizeof(id_table[0]); i++) {
        if ((!id_table[i].vendor ||
             !strcasecmp(id_table[i].vendor, elems[ELEM_VENDOR])) &&
            (!id_table[i].model ||
             !strcasecmp(id_table[i].model, elems[ELEM_MODEL]))) {
            return id_table[i].config;
        }
    }

    return NULL;
}
