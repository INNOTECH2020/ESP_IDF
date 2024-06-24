/******************** (C) COPYRIGHT 2021 INNOTECH **************************
* COMPANY:			INNOTECH
* DATE:				2021/11
* AUTHOR:			qiang.zhang
* IC:				ESP32S3
* DESCRIPTION:	    Device Config Info.
*____________________________________________________________________________
* REVISION  Date		    User            Description
* 1.0		2021/11/24	    qiang.zhang		First release
*
*____________________________________________________________________________

*****************************************************************************/

#pragma once
#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C"
{
#endif

/* Exported macro -----------------------------------------------------------*/
#define PT01_VERSION "0.0.1_0015"

/* Exported constants --------------------------------------------------------*/


typedef struct _innotech_timer_t{
    uint8_t schedule_id[50];
    bool enable;//0-disable,1-enable
    bool is_valid;//0-not executable,1-executable(Not executable indicates that the timer is not initialized)
    bool onoff;//0-off,1-on
    uint8_t time[32];
    uint8_t repeat[32];
}innotech_timer_t;

typedef struct _innotech_count_down_t{  //for sleep
    uint8_t schedule_id[50];
    int32_t time_left;//Value range: 1 to 86399(unit: minute)
    bool onoff;//0-off,1-on
    uint8_t timestamp[32];//String UTC timestamp (milliseconds)
    bool is_running;//0-not executable,1-executable
}innotech_count_down_t;
typedef struct _innotech_config_t
{
    bool power_switch;
    bool lcd_switch;
    bool brightness_switch;
    uint8_t lcd_brightness;
    bool memory;
    double line_diameter;
    innotech_timer_t timer[5];
    innotech_count_down_t sleep[3];
} innotech_config_t;

/* Exported functions ------------------------------------------------------- */
void innotech_config_check(void);
void* innotech_config_get_handle(void);
void innotech_config_init(void);
void innotech_default_device_config(void);

#ifdef __cplusplus
}
#endif
