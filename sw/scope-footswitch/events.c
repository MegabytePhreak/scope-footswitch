#include "events.h"

#define LONGPRESS_MS 1000

static uint8_t event_queue_data[16];
input_queue_t  event_queue;

enum {
    SRC_BTN,
    SRC_FOOTSW_BTN_1,
    SRC_FOOTSW_BTN_2,
    SRC_MODE,
    SRC_HOST_FAULT_N
};

enum {
    STATE_IDLE,
    STATE_BTN_PRESSED,
    STATE_BTN_LONG_PRESS,
};

enum {
    FLAG_INVERT     = 1,
    FLAG_BUTTON     = 2,
    FLAG_LONG_PRESS = 4,
};

typedef struct {
    uint8_t         state;
    virtual_timer_t vt;
} evt_state_t;
typedef struct {
    ioline_t line;
    uint8_t  flags;
    msg_t    events[2];
} evt_config_t;

static evt_state_t        evt_state[5]  = {};
static const evt_config_t evt_config[5] = {
    [SRC_BTN]          = {LINE_BTN,
                 FLAG_BUTTON | FLAG_LONG_PRESS,
                 {EVT_BTN_CLICK, EVT_BTN_HOLD}},
    [SRC_FOOTSW_BTN_1] = {LINE_FOOTSW_BTN1,
                          FLAG_BUTTON | FLAG_INVERT,
                          {EVT_FOOTSW1_PRESS, 0}},
    [SRC_FOOTSW_BTN_2] = {LINE_FOOTSW_BTN2,
                          FLAG_BUTTON | FLAG_INVERT,
                          {EVT_FOOTSW2_PRESS, 0}},
    [SRC_MODE]         = {LINE_MODE, 0, {EVT_MODE_CHANGE, EVT_MODE_CHANGE}},
    [SRC_HOST_FAULT_N] = {LINE_HOST_FAULT_N, FLAG_INVERT, {EVT_VBUS_FAULT, 0}},
};

static void longpress_cb(void *arg) {
    chSysLockFromISR();
    size_t src = (size_t)arg;
    if (src < (sizeof(evt_config) / sizeof(evt_config[0]))) {
        const evt_config_t *cfg   = &evt_config[src];
        evt_state_t *       state = &evt_state[src];
        if (state->state == STATE_BTN_PRESSED) {

            state->state = STATE_BTN_LONG_PRESS;
            if (cfg->events[1]) {
                iqPutI(&event_queue, cfg->events[1]);
            }
        }
    }

    chSysUnlockFromISR();
}

static void pal_event_cb(void *arg) {
    size_t src = (size_t)arg;

    chSysLockFromISR();
    if (src < (sizeof(evt_config) / sizeof(evt_config[0]))) {
        const evt_config_t *cfg   = &evt_config[src];
        evt_state_t *       state = &evt_state[src];

        uint8_t pin_state = palReadLine(cfg->line);
        if (cfg->flags & FLAG_INVERT)
            pin_state = !pin_state;
        if (cfg->flags & FLAG_BUTTON) {
            if (pin_state) {
                if (state->state == STATE_IDLE) {
                    state->state = STATE_BTN_PRESSED;
                    if (cfg->flags & FLAG_LONG_PRESS) // Detect long press
                    {
                        if (!chVTIsArmedI(&state->vt))
                            chVTSetI(&state->vt, TIME_MS2I(LONGPRESS_MS),
                                     longpress_cb, 0);
                    } else if (cfg->events[0]) { // Generate immediate event for
                                                 // low latecy processing
                        iqPutI(&event_queue, cfg->events[0]);
                    }
                }
            } else {
                if (cfg->flags & FLAG_LONG_PRESS) {
                    chVTResetI(&state->vt);
                    if (state->state == STATE_BTN_PRESSED && cfg->events[0]) {
                        iqPutI(&event_queue, cfg->events[0]);
                    }
                } else {
                    if (state->state == STATE_BTN_PRESSED && cfg->events[1]) {
                        iqPutI(&event_queue, cfg->events[1]);
                    }
                }
                state->state = STATE_IDLE;
            }
        } else { // Generate event 0/1 based on state
            msg_t evt = cfg->events[pin_state];
            if (evt) {
                iqPutI(&event_queue, evt);
            }
        }
    }
    chSysUnlockFromISR();
}

void events_init(void) {
    chSysLock();
    iqObjectInit(&event_queue, event_queue_data, sizeof(event_queue_data), NULL,
                 NULL);
    iqPutI(&event_queue, EVT_STARTUP);
    for (size_t i = 0; i < (sizeof(evt_config) / sizeof(evt_config[0])); i++) {
        palSetLineCallbackI(evt_config[i].line, pal_event_cb, (void *)i);
        palEnableLineEventI(evt_config[i].line, PAL_EVENT_MODE_BOTH_EDGES);
    }
    chSysUnlock();
}
