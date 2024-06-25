/******************** (C) COPYRIGHT 2021 INNOTECH **************************
* COMPANY:			INNOTECH
* DATE:				2024/01
* AUTHOR:			qiang.zhang
* IC:				ESP32S3
* DESCRIPTION:	    Mqtt JSON Protocol.
*____________________________________________________________________________
* REVISION  Date		    User            Description
* 1.0		2024/01/20	    qiang.zhang		First release
*
*____________________________________________________________________________

*****************************************************************************/
#include <stdio.h>
#include <string.h>
#include "cJSON.h"
#include "esp_vfs.h"
#include "innotech_mqtt_json.h"
#include "innotech_config.h"
#include "innotech_relay.h"
#include "innotech_meter.h"
#include "innotech_weather.h"

static void mqtt_json_pack_sleep(cJSON *sleepObject, uint8_t id)
{
    innotech_config_t *innotech_config = (innotech_config_t *)innotech_config_get_handle();
    cJSON_AddItemToObject(sleepObject, "PowerSwitch", cJSON_CreateNumber(innotech_config->sleep[id].onoff));			 
    cJSON_AddItemToObject(sleepObject, "TimeLeft", cJSON_CreateNumber(innotech_config->sleep[id].time_left));   
    cJSON_AddItemToObject(sleepObject, "IsRunning", cJSON_CreateNumber(innotech_config->sleep[id].is_running));
    cJSON_AddItemToObject(sleepObject, "ScheduleId", cJSON_CreateString((const char *)innotech_config->sleep[id].schedule_id));
    cJSON_AddItemToObject(sleepObject, "Timestamp", cJSON_CreateString((const char *)innotech_config->sleep[id].timestamp));
}

static void mqtt_json_pack_timer(cJSON *timerObject, uint8_t id)
{
    innotech_config_t *innotech_config = (innotech_config_t *)innotech_config_get_handle();
    cJSON_AddItemToObject(timerObject, "PowerSwitch", cJSON_CreateNumber(innotech_config->timer[id].onoff));			 
    cJSON_AddItemToObject(timerObject, "Timer", cJSON_CreateString((const char *)innotech_config->timer[id].time));   
    cJSON_AddItemToObject(timerObject, "Enable", cJSON_CreateNumber(innotech_config->timer[id].enable));
    cJSON_AddItemToObject(timerObject, "ScheduleId", cJSON_CreateString((const char *)innotech_config->timer[id].schedule_id));
    cJSON_AddItemToObject(timerObject, "IsValid", cJSON_CreateNumber(innotech_config->timer[id].is_valid));
    cJSON_AddItemToObject(timerObject, "Repeat", cJSON_CreateString((const char *)innotech_config->timer[id].repeat));
}

int mqtt_json_last_will(char *result, char * timestamp, char *package_msg)
{
    if( (result == NULL) || (package_msg == NULL) )
    {
        printf("mqtt_json_last_will is null\n");
        return -1;
    }

    cJSON *IOTJSObjectmsg = NULL;
    cJSON *stateJSObject = NULL;
    cJSON *reportedObject = NULL;

    char *iot_jsonmsg = NULL;
    
    if((IOTJSObjectmsg = cJSON_CreateObject()) != NULL)
    {
		cJSON_AddItemToObject(IOTJSObjectmsg, "proType", cJSON_CreateNumber(2));			 
        cJSON_AddItemToObject(IOTJSObjectmsg, "state", stateJSObject = cJSON_CreateObject());   
        cJSON_AddItemToObject(stateJSObject, "reported", reportedObject = cJSON_CreateObject());
        cJSON_AddItemToObject(reportedObject, "connected", cJSON_CreateString(result));//select from device configure
        cJSON_AddItemToObject(reportedObject, "connectedTimestamp", cJSON_CreateString(timestamp));
        iot_jsonmsg = cJSON_PrintUnformatted(IOTJSObjectmsg);  
        
        sprintf(package_msg, "%s", iot_jsonmsg);
        printf("\n\nmsg:%s\n\n", package_msg);
        free((void *)iot_jsonmsg);
        cJSON_Delete(IOTJSObjectmsg);
    }
    return 0;
}

