/******************** (C) COPYRIGHT 2021 INNOTECH **************************
* COMPANY:			INNOTECH
* DATE:				2021/08
* AUTHOR:			qiang.zhang
* IC:				ESP32S3
* DESCRIPTION:	    Device Service Handle.
*____________________________________________________________________________
* REVISION  Date		    User            Description
* 1.0		2021/08/29	    qiang.zhang		First release
*
*____________________________________________________________________________

*****************************************************************************/
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "innotech_device.h"
#include "innotech_lcd.h"
#include "innotech_config.h"
#include "innotech_ble.h"
#include "innotech_wifi.h"
#include "innotech_rtc.h"
#include "innotech_factory.h"
#include "innotech_relay.h"
#include "innotech_button.h"
#include "innotech_meter.h"
#include "innotech_weather.h"
#include "api_bridge.h"

static uint16_t time_tick = 0;
static uint16_t energy_tick = 0;
static uint16_t ble_tick = 0;

void innotech_update_save_tick(void)
{
    time_tick = 0;
}

void innotech_device_service_start(void)
{
    
}

void innotech_device_service_handle(void *args)
{
    while(1)
    {
        innotech_lcd_process();
        innotech_button_process();
        innotech_meter_process();
        innotech_overload_buzzer();
        if(++time_tick >= 150)
        {
            time_tick = 0;
            innotech_config_check();
            mqtt_send_ota_step();
        }
        if(++energy_tick >= 3000)
        {
            energy_tick = 0;
            mqtt_send_device_energy();
        }
        if(++ble_tick >= 30000)
        {
            ble_tick = 0;
            innotech_ble_connect_timeout();
        }
        vTaskDelay(20 / portTICK_PERIOD_MS);
       /*static int tick = 0;
        if(++tick >= 50)
        {
            tick = 0;
            printf("heap size: %ld\r\n", innotech_get_heap_size());
        }*/
    }    
}

void innotech_device_service_init(void)
{    
    if(xTaskCreate(&innotech_device_service_handle, "device_service_handle", 5120, NULL, 23, NULL) != pdTRUE)
    {
        //ESP_LOGI(TAG, "create device process task failed!!!");
    }
}

void innotech_device_init(void)
{
    innotech_flash_init();
    innotech_triplet_init();
    innotech_config_init();
    innotech_factory_init();
    innotech_relay_init();
    innotech_button_init();
    innotech_meter_init();
    innotech_lcd_init();
    innotech_wifi_init();
    innotech_ble_init();
    innotech_rtc_init();
    innotech_weather_init();
	innotech_device_service_init();
    innotech_device_service_start();
}