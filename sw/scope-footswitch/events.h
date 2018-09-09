#ifndef EVENTS_H
#define EVENTS_H

#include "hal.h"

enum events {
	EVT_NOP = 0,
	EVT_STARTUP,
	EVT_BTN_CLICK,
	EVT_BTN_HOLD,
	EVT_FOOTSW1_PRESS,
	EVT_FOOTSW2_PRESS,
	EVT_MODE_CHANGE,
	EVT_VBUS_FAULT
};

extern input_queue_t event_queue;
void events_init(void);

#endif