static int mqtt_json_params_pack(char *cmd, cJSON *stateJSObject)
{
    cJSON *timeJSObject  = NULL;
    innotech_config_t *innotech_config = (innotech_config_t *)innotech_config_get_handle();

    if( (strncmp(cmd, "PowerSwitch", strlen(cmd)) == 0) || (strncmp(cmd, "status", strlen(cmd)) == 0))
    {
        cJSON_AddItemToObject(stateJSObject, "PowerSwitch", cJSON_CreateNumber(innotech_config->power_switch));
    }
        
    if( (strncmp(cmd, "ScreenBrightValue", strlen(cmd)) == 0) || (strncmp(cmd, "status", strlen(cmd)) == 0) )
    {
        cJSON_AddItemToObject(stateJSObject, "ScreenBrightValue", cJSON_CreateNumber(innotech_config->lcd_brightness));
    }
        
    if( (strncmp(cmd, "ScreenSwitch", strlen(cmd)) == 0) || (strncmp(cmd, "status", strlen(cmd)) == 0) )
    {
        cJSON_AddItemToObject(stateJSObject, "ScreenSwitch", cJSON_CreateNumber(innotech_config->lcd_switch));
    }
        
    if( (strncmp(cmd, "AutoBrightnessSwitch", strlen(cmd)) == 0) || (strncmp(cmd, "status", strlen(cmd)) == 0) )
    {
        cJSON_AddItemToObject(stateJSObject, "AutoBrightnessSwitch", cJSON_CreateNumber(innotech_config->brightness_switch));
    }
        
    if( (strncmp(cmd, "Memory", strlen(cmd)) == 0) || (strncmp(cmd, "status", strlen(cmd)) == 0) )
    {
        cJSON_AddItemToObject(stateJSObject, "Memory", cJSON_CreateNumber(innotech_config->memory));
    }
    
    if( (strncmp(cmd, "LineDiameter", strlen(cmd)) == 0) || (strncmp(cmd, "status", strlen(cmd)) == 0) )
    {
        cJSON_AddItemToObject(stateJSObject, "LineDiameter", cJSON_CreateNumber(innotech_config->line_diameter));
    }
    
    if( (strncmp(cmd, "RealTimeCurrent", strlen(cmd)) == 0) || (strncmp(cmd, "status", strlen(cmd)) == 0) || (strncmp(cmd, "energy", strlen(cmd)) == 0) )
    {
        cJSON_AddItemToObject(stateJSObject, "RealTimeCurrent", cJSON_CreateNumber(innotech_current_get()));
    }
    
    if( (strncmp(cmd, "RealTimeVoltage", strlen(cmd)) == 0) || (strncmp(cmd, "status", strlen(cmd)) == 0) || (strncmp(cmd, "energy", strlen(cmd)) == 0) )
    {
        cJSON_AddItemToObject(stateJSObject, "RealTimeVoltage", cJSON_CreateNumber(innotech_voltage_get()));
    }

    if( (strncmp(cmd, "RealTimePower", strlen(cmd)) == 0) || (strncmp(cmd, "status", strlen(cmd)) == 0) || (strncmp(cmd, "energy", strlen(cmd)) == 0) )
    {
        cJSON_AddItemToObject(stateJSObject, "RealTimePower", cJSON_CreateNumber(innotech_power_get()));
    }

    if( (strncmp(cmd, "TotalConsumption", strlen(cmd)) == 0) || (strncmp(cmd, "status", strlen(cmd)) == 0) || (strncmp(cmd, "energy", strlen(cmd)) == 0) )
    {
        cJSON_AddItemToObject(stateJSObject, "TotalConsumption", cJSON_CreateNumber(innotech_consumption_get()));
    }

    if( (strncmp(cmd, "LocalTimer_1", strlen(cmd)) == 0))
    {
        cJSON_AddItemToObject(stateJSObject, "LocalTimer_1", timeJSObject = cJSON_CreateObject());
        mqtt_json_pack_timer(timeJSObject, 0);
    }
    else if( (strncmp(cmd, "LocalTimer_2", strlen(cmd)) == 0))
    {
        cJSON_AddItemToObject(stateJSObject, "LocalTimer_2", timeJSObject = cJSON_CreateObject());
        mqtt_json_pack_timer(timeJSObject, 1);
    }
    else if( (strncmp(cmd, "LocalTimer_3", strlen(cmd)) == 0))
    {
        cJSON_AddItemToObject(stateJSObject, "LocalTimer_3", timeJSObject = cJSON_CreateObject());
        mqtt_json_pack_timer(timeJSObject, 2);
    }
    else if( (strncmp(cmd, "LocalTimer_4", strlen(cmd)) == 0))
    {
        cJSON_AddItemToObject(stateJSObject, "LocalTimer_4", timeJSObject = cJSON_CreateObject());
        mqtt_json_pack_timer(timeJSObject, 3);
    }
    else if( (strncmp(cmd, "LocalTimer_5", strlen(cmd)) == 0))
    {
        cJSON_AddItemToObject(stateJSObject, "LocalTimer_5", timeJSObject = cJSON_CreateObject());
        mqtt_json_pack_timer(timeJSObject, 4);
    }
    else if( (strncmp(cmd, "CountDown_1", strlen(cmd)) == 0))
    {
        cJSON_AddItemToObject(stateJSObject, "CountDown_1", timeJSObject = cJSON_CreateObject());
        mqtt_json_pack_sleep(timeJSObject, 0);
    }
    else if( (strncmp(cmd, "CountDown_2", strlen(cmd)) == 0))
    {
        cJSON_AddItemToObject(stateJSObject, "CountDown_2", timeJSObject = cJSON_CreateObject());
        mqtt_json_pack_sleep(timeJSObject, 1);
    }
    else if( (strncmp(cmd, "CountDown_3", strlen(cmd)) == 0))
    {
        cJSON_AddItemToObject(stateJSObject, "CountDown_3", timeJSObject = cJSON_CreateObject());
        mqtt_json_pack_sleep(timeJSObject, 2);
    }
    
    return 0;
}

