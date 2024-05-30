/******************** (C) COPYRIGHT 2021 INNOTECH **************************
* COMPANY:			INNOTECH
* DATE:				2024/01
* AUTHOR:			qiang.zhang
* IC:				ESP32S3
* DESCRIPTION:	    WiFi Connection.
*____________________________________________________________________________
* REVISION  Date		    User            Description
* 1.0		2024/01/20	    qiang.zhang		First release
*
*____________________________________________________________________________

*****************************************************************************/

#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C"
{
#endif

/* Exported macro -----------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

typedef enum _ENUM_WIFI_CONFIG_{
    WIFI_CONFIG_FAIL  = 0x0,
    WIFI_CONFIG_SUC   = 0x1
}ENUM_WIFI_CONFIG;
typedef struct
{
    uint8_t flag;
    uint8_t ssid_len;
	uint8_t ssid[65];
    uint8_t pwd_len;
	uint8_t password[65];
} wifi_param_t;

typedef struct _aliyun_triad_type {
    char productkey[20];
    char devicename[20];
    char devicesecret[40];
}aliyun_triad_t;

typedef void (*callback)(int);

/* Exported functions ------------------------------------------------------- */
void innotech_wifi_state_report(callback function);
uint8_t innotech_wifi_config_flag_get(void);
void innotech_wifi_config_reset(void);
uint8_t innotech_wifi_state_get(void);
void wifi_init_sta(wifi_param_t wifi);
void mqtt_send_device_info(char *cmd);
void mqtt_send_device_energy(void);
void mqtt_send_ota_step(void);
void mqtt_send_factory_reset(void);
void mqtt_send_power_overload(void);
void* innotech_triad_get_handle(void);
uint16_t innotech_wifi_scan(uint8_t* ssid);
void innotech_netif_init(void);
uint8_t innotech_get_disconnet_flag(void);
void innotech_wifi_init(void);
void innotech_triplet_init(void);
uint8_t innotech_get_ota_start_flag(void);
void innotech_wifi_restore(void);

#ifdef __cplusplus
}
#endif
