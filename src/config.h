#ifndef CONFIG_H
#define CONFIG_H

//#define LOGGING

#define VERSION                0.6

#define PIN_KOCHER_RELAIS 	    23
#define PIN_RW_RELAIS           25
#define PIN_PWM_RUEHRWERK 	    13 
#define PIN_KOCHER_AUTO 	      22  
#define PIN_KOCHER_MAN		      24  
#define PIN_RW_AUTO		          26  
#define PIN_RW_MAN		          28  
#define PIN_ONE_WIRE_BUS        32

#define BT_CMD_START            1
#define BT_CMD_STOP             2
#define BT_CMD_TIMER_UPDATE     3
#define BT_CMD_SEND_RECIPE      4
#define BT_CMD_MASHED           5
#define BT_CMD_PLUS_5           6
#define BT_CMD_PREVIOUS_STEP    7
#define BT_CMD_NEXT_STEP        8

#endif