static void mqtt_json_sys_pack(uint8_t ack, cJSON *sysJSObject)
{
    cJSON_AddItemToObject(sysJSObject, "ack", cJSON_CreateNumber(ack));
}

int mqtt_json_location_get(char *id, char *package_msg)
{
	cJSON *IOTJSObject = NULL, *stateJSObject = NULL;
	char *iot_json = NULL;

	//pack msg
	if((IOTJSObject = cJSON_CreateObject()) != NULL)
	{
		cJSON_AddItemToObject(IOTJSObject, "id", cJSON_CreateString(id));   		 	  
        cJSON_AddItemToObject(IOTJSObject, "params", stateJSObject = cJSON_CreateObject());

		iot_json = cJSON_PrintUnformatted(IOTJSObject);  
		sprintf(package_msg, "%s", iot_json);
		printf("pack:%s\n", package_msg);
		free((void *)iot_json);
		cJSON_Delete(IOTJSObject);
	}
    
    return 0;
}

///////////////msg body////////////////
int mqtt_json_pack(char *cmd, char *id, char *version, char *package_msg)
{
	cJSON *IOTJSObject = NULL, *stateJSObject = NULL, *sysJSObject = NULL;
	char *iot_json = NULL;

	//pack msg
	if((IOTJSObject = cJSON_CreateObject()) != NULL)
	{
		cJSON_AddItemToObject(IOTJSObject, "id", cJSON_CreateString(id));   		 
		cJSON_AddItemToObject(IOTJSObject, "version", cJSON_CreateString(version));		  

		if( (strncmp(cmd, "PowerSwitch", strlen(cmd)) == 0) ||\
            (strncmp(cmd, "ScreenBrightValue", strlen(cmd)) == 0) ||\
            (strncmp(cmd, "ScreenSwitch", strlen(cmd)) == 0) ||\
            (strncmp(cmd, "AutoBrightnessSwitch", strlen(cmd)) == 0) ||\
            (strncmp(cmd, "Memory", strlen(cmd)) == 0) ||\
            (strncmp(cmd, "LineDiameter", strlen(cmd)) == 0) ||\
            (strncmp(cmd, "LocalTimer_1", strlen(cmd)) == 0) ||\
            (strncmp(cmd, "LocalTimer_2", strlen(cmd)) == 0) ||\
            (strncmp(cmd, "LocalTimer_3", strlen(cmd)) == 0) ||\
            (strncmp(cmd, "LocalTimer_4", strlen(cmd)) == 0) ||\
            (strncmp(cmd, "LocalTimer_5", strlen(cmd)) == 0) ||\
            (strncmp(cmd, "CountDown_1", strlen(cmd)) == 0) ||\
            (strncmp(cmd, "CountDown_2", strlen(cmd)) == 0) ||\
			(strncmp(cmd, "CountDown_3", strlen(cmd)) == 0) ||\
            (strncmp(cmd, "energy", strlen(cmd)) == 0))
		{
			cJSON_AddItemToObject(IOTJSObject, "params", stateJSObject = cJSON_CreateObject());
            cJSON_AddItemToObject(IOTJSObject, "sys", sysJSObject = cJSON_CreateObject());
			mqtt_json_params_pack(cmd, stateJSObject);
            mqtt_json_sys_pack(0, sysJSObject);
		}
		else if(strncmp(cmd, "status", strlen(cmd)) == 0)
		{
			cJSON_AddItemToObject(IOTJSObject, "params", stateJSObject = cJSON_CreateObject());
            cJSON_AddItemToObject(IOTJSObject, "sys", sysJSObject = cJSON_CreateObject());
			mqtt_json_params_pack(cmd, stateJSObject);
            mqtt_json_sys_pack(1, sysJSObject);
		}

		iot_json = cJSON_PrintUnformatted(IOTJSObject);  
		sprintf(package_msg, "%s", iot_json);
		printf("pack:%s\n", package_msg);
		free((void *)iot_json);
		cJSON_Delete(IOTJSObject);
	}
    
    return 0;
}

