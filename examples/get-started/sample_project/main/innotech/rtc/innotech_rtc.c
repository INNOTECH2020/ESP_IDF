/******************** (C) COPYRIGHT 2024 INNOTECH **************************
* COMPANY:			INNOTECH
* DATE:				2024/02
* AUTHOR:			qiang.zhang
* IC:				ESP32C3
* DESCRIPTION:	    Device Network Time Synchronization.
*____________________________________________________________________________
* REVISION  Date		    User            Description
* 1.0		2024/02/27	    qiang.zhang		First release
*
*____________________________________________________________________________

*****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "api_bridge.h"
#include "innotech_wifi.h"
#include "innotech_rtc.h"
#include "innotech_config.h"
#include "innotech_relay.h"

#define RTC_TAG "INNOTECH_RTC"
time_t now;
struct tm timeinfo;

struct tm innotech_time_get(void)
{
    return timeinfo;
}

time_t innotech_timestamp_get(void)
{
    return now;
}

static uint16_t timer_repeat_get(uint8_t *arr, int size) 
{
    uint16_t repeat = 0x80;

    for (int i = 0; i < size; i++) 
    {
        if (isdigit(arr[i])) 
        {
            repeat |= (1 << (i / 2));
        }
    }

    return repeat;
}

static void innotech_rtc_thread(void *arg)
{
    while(!innotech_wifi_state_get())  //must be wifi connected
    {
        // ESP_LOGI(RTC_TAG, "RTC wait wifi connect!!!\n");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    innotech_config_t *innotech_config = (innotech_config_t *)innotech_config_get_handle();
    char cmd[16] = {0};
    
    time(&now);
    localtime_r(&now, &timeinfo);
    // Set timezone to China Standard Time
    setenv("TZ", "CST-8", 1);
    tzset();
    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2016 - 1900)) {
        ESP_LOGI(RTC_TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        innotech_initialize_sntp();
        // update 'now' variable with current time
        time(&now);
    }
    // wait for time to be set
    // while (innotech_get_sync_status()) 
    // {
    //     ESP_LOGI(RTC_TAG, "Waiting for system time to be set...");
    //     vTaskDelay(2000 / portTICK_PERIOD_MS);
    // }

    while(1)
    {
        uint8_t hour;
        time(&now);
        localtime_r(&now, &timeinfo);
        //printf("[sync time]%d-%d-%d %d:%d:%d week:%d\n", timeinfo.tm_year+1900, timeinfo.tm_mon+1, 
        //    timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, timeinfo.tm_wday);
        for(uint8_t i = 0; i < 5; i++)
		{
			if(innotech_config->timer[i].enable == 1)
			{
				uint8_t onoff = innotech_config->timer[i].onoff;
				
				uint8_t min = atoi((char *)&(innotech_config->timer[i].time[0]));
                if(min >= 10)
                {
                    hour = atoi((char *)&(innotech_config->timer[i].time[3]));
                }else
                {
                    hour = atoi((char *)&(innotech_config->timer[i].time[2]));
                }
				uint8_t repeat = timer_repeat_get(innotech_config->timer[i].repeat, strlen((char *)innotech_config->timer[i].repeat)+1);
                
                //printf("timer[%d]:onoff:%d, time %d:%d, repeat: %d \r\n", i, onoff, hour, min, repeat);
				if(((repeat == 0x80) || (((repeat&0x7f) & (1 << (timeinfo.tm_wday - 1))) !=0))
					&&((hour == timeinfo.tm_hour) && (min == timeinfo.tm_min)&& (timeinfo.tm_sec == 0)))
				{
					printf("detect timer %d \r\n",i);
                    if(repeat ==0x80)
					{
						innotech_config->timer[i].enable = 0;
					}
					if(onoff == 1)//open
					{
                        if(innotech_config->power_switch == 0)
                        {
                            innotech_config->power_switch = 1;
                            innotech_set_relay_status(innotech_config->power_switch);
                            memcpy(cmd, "PowerSwitch", strlen("PowerSwitch")+1);
                            mqtt_send_device_info(cmd);
                            switch(i)
                            {
                                case 0:
                                    memcpy(cmd, "LocalTimer_1", strlen("LocalTimer_1")+1);
                                    mqtt_send_device_info(cmd);
                                    break;
                                case 1:
                                    memcpy(cmd, "LocalTimer_2", strlen("LocalTimer_2")+1);
                                    mqtt_send_device_info(cmd);
                                    break;
                                case 2:
                                    memcpy(cmd, "LocalTimer_3", strlen("LocalTimer_3")+1);
                                    mqtt_send_device_info(cmd);
                                    break;
                                case 3:
                                    memcpy(cmd, "LocalTimer_4", strlen("LocalTimer_4")+1);
                                    mqtt_send_device_info(cmd);
                                    break;
                                case 4:
                                    memcpy(cmd, "LocalTimer_5", strlen("LocalTimer_5")+1);
                                    mqtt_send_device_info(cmd);
                                    break;
                            }
                        }
					}
					else//close
					{
						if(innotech_config->power_switch == 1)
                        {
                            innotech_config->power_switch = 0;
                            innotech_set_relay_status(innotech_config->power_switch);
                            memcpy(cmd, "PowerSwitch", strlen("PowerSwitch")+1);
                            mqtt_send_device_info(cmd);
                            switch(i)
                            {
                                case 0:
                                    memcpy(cmd, "LocalTimer_1", strlen("LocalTimer_1")+1);
                                    mqtt_send_device_info(cmd);
                                    break;
                                case 1:
                                    memcpy(cmd, "LocalTimer_2", strlen("LocalTimer_2")+1);
                                    mqtt_send_device_info(cmd);
                                    break;
                                case 2:
                                    memcpy(cmd, "LocalTimer_3", strlen("LocalTimer_3")+1);
                                    mqtt_send_device_info(cmd);
                                    break;
                                case 3:
                                    memcpy(cmd, "LocalTimer_4", strlen("LocalTimer_4")+1);
                                    mqtt_send_device_info(cmd);
                                    break;
                                case 4:
                                    memcpy(cmd, "LocalTimer_5", strlen("LocalTimer_5")+1);
                                    mqtt_send_device_info(cmd);
                                    break;
                            }
                        }					
					}                    
				}
			}
		}
        for(uint8_t i = 0; i < 3; i++)
		{
			if(innotech_config->sleep[i].is_running == 1)
			{
				uint8_t onoff = innotech_config->sleep[i].onoff;
				int32_t time_left = innotech_config->sleep[i].time_left;
                time_t timestamp = atoll((char *)innotech_config->sleep[i].timestamp) / 1000;
                //printf("sleep[%d]:onoff:%d, time_left %ld, nowstamp: %lld timestamp: %lld\r\n", i, onoff, time_left, now, timestamp);
				if(now - timestamp >= time_left)
				{
                    innotech_config->sleep[i].is_running = 0;
					if(onoff == 1)//open
					{
                        if(innotech_config->power_switch == 0)
                        {
                            innotech_config->power_switch = 1;
                            innotech_set_relay_status(innotech_config->power_switch);
                            memcpy(cmd, "PowerSwitch", strlen("PowerSwitch")+1);
                            mqtt_send_device_info(cmd);
                            switch(i)
                            {
                                case 0:
                                    memcpy(cmd, "CountDown_1", strlen("CountDown_1")+1);
                                    mqtt_send_device_info(cmd);
                                    break;
                                case 1:
                                    memcpy(cmd, "CountDown_2", strlen("CountDown_2")+1);
                                    mqtt_send_device_info(cmd);
                                    break;
                                case 2:
                                    memcpy(cmd, "CountDown_3", strlen("CountDown_3")+1);
                                    mqtt_send_device_info(cmd);
                                    break;
                            }
                        }
					}
					else//close
					{
						if(innotech_config->power_switch == 1)
                        {
                            innotech_config->power_switch = 0;
                            innotech_set_relay_status(innotech_config->power_switch);
                            memcpy(cmd, "PowerSwitch", strlen("PowerSwitch")+1);
                            mqtt_send_device_info(cmd);
                            switch(i)
                            {
                                case 0:
                                    memcpy(cmd, "CountDown_1", strlen("CountDown_1")+1);
                                    mqtt_send_device_info(cmd);
                                    break;
                                case 1:
                                    memcpy(cmd, "CountDown_2", strlen("CountDown_2")+1);
                                    mqtt_send_device_info(cmd);
                                    break;
                                case 2:
                                    memcpy(cmd, "CountDown_3", strlen("CountDown_3")+1);
                                    mqtt_send_device_info(cmd);
                                    break;
                            }
                        }					
					}                    
				}
			}
		}
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

}

void innotech_rtc_init(void)
{
    if(xTaskCreate(innotech_rtc_thread, (char const *)"innotech_rtc_thread", 4096, NULL, 23, NULL) != pdPASS)
    {
        printf("\n\r[%s] Create RTC update task failed", __FUNCTION__);
    }   
}

