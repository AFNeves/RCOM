// File containing all the macros used in the project

#define FLAG 0x7E

// SET UP

#define SET_UP_ADDRESS 0x03
#define SET_UP_CONTROL 0x03
#define SET_UP_BCC (SET_UP_ADDRESS ^ SET_UP_CONTROL)

// UA

#define UA_ADDRESS 0x01
#define UA_CONTROL 0x07
#define UA_BCC (UA_ADDRESS ^ UA_CONTROL)

// STATE MACHINE

#define STATE_START 0
#define STATE_FLAG_RCV 1
#define STATE_A_RCV 2
#define STATE_C_RCV 3
#define STATE_BCC_OK 4
#define STATE_STOP 5