void mqtt_json_pack_reply(char *id, char *version, char *package_msg)
{
	cJSON *IOTJSObject = NULL, *dataJSObject = NULL;
	char *iot_json = NULL;

	//pack msg
	if((IOTJSObject = cJSON_CreateObject()) != NULL)
	{
        cJSON_AddItemToObject(IOTJSObject, "code", cJSON_CreateNumber(200)); 
        cJSON_AddItemToObject(IOTJSObject, "data", dataJSObject = cJSON_CreateObject());
		cJSON_AddItemToObject(IOTJSObject, "id", cJSON_CreateString(id));  
        cJSON_AddItemToObject(IOTJSObject, "message", cJSON_CreateString("success")); 		 
		cJSON_AddItemToObject(IOTJSObject, "version", cJSON_CreateString(version));		  

		iot_json = cJSON_PrintUnformatted(IOTJSObject);  
		sprintf(package_msg, "%s", iot_json);
		printf("pack:%s\n", package_msg);
		free((void *)iot_json);
		cJSON_Delete(IOTJSObject);
	}

}

void mqtt_json_pack_ota(char *id, char *version, char *package_msg)
{
	cJSON *IOTJSObject = NULL, *dataJSObject = NULL;
	char *iot_json = NULL;

	//pack msg
	if((IOTJSObject = cJSON_CreateObject()) != NULL)
	{
		cJSON_AddItemToObject(IOTJSObject, "id", cJSON_CreateString(id));  		 
		cJSON_AddItemToObject(IOTJSObject, "version", cJSON_CreateString(version));
        cJSON_AddItemToObject(IOTJSObject, "params", dataJSObject = cJSON_CreateObject());
        cJSON_AddItemToObject(dataJSObject, "version", cJSON_CreateString(PT01_VERSION));
        cJSON_AddItemToObject(dataJSObject, "module", cJSON_CreateString("default"));		  

		iot_json = cJSON_PrintUnformatted(IOTJSObject);  
		sprintf(package_msg, "%s", iot_json);
		printf("pack:%s\n", package_msg);
		free((void *)iot_json);
		cJSON_Delete(IOTJSObject);
	}

}

