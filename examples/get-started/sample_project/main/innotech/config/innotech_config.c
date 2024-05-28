/******************** (C) COPYRIGHT 2021 INNOTECH **************************
* COMPANY:			INNOTECH
* DATE:				2021/11
* AUTHOR:			qiang.zhang
* IC:				ESP32S3
* DESCRIPTION:	    Device Config Info.
*____________________________________________________________________________
* REVISION  Date		    User            Description
* 1.0		2024/01/04	    qiang.zhang		First release
*
*____________________________________________________________________________

*****************************************************************************/

#include <stdio.h>
#include <string.h>
#include "innotech_config.h"
#include "innotech_wifi.h"
#include "api_bridge.h"

typedef struct _base_config_t
{
    uint8_t flash_init[4];
    bool power_switch;
    bool lcd_switch;
    bool brightness_switch;
    uint8_t lcd_brightness;
    bool memory;
    double line_diameter;
} base_config_t;

typedef struct _timer_config_t
{
    innotech_timer_t timer[5];
} timer_config_t;

typedef struct _sleep_config_t
{
    innotech_count_down_t sleep[3];
} sleep_config_t;

const uint8_t config_init_check[4] = {0x04, 0x01, 0x24, 0x20};
static innotech_config_t innotech_config;
static base_config_t base_config;
static timer_config_t timer_config;
static sleep_config_t sleep_config;

void innotech_config_printf(void)
{
    uint8_t i = 0;

    printf("/*****************************************************************************\r\n");
    printf("*____________________________________________________________________________\r\n");
    printf("power_switch: %d\r\n", innotech_config.power_switch);
    printf("lcd_switch: %d\r\n", innotech_config.lcd_switch);
    printf("brightness_switch: %d\r\n", innotech_config.brightness_switch);
    printf("lcd_brightness: %d\r\n", innotech_config.lcd_brightness);
    for(i = 0; i < 5; i++)
    {
        printf("timer[%d].schedule_id: %s\r\n", i, innotech_config.timer[i].schedule_id);
        printf("timer[%d].enable: %d\r\n", i, innotech_config.timer[i].enable);
        printf("timer[%d].is_valid: %d\r\n", i, innotech_config.timer[i].is_valid);
        printf("timer[%d].onoff: %d\r\n", i, innotech_config.timer[i].onoff);
        printf("timer[%d].time: %s\r\n", i, innotech_config.timer[i].time);
        printf("timer[%d].repeat: %s\r\n", i, innotech_config.timer[i].repeat);
    }

    for(i = 0; i < 3; i++)
    {
        printf("sleep[%d].schedule_id: %s\r\n", i, innotech_config.sleep[i].schedule_id);
        printf("sleep[%d].time_left: %ld\r\n", i, innotech_config.sleep[i].time_left);
        printf("sleep[%d].onoff: %d\r\n", i, innotech_config.sleep[i].onoff);
        printf("sleep[%d].timestamp: %s\r\n", i, innotech_config.sleep[i].timestamp);
        printf("sleep[%d].is_running: %d\r\n", i, innotech_config.sleep[i].is_running);
    }
    printf("memory: %d\r\n", innotech_config.memory);
    printf("*____________________________________________________________________________\r\n");
    printf("*****************************************************************************/\r\n");
}

void innotech_config_copy(void)
{
    innotech_config.power_switch      = base_config.power_switch;
    innotech_config.lcd_switch        = base_config.lcd_switch;
    innotech_config.brightness_switch = base_config.brightness_switch;
    innotech_config.lcd_brightness    = base_config.lcd_brightness;
    innotech_config.line_diameter     = base_config.line_diameter;
    innotech_config.memory            = base_config.memory;

    memcpy(innotech_config.timer, timer_config.timer, sizeof(timer_config_t));
    memcpy(innotech_config.sleep, sleep_config.sleep, sizeof(sleep_config_t));
}

void innotech_base_copy(void)
{
    base_config.power_switch      = innotech_config.power_switch;
    base_config.lcd_switch        = innotech_config.lcd_switch;
    base_config.brightness_switch = innotech_config.brightness_switch;
    base_config.lcd_brightness    = innotech_config.lcd_brightness;
    base_config.line_diameter     = innotech_config.line_diameter;
    base_config.memory            = innotech_config.memory;
}

void innotech_config_check(void)
{
    if (base_config.power_switch      != innotech_config.power_switch ||
        base_config.lcd_switch        != innotech_config.lcd_switch ||
        base_config.brightness_switch != innotech_config.brightness_switch ||
        base_config.lcd_brightness    != innotech_config.lcd_brightness ||
        base_config.line_diameter     != innotech_config.line_diameter ||
        base_config.memory            != innotech_config.memory )
    {
        innotech_base_copy();
        innotech_flash_write("innotech", (char *)&base_config, sizeof(base_config_t));

    }

    if(memcmp(timer_config.timer, innotech_config.timer, sizeof(timer_config_t)) != 0)
    {
        memcpy(timer_config.timer, innotech_config.timer, sizeof(timer_config_t));
        innotech_flash_write("timer", (char *)&timer_config, sizeof(timer_config_t));
    }
    
    if(memcmp(sleep_config.sleep, innotech_config.sleep, sizeof(sleep_config_t)) != 0)
    {
        memcpy(sleep_config.sleep, innotech_config.sleep, sizeof(sleep_config_t));
        innotech_flash_write("sleep", (char *)&sleep_config, sizeof(sleep_config_t));
    }
}

void innotech_default_device_config(void)
{   
    memset(&base_config, 0, sizeof(base_config_t));
    memcpy(base_config.flash_init, config_init_check, 4);
    base_config.power_switch      = true;
    base_config.lcd_switch        = true;
    base_config.brightness_switch = false;
    base_config.lcd_brightness    = 100;
    base_config.line_diameter     = 0;
    base_config.memory            = false;

	innotech_flash_write("innotech", (char *)&base_config, sizeof(base_config_t));

    memset(&timer_config, 0, sizeof(timer_config_t));
    innotech_flash_write("timer", (char *)&timer_config, sizeof(timer_config_t));

    memset(&sleep_config, 0, sizeof(sleep_config_t));
    innotech_flash_write("sleep", (char *)&sleep_config, sizeof(sleep_config_t));

    innotech_config_copy();
}

void* innotech_config_get_handle(void)
{
    return (void*)&innotech_config;
}

void innotech_config_init(void)
{
    uint8_t* p_flash = (uint8_t*)base_config.flash_init;
    uint8_t* p_flag = (uint8_t*)config_init_check;
    uint8_t i = 0;

    memset(&base_config, 0, sizeof(base_config_t));
    innotech_flash_read("innotech", (char *)&base_config, sizeof(base_config_t));

    memset(&timer_config, 0, sizeof(timer_config_t));
    innotech_flash_read("timer", (char *)&timer_config, sizeof(timer_config_t));

    memset(&sleep_config, 0, sizeof(sleep_config_t));
    innotech_flash_read("sleep", (char *)&sleep_config, sizeof(sleep_config_t));

    for(i = 0; i < 4; i++)
    {
        if (p_flash[i] != p_flag[i])
        {
            innotech_default_device_config();
            break;
        }
    }
    innotech_config_copy();
    //innotech_config_printf();
    if(innotech_config.memory == 0 && innotech_reset_reason_get() == 1 && innotech_wifi_config_flag_get() == 1)
    {
        innotech_config.power_switch = 0;
    }
    else if(innotech_reset_reason_get() == 1 && innotech_wifi_config_flag_get() == 0)
    {
        innotech_config.power_switch = 1;
    }
}


