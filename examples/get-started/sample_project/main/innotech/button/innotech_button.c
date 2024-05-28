/******************** (C) COPYRIGHT 2023 INNOTECH **************************
* COMPANY:			INNOTECH
* DATE:				2023/05
* AUTHOR:			qiang.zhang
* IC:				ESP32C3
* DESCRIPTION:	    Device button Handle.
*____________________________________________________________________________
* REVISION  Date		    User            Description
* 1.0		2023/05/16	    qiang.zhang		First release
*
*____________________________________________________________________________

*****************************************************************************/
#include <stdio.h>
#include <string.h>
#include "innotech_button.h"
#include "innotech_relay.h"
#include "innotech_config.h"
#include "innotech_factory.h"
#include "innotech_wifi.h"
#include "innotech_ble.h"
#include "innotech_lcd.h"
#include "api_bridge.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define BTN_GPIO_NUM          45

static uint8_t factory_reset_flag = 0;
static uint8_t first_key_press = 0;

uint8_t innotech_factory_flag_get(void)
{
    return factory_reset_flag;
}

uint8_t innotech_first_key_press_get(void)
{
    return first_key_press;
}

void innotech_button_process(void)
{
    static uint32_t key_count = 0;
    innotech_config_t *innotech_config = (innotech_config_t *)innotech_config_get_handle();
    uint8_t key_state = innotech_read_gpio_level(BTN_GPIO_NUM);

    if(key_state == 0)
    {
        key_count++;
    }
    else if(key_count >= 5 && key_count < 20)
    {
        key_count = 0;
        if(innotech_config->power_switch)
        {
            innotech_config->power_switch = 0;
        }
        else
        {
            innotech_config->power_switch = 1;
        }
        innotech_set_relay_status(innotech_config->power_switch);
        if(first_key_press == 0)
        {
            first_key_press = 1;
        }
    }
    // else if(key_count >= 300 && key_count < 600)
    // {
    //     esp_restart();
    // }
    else
    {
        factory_reset_flag = 0;
        key_count = 0;
    }


    if(key_count >= 600)
    {
        factory_reset_flag = 1;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        innotech_factory_reset();
    }else if(key_count >= 300 && key_count < 600)
    {
        factory_reset_flag = 2;
        if(first_key_press == 0)
        {
            first_key_press = 1;
            innotech_wifi_restore();
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            innotech_ble_init();
            innotech_power_switch_change_clear();
        }
        
    }
}

void innotech_button_init(void)
{
    innotech_gpio_mode_init(BTN_GPIO_NUM, 1, 0, 0, 0);
    first_key_press = 0;
}