static void mqtt_json_unpack_location(cJSON *locationObject)
{
    if(locationObject == NULL)
    {
        printf("unpack_params null\n");
        return;
    }
    cJSON *latitudeObject, *longitudeObject, *idObject;
    location_t location;

    if( (idObject = cJSON_GetObjectItem(locationObject, "CityId")) != NULL )
    {
        location.city_id = idObject->valueint;
    }
    if( (latitudeObject = cJSON_GetObjectItem(locationObject, "Latitude")) != NULL )
    {
        location.latitude = latitudeObject->valuedouble;
    }
    if( (longitudeObject = cJSON_GetObjectItem(locationObject, "Longitude")) != NULL )
    {
        location.longitude = longitudeObject->valuedouble;
    }
    innotech_location_set(location);
}

static void mqtt_json_unpack_sleep(cJSON *sleepObject, uint8_t id)
{
    if(sleepObject == NULL)
    {
        printf("unpack_params null\n");
        return;
    }
    cJSON *onoffObject, *timeObject, *runObject, *stampObject, *idObject;
    innotech_config_t *innotech_config = (innotech_config_t *)innotech_config_get_handle();

    if( (onoffObject = cJSON_GetObjectItem(sleepObject, "PowerSwitch")) != NULL )
    {
        innotech_config->sleep[id].onoff = onoffObject->valueint;
    }
    if( (timeObject = cJSON_GetObjectItem(sleepObject, "TimeLeft")) != NULL )
    {
        innotech_config->sleep[id].time_left = timeObject->valueint;
    }
    if( (runObject = cJSON_GetObjectItem(sleepObject, "IsRunning")) != NULL )
    {
        innotech_config->sleep[id].is_running = runObject->valueint;
    }
    if( (idObject = cJSON_GetObjectItem(sleepObject, "ScheduleId")) != NULL )
    {
        memcpy((void *)innotech_config->sleep[id].schedule_id, (void *)idObject->valuestring, strlen(idObject->valuestring)+1);
    }
    if( (stampObject = cJSON_GetObjectItem(sleepObject, "Timestamp")) != NULL )
    {
        memcpy((void *)innotech_config->sleep[id].timestamp, (void *)stampObject->valuestring, strlen(stampObject->valuestring)+1);
    }
}

