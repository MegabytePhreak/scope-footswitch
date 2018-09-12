#ifndef _SCOPE_H
#define _SCOPE_H


typedef struct USBHTmcDriver USBHTmcDriver;


//typedef struct USBHTmcDriver;

typedef enum {
    SCOPE_STATE_STOPPED,
    SCOPE_STATE_RUNNING,
    SCOPE_STATE_SINGLE
} scope_state_t;

typedef struct {
    int (*set_state)(USBHTmcDriver* tmcp, scope_state_t state);
    int (*get_state)(USBHTmcDriver* tmcp, scope_state_t* state);
} scope_config_t;


const scope_config_t * detect_scope(USBHTmcDriver* tmcp);


#endif /* _SCOPE_H */