static void mqtt_json_unpack_timer(cJSON *timerObject, uint8_t id)
{
    if(timerObject == NULL)
    {
        printf("unpack_params null\n");
        return;
    }
    cJSON *onoffObject, *timeObject, *enableObject, *repeatObject, *idObject, *validObject;
    innotech_config_t *innotech_config = (innotech_config_t *)innotech_config_get_handle();

    if( (onoffObject = cJSON_GetObjectItem(timerObject, "PowerSwitch")) != NULL )
    {
        innotech_config->timer[id].onoff = onoffObject->valueint;
    }
    if( (timeObject = cJSON_GetObjectItem(timerObject, "Timer")) != NULL )
    {
        memcpy((void *)innotech_config->timer[id].time, (void *)timeObject->valuestring, strlen(timeObject->valuestring)+1);
    }
    if( (enableObject = cJSON_GetObjectItem(timerObject, "Enable")) != NULL )
    {
        innotech_config->timer[id].enable = enableObject->valueint;
    }
    if( (repeatObject = cJSON_GetObjectItem(timerObject, "Repeat")) != NULL )
    {
        memcpy((void *)innotech_config->timer[id].repeat, (void *)repeatObject->valuestring, strlen(repeatObject->valuestring)+1);
    }
    if( (idObject = cJSON_GetObjectItem(timerObject, "ScheduleId")) != NULL )
    {
        memcpy((void *)innotech_config->timer[id].schedule_id, (void *)idObject->valuestring, strlen(idObject->valuestring)+1);
    }
    if( (validObject = cJSON_GetObjectItem(timerObject, "IsValid")) != NULL )
    {
        innotech_config->timer[id].is_valid = validObject->valueint;
    }
}

static void mqtt_json_unpack_params(cJSON *paramsObject, char *get_cmd)
{
    if(paramsObject == NULL)
    {
        printf("unpack_params null\n");
        return;
    }
    cJSON *data_Object;
    innotech_config_t *innotech_config = (innotech_config_t *)innotech_config_get_handle();

    if( (data_Object = cJSON_GetObjectItem(paramsObject, "PowerSwitch")) != NULL )
    {
        innotech_config->power_switch = data_Object->valueint;
        innotech_set_relay_status(innotech_config->power_switch);
        memcpy(get_cmd, "PowerSwitch", strlen("PowerSwitch")+1);
    }
    else if( (data_Object = cJSON_GetObjectItem(paramsObject, "ScreenSwitch")) != NULL )
    {
        innotech_config->lcd_switch = data_Object->valueint;
        memcpy(get_cmd, "ScreenSwitch", strlen("ScreenSwitch")+1);
    }
    else if( (data_Object = cJSON_GetObjectItem(paramsObject, "ScreenBrightValue")) != NULL )
    {
        innotech_config->lcd_brightness = data_Object->valueint;
        memcpy(get_cmd, "ScreenBrightValue", strlen("ScreenBrightValue")+1);
    }
    else if( (data_Object = cJSON_GetObjectItem(paramsObject, "AutoBrightnessSwitch")) != NULL )
    {
        innotech_config->brightness_switch = data_Object->valueint;
        memcpy(get_cmd, "AutoBrightnessSwitch", strlen("AutoBrightnessSwitch")+1);
    }
    else if( (data_Object = cJSON_GetObjectItem(paramsObject, "LineDiameter")) != NULL )
    {
        innotech_config->line_diameter = data_Object->valuedouble;
        memcpy(get_cmd, "LineDiameter", strlen("LineDiameter")+1);
    }
    else if( (data_Object = cJSON_GetObjectItem(paramsObject, "Memory")) != NULL )
    {
        innotech_config->memory = data_Object->valueint;
        memcpy(get_cmd, "Memory", strlen("Memory")+1);
    }
    else if( (data_Object = cJSON_GetObjectItem(paramsObject, "LocalTimer_1")) != NULL )
    {
        mqtt_json_unpack_timer(data_Object, 0);
        memcpy(get_cmd, "LocalTimer_1", strlen("LocalTimer_1")+1);
    }
    else if( (data_Object = cJSON_GetObjectItem(paramsObject, "LocalTimer_2")) != NULL )
    {
        mqtt_json_unpack_timer(data_Object, 1);
        memcpy(get_cmd, "LocalTimer_2", strlen("LocalTimer_2")+1);
    }
    else if( (data_Object = cJSON_GetObjectItem(paramsObject, "LocalTimer_3")) != NULL )
    {
        mqtt_json_unpack_timer(data_Object, 2);
        memcpy(get_cmd, "LocalTimer_3", strlen("LocalTimer_3")+1);
    }
    else if( (data_Object = cJSON_GetObjectItem(paramsObject, "LocalTimer_4")) != NULL )
    {
        mqtt_json_unpack_timer(data_Object, 3);
        memcpy(get_cmd, "LocalTimer_4", strlen("LocalTimer_4")+1);
    }
    else if( (data_Object = cJSON_GetObjectItem(paramsObject, "LocalTimer_5")) != NULL )
    {
        mqtt_json_unpack_timer(data_Object, 4);
        memcpy(get_cmd, "LocalTimer_5", strlen("LocalTimer_5")+1);
    }
    else if( (data_Object = cJSON_GetObjectItem(paramsObject, "CountDown_1")) != NULL )
    {
        mqtt_json_unpack_sleep(data_Object, 0);
        memcpy(get_cmd, "CountDown_1", strlen("CountDown_1")+1);
    }
    else if( (data_Object = cJSON_GetObjectItem(paramsObject, "CountDown_2")) != NULL )
    {
        mqtt_json_unpack_sleep(data_Object, 1);
        memcpy(get_cmd, "CountDown_2", strlen("CountDown_2")+1);
    }
    else if( (data_Object = cJSON_GetObjectItem(paramsObject, "CountDown_3")) != NULL )
    {
        mqtt_json_unpack_sleep(data_Object, 2);
        memcpy(get_cmd, "CountDown_3", strlen("CountDown_3")+1);
    }
    else if( (data_Object = cJSON_GetObjectItem(paramsObject, "GeoLocatioin")) != NULL )
    {
        mqtt_json_unpack_location(data_Object);
        memcpy(get_cmd, "GeoLocatioin", strlen("GeoLocatioin")+1);
    }
}

//return -1:fail,0 app read device's status,1 app set device
data_permission_e mqtt_json_unpack(char *iot_json, char *get_cmd, char *method, char *id, char *version)
{
    if(iot_json == NULL || get_cmd == NULL || method == NULL || id == NULL || version == NULL)
    {
        printf("mqtt_json_unpack input error\n");
    }

    data_permission_e ret = PERM_INVALID;
    cJSON *IOTJSObject, *idObject, *methodObject, *versionObject, *paramsObject;

    if((IOTJSObject = cJSON_Parse(iot_json)) != NULL)
    {

        if( (idObject = cJSON_GetObjectItem(IOTJSObject, "id")) != NULL )
        {
            memcpy((void *)id, (void *)idObject->valuestring, strlen(idObject->valuestring)+1);
        }
        else
        {
            printf("mqtt_json_unpack no id\r\n");
        }

        if( (methodObject = cJSON_GetObjectItem(IOTJSObject, "method")) != NULL )
        {
            memcpy((void *)method, (void *)methodObject->valuestring, strlen(methodObject->valuestring)+1);
        }
        else
        {
            printf("mqtt_json_unpack no method\r\n");
        }

        if( (versionObject = cJSON_GetObjectItem(IOTJSObject, "version")) != NULL )
        {
            memcpy((void *)version, (void *)versionObject->valuestring, strlen(versionObject->valuestring)+1);
        }
        else
        {
            printf("mqtt_json_unpack no version\r\n");
        }

        if( (paramsObject= cJSON_GetObjectItem(IOTJSObject, "params")) != NULL )
        {
            mqtt_json_unpack_params(paramsObject, get_cmd);
            ret = PERM_WRITE;
        }
        else
        {
            printf("mqtt_json_unpack no data\r\n");
        }

        cJSON_Delete(IOTJSObject);
    }
    return ret;
}

int mqtt_ota_json_unpack(char *json_data, char *id, char *version, char *ota_url, char *taskId)
{
    cJSON *data = cJSON_Parse(json_data);
    if (data == NULL) {
        printf("Error parsing JSON: %s\n", cJSON_GetErrorPtr());
        return -1;
    }

    cJSON *jsonid = cJSON_GetObjectItem(data, "id");
    cJSON *json_version = cJSON_GetObjectItem(data, "version");
    cJSON *params = cJSON_GetObjectItem(data, "params");
    cJSON *url = cJSON_GetObjectItem(params, "url");
    cJSON *dataId = cJSON_GetObjectItem(params, "taskId");
    cJSON *file_size = cJSON_GetObjectItem(params, "size");

    memcpy(id, jsonid->valuestring, strlen(jsonid->valuestring));
    memcpy(version, json_version->valuestring, strlen(json_version->valuestring));
    memcpy(ota_url, url->valuestring, strlen(url->valuestring));
    memcpy(taskId, dataId->valuestring, strlen(dataId->valuestring));

    cJSON_Delete(data);
    return file_size->valueint;
}

void mqtt_ota_pack_reply(char *id, char *version, char *taskId, char *package_msg)
{
	cJSON *IOTJSObject = NULL, *dataJSObject = NULL;
	char *iot_json = NULL;

	//pack msg
	if((IOTJSObject = cJSON_CreateObject()) != NULL)
	{
		cJSON_AddItemToObject(IOTJSObject, "id", cJSON_CreateString(id));   		 
		cJSON_AddItemToObject(IOTJSObject, "version", cJSON_CreateString(version));	
        cJSON_AddItemToObject(IOTJSObject, "data", dataJSObject = cJSON_CreateObject());
        cJSON_AddItemToObject(dataJSObject, "taskId", cJSON_CreateString(taskId));		  
		iot_json = cJSON_PrintUnformatted(IOTJSObject);  
		sprintf(package_msg, "%s", iot_json);
		//printf("pack:%s\n", package_msg);
		free((void *)iot_json);
		cJSON_Delete(IOTJSObject);
	}

}

void mqtt_ota_pack_step(char *id, char *version, char *taskId, int step, char *package_msg)
{
	cJSON *IOTJSObject = NULL, *dataJSObject = NULL;
	char *iot_json = NULL;

	//pack msg
	if((IOTJSObject = cJSON_CreateObject()) != NULL)
	{
		cJSON_AddItemToObject(IOTJSObject, "id", cJSON_CreateString(id));   		 
		cJSON_AddItemToObject(IOTJSObject, "version", cJSON_CreateString(version));	
        cJSON_AddItemToObject(IOTJSObject, "params", dataJSObject = cJSON_CreateObject());
        cJSON_AddItemToObject(dataJSObject, "taskId", cJSON_CreateString(taskId));		  
        if(step != 0)
        {
            cJSON_AddItemToObject(dataJSObject, "step", cJSON_CreateNumber(step));
        }
		iot_json = cJSON_PrintUnformatted(IOTJSObject);  
		sprintf(package_msg, "%s", iot_json);
		//printf("pack:%s\n", package_msg);
		free((void *)iot_json);
		cJSON_Delete(IOTJSObject);
	}

}
void mqtt_timestamp_pack(char *id, char *timestamp, char *package_msg)
{
	cJSON *IOTJSObject = NULL, *dataJSObject = NULL;
	char *iot_json = NULL;

	//pack msg
	if((IOTJSObject = cJSON_CreateObject()) != NULL)
	{
        cJSON_AddItemToObject(IOTJSObject, "id", cJSON_CreateString(id));    			 
        cJSON_AddItemToObject(IOTJSObject, "params", dataJSObject = cJSON_CreateObject());
        cJSON_AddItemToObject(dataJSObject, "TimeStamp", cJSON_CreateString(timestamp));
		iot_json = cJSON_PrintUnformatted(IOTJSObject);  
		sprintf(package_msg, "%s", iot_json);
		//printf("pack:%s\n", package_msg);
		free((void *)iot_json);
		cJSON_Delete(IOTJSObject);
	}
}