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

#include <stdio.h>
#include <string.h>
#include "api_bridge.h"
#include "innotech_wifi.h"
#include "innotech_mqtt_json.h"
#include "innotech_config.h"
#include "innotech_factory.h"
#include "innotech_ota.h"
#include "lwip/dns.h"

#include "aiot_mqtt_sign.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_event.h"
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "mqtt_client.h"
#include "mqtt_ca.h"

#include "innotech_meter.h"

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;
typedef struct _aliyun_mqtt_type {
	uint32_t port;
    char address[150];
    char client_id[150];
    char username[65];
    char password[65];
    char pub_topic[100];
    char location_topic[100];
    char clear_topic[100];
    char power_topic[100];
    char reset_set_topic[100];
    char reset_report_topic[100];
    char ota_sub_topic[100];
    char ota_pub_topic[100];
    char ota_report_topic[100];
}aliyun_matt_t;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "INNOTECH_WIFI";

esp_mqtt_client_handle_t client;
static wifi_param_t wifi_config;
static aliyun_triad_t triad_config;
static aliyun_matt_t mqtt_type;
static int s_retry_num = 0;
static uint8_t wifi_connect_state = 0;
static uint8_t wifi_restore_state = 0;
static char ota_taskId[50] = {0};
static uint8_t ota_start_flag = 0;
static uint8_t disconnet_flag = 1;
static uint8_t mqtt_init_flag = 0;

#define ESP_MAXIMUM_RETRY  3
#define H2E_IDENTIFIER ""

// #define   AliyunPublishTopic_user_update    "/sys/g8uj3GQEAGp/C411E10077EF/thing/event/property/post"
// #define   AliyunSubscribeTopic_user_reset   "/sys/g8uj3GQEAGp/C411E10077EF/thing/service/Reset"
// #define   AliyunPublishTopic_device_location    "/sys/g8uj3GQEAGp/C411E10077EF/thing/event/GetLocationEvent/post"         
// #define   AliyunSubscribeTopic_user_clear   "/sys/g8uj3GQEAGp/C411E10077EF/thing/service/ClearConsumption"
// #define   AliyunPublishTopic_device_power    "/sys/g8uj3GQEAGp/C411E10077EF/thing/event/PowerOverloadEvent/post"  
// #define   AliyunPublishTopic_device_reset    "/sys/g8uj3GQEAGp/C411E10077EF/thing/event/ResetEvent/post"  

#define ALIYUN_MQTT_HOST "%s.iot-as-mqtt.cn-shanghai.aliyuncs.com"

#define ALIYUN_MQTT_PUBLISH_TOPIC "/sys/%s/%s/thing/event/property/post"
#define ALIYUN_MQTT_RESET_SET_TOPIC "/sys/%s/%s/thing/service/Reset"
#define ALIYUN_MQTT_LOCATION_TOPIC "/sys/%s/%s/thing/event/GetLocationEvent/post"
#define ALIYUN_MQTT_CLEAR_TOPIC "/sys/%s/%s/thing/service/ClearConsumption"
#define ALIYUN_MQTT_POWER_TOPIC "/sys/%s/%s/thing/event/PowerOverloadEvent/post"
#define ALIYUN_MQTT_RESET_REPORT_TOPIC "/sys/%s/%s/thing/event/ResetEvent/post"
#define ALIYUN_MQTT_OTA_SUBSCRIBE_TOPIC "/%s/%s/user/ota/device/upgrade"
#define ALIYUN_MQTT_OTA_PUBLISH_TOPIC "/%s/%s/user/ota/device/inform"
#define ALIYUN_MQTT_OTA_REPORT_TOPIC "/ota/device/inform/%s/%s"
#define ALIYUN_MQTT_OTA_STEP_TOPIC "/%s/%s/user/ota/device/progress"

callback wifi_connect_result = NULL;
void innotech_wifi_state_report(callback function) 
{
    wifi_connect_result = function;
}

uint8_t innotech_get_ota_start_flag(void)
{
    return ota_start_flag;
}
uint8_t innotech_get_disconnet_flag(void)
{
    return disconnet_flag;
}
void* innotech_triad_get_handle(void)
{
    return (void*)&triad_config;
}

uint8_t innotech_wifi_config_flag_get(void) 
{
    return wifi_config.flag;
}

void innotech_wifi_config_reset(void) 
{
    memset(&wifi_config, 0, sizeof(wifi_param_t));
    innotech_flash_write("wifi", (char *)&wifi_config, sizeof(wifi_param_t));
}

uint8_t innotech_wifi_state_get(void) 
{
    return wifi_connect_state;
}

void mqtt_send_device_status(void)
{
    char payload[1024] = {0};
    char get_cmd[] = "status";
    char id[] = "26";
    char version[] = "1.0";

    mqtt_json_pack(get_cmd, id, version, payload);
    esp_mqtt_client_publish(client, mqtt_type.pub_topic, payload, strlen(payload), 0, 0);
}

void mqtt_send_device_energy(void)
{
    char payload[1024] = {0};
    char get_cmd[] = "energy";
    char id[] = "27";
    char version[] = "1.0";
    if(innotech_wifi_state_get() == 1)
    {
        mqtt_json_pack(get_cmd, id, version, payload);
        esp_mqtt_client_publish(client, mqtt_type.pub_topic, payload, strlen(payload), 0, 0);
    }
    
}

void mqtt_send_device_info(char *cmd)
{
    char payload[1024] = {0};
    char id[] = "26";
    char version[] = "1.0";

    mqtt_json_pack(cmd, id, version, payload);
    esp_mqtt_client_publish(client, mqtt_type.pub_topic, payload, strlen(payload), 0, 0);
}

void mqtt_get_device_location(void)
{
    char payload[256] = {0};
    char id[] = "1103";

    mqtt_json_location_get(id, payload);
    esp_mqtt_client_publish(client, mqtt_type.location_topic, payload, strlen(payload), 0, 0);
}

void mqtt_send_data_reply(char *set_topic, char *id, char *version)
{
    char payload[1024] = {0};
    char reply_topic[64] = {0};

    sprintf(reply_topic, "%s_reply", set_topic);
    //printf("reply_topic=%s\r\n", reply_topic);
    mqtt_json_pack_reply(id, version, payload);
    esp_mqtt_client_publish(client, reply_topic, payload, strlen(payload), 0, 0);
}

void mqtt_send_ota_version(void)
{
    char payload[1024] = {0};
    char id[] = "123";
    char version[] = "1.0";

    mqtt_json_pack_ota(id, version, payload);
    esp_mqtt_client_publish(client, mqtt_type.ota_pub_topic, payload, strlen(payload), 0, 0);
    esp_mqtt_client_publish(client, mqtt_type.ota_report_topic, payload, strlen(payload), 0, 0);
}

void mqtt_send_ota_reply(char *set_topic, char *id, char *version, char *taskId)
{
    char payload[1024] = {0};
    char reply_topic[64] = {0};

    sprintf(reply_topic, "%s_reply", set_topic);
    mqtt_ota_pack_reply(id, version, taskId, payload);
    esp_mqtt_client_publish(client, reply_topic, payload, strlen(payload), 0, 0);
}

void mqtt_send_ota_step(void)
{
    if(ota_start_flag == 1)
    {
        char payload[1024] = {0};
        char id[] = "123";
        char version[] = "1.0";
        char step_topic[100] = {0};

        sprintf(step_topic, ALIYUN_MQTT_OTA_STEP_TOPIC, triad_config.productkey, triad_config.devicename);
        mqtt_ota_pack_step(id, version, ota_taskId, innotech_ota_step_get(), payload);
        esp_mqtt_client_publish(client, step_topic, payload, strlen(payload), 0, 0);

        if(innotech_ota_step_get() == 100)
        {
            vTaskDelay(5000 / portTICK_PERIOD_MS);
            esp_restart();
        }
    }
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    client = event->client;
    char payload[1024] = {0};
    char method[32] = {0};
    char id[12] = {0};
    char version[10] = {0};
    char topic[64] = {0};
    char get_cmd[32] = {0};
    int ret = PERM_INVALID;
 
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        disconnet_flag = 0;
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        mqtt_send_device_status();
        mqtt_get_device_location();
        mqtt_send_ota_version();
        esp_mqtt_client_subscribe(client, mqtt_type.ota_sub_topic, 1);
        break;
    case MQTT_EVENT_DISCONNECTED:
        disconnet_flag = 1;
        dns_clear_cache();
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);

        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        if(memcmp(event->topic, mqtt_type.reset_set_topic, event->topic_len) == 0)
        {
            innotech_factory_reset();
        }
        else if(memcmp(event->topic, mqtt_type.ota_sub_topic, event->topic_len) == 0)
        {
            char ota_url[256] = {0};
            int size = 0;

            size = mqtt_ota_json_unpack(event->data, id, version, ota_url, ota_taskId);
            memcpy(topic, event->topic, event->topic_len);
            mqtt_send_ota_reply(topic, id, version, ota_taskId);
            innotech_ota_start(ota_url, size);
            ota_start_flag = 1;
        }
        else if(memcmp(event->topic, mqtt_type.clear_topic, event->topic_len) == 0)
        {
            innotech_clear_consume();
            mqtt_send_device_energy();
        }
        else
        {
            ret = mqtt_json_unpack(event->data, get_cmd, method, id, version);
            if(ret == PERM_WRITE)
            {
                memcpy(topic, event->topic, event->topic_len);
                mqtt_send_data_reply(topic, id, version);
                //printf("get_cmd: %s\r\n",get_cmd);
                if(strncmp(get_cmd, "GeoLocatioin", strlen(get_cmd)) != 0)
                {
                    mqtt_json_pack(get_cmd, id, version, payload);
                    esp_mqtt_client_publish(client, mqtt_type.pub_topic, payload, strlen(payload), 0, 0); 
                }
                if(strncmp(get_cmd, "LineDiameter", strlen(get_cmd)) == 0)
                {
                    mqtt_send_device_energy(); 
                }else if(strncmp(get_cmd, "ScreenSwitch", strlen(get_cmd)) == 0)
                {
                    mqtt_send_device_energy(); 
                }else if(strncmp(get_cmd, "ScreenBrightValue", strlen(get_cmd)) == 0)
                {
                    mqtt_send_device_energy(); 
                }
            }
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            ESP_LOGI(TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
            ESP_LOGI(TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
            ESP_LOGI(TAG, "Last captured errno : %d (%s)",  event->error_handle->esp_transport_sock_errno,
                     strerror(event->error_handle->esp_transport_sock_errno));
        } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
            ESP_LOGI(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
        } else {
            ESP_LOGW(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt_app_start(void)
{
    
    int rc = 0;
    
    sprintf(mqtt_type.address, ALIYUN_MQTT_HOST, triad_config.productkey);
    sprintf(mqtt_type.pub_topic, ALIYUN_MQTT_PUBLISH_TOPIC, triad_config.productkey, triad_config.devicename);
    sprintf(mqtt_type.location_topic, ALIYUN_MQTT_LOCATION_TOPIC, triad_config.productkey, triad_config.devicename);
    sprintf(mqtt_type.clear_topic, ALIYUN_MQTT_CLEAR_TOPIC, triad_config.productkey, triad_config.devicename);
    sprintf(mqtt_type.power_topic, ALIYUN_MQTT_POWER_TOPIC, triad_config.productkey, triad_config.devicename);
    sprintf(mqtt_type.reset_set_topic, ALIYUN_MQTT_RESET_SET_TOPIC, triad_config.productkey, triad_config.devicename);
    sprintf(mqtt_type.reset_report_topic, ALIYUN_MQTT_RESET_REPORT_TOPIC, triad_config.productkey, triad_config.devicename);
    sprintf(mqtt_type.ota_sub_topic, ALIYUN_MQTT_OTA_SUBSCRIBE_TOPIC, triad_config.productkey, triad_config.devicename);
    sprintf(mqtt_type.ota_pub_topic, ALIYUN_MQTT_OTA_PUBLISH_TOPIC, triad_config.productkey, triad_config.devicename);
    sprintf(mqtt_type.ota_report_topic, ALIYUN_MQTT_OTA_REPORT_TOPIC, triad_config.productkey, triad_config.devicename);
 
    if ((rc = aiotMqttSign(triad_config.productkey, triad_config.devicename, triad_config.devicesecret, mqtt_type.client_id, mqtt_type.username, mqtt_type.password) < 0)) 
    {
		printf("aiotMqttSign -%0x4x\n", -rc);
		return;
	}
    // printf("broker: %s %d\n", mqtt_type.address, strlen(mqtt_type.address));
    // printf("client_id: %s %d\n", mqtt_type.client_id, strlen(mqtt_type.client_id));
    // printf("username: %s %d\n", mqtt_type.username, strlen(mqtt_type.username));
    // printf("password: %s %d\n", mqtt_type.password, strlen(mqtt_type.password));
    // printf("update: %s %d\n", mqtt_type.pub_topic, strlen(mqtt_type.pub_topic));
    // printf("reset: %s %d\n", mqtt_type.location_topic, strlen(mqtt_type.location_topic));
    // printf("location: %s %d\n", mqtt_type.clear_topic, strlen(mqtt_type.clear_topic));
    // printf("clear: %s %d\n", mqtt_type.power_topic, strlen(mqtt_type.power_topic));
    // printf("power: %s %d\n", mqtt_type.reset_set_topic, strlen(mqtt_type.reset_set_topic));
    // printf("reset: %s %d\n", mqtt_type.reset_report_topic, strlen(mqtt_type.reset_report_topic));
    // printf("ota_sub_topic: %s %d\n", mqtt_type.ota_sub_topic, strlen(mqtt_type.ota_sub_topic));
    // printf("ota_pub_topic: %s %d\n", mqtt_type.ota_pub_topic, strlen(mqtt_type.ota_pub_topic));
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.verification.certificate = (const char *)ali_ca_cert,
		.broker.address.hostname = mqtt_type.address,
		.broker.address.port = mqtt_type.port,
        .broker.address.transport = MQTT_TRANSPORT_OVER_SSL,
		.credentials.client_id = mqtt_type.client_id,
		.credentials.username = mqtt_type.username,
		.credentials.authentication.password = mqtt_type.password,

    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) 
    {
        esp_wifi_connect();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) 
    {
        wifi_event_sta_disconnected_t *disconnected = (wifi_event_sta_disconnected_t*) event_data;
        ESP_LOGE(TAG, "Disconnect reason : %d", disconnected->reason);
        if (s_retry_num < ESP_MAXIMUM_RETRY) 
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
            vTaskDelay(s_retry_num * 1000 / portTICK_PERIOD_MS);
        } 
        else 
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGI(TAG,"connect to the AP fail");
            esp_wifi_connect();
            vTaskDelay(30000 / portTICK_PERIOD_MS);
        }
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

        if(mqtt_init_flag == 0)
        {
            mqtt_init_flag = 1;
            mqtt_app_start();
        }
        else
        {
            esp_mqtt_client_reconnect(client);
        }
    }
}

void wifi_init_sta(wifi_param_t wifi)
{
    wifi_config_t wifi_config = 
    {
        .sta = 
        {
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (pasword len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
             * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            .threshold.authmode = WIFI_AUTH_WEP,
            // .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
            // .sae_h2e_identifier = H2E_IDENTIFIER,
        },
    };

    memcpy(wifi_config.sta.ssid, wifi.ssid, wifi.ssid_len);
    memcpy(wifi_config.sta.password, wifi.password, wifi.pwd_len);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    if(wifi_restore_state == 1)
    {
        ESP_ERROR_CHECK(esp_wifi_start() );
    }
    else
    {
        ESP_ERROR_CHECK(esp_wifi_connect() );
    }
    
    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) 
    {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", wifi.ssid, wifi.password);
        wifi_connect_state = 1;
        if(wifi.flag == WIFI_CONFIG_FAIL)
        {
            wifi.flag = WIFI_CONFIG_SUC;
            memcpy(&wifi_config, &wifi, sizeof(wifi_param_t));
            innotech_flash_write("wifi", (char *)&wifi_config, sizeof(wifi_param_t));
            if (wifi_connect_result != NULL)
            {
                wifi_connect_result(1);
            }
        }
    } 
    else if (bits & WIFI_FAIL_BIT) 
    {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", wifi.ssid, wifi.password);
        wifi_connect_state = 2;
        if (wifi_connect_result != NULL)
        {
            wifi_connect_result(0);
        }
    }
    else 
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

void innotech_wifi_init(void)
{
    if(wifi_config.flag == WIFI_CONFIG_SUC)
    {
        wifi_init_sta(wifi_config);
    }
}

void innotech_triplet_init(void)
{
    memset(&wifi_config, 0, sizeof(wifi_param_t));
    innotech_flash_read("wifi", (char *)&wifi_config, sizeof(wifi_param_t));

    memset(&triad_config, 0, sizeof(aliyun_triad_t));
    innotech_triad_get(triad_config.devicename, "DeviceName");
    innotech_triad_get(triad_config.devicesecret, "DeviceSecret");
    innotech_triad_get(triad_config.productkey, "ProductKey");

    printf("name: %s\r\n", triad_config.devicename);
    printf("secret: %s\r\n", triad_config.devicesecret);
    printf("key: %s\r\n", triad_config.productkey);
}

void innotech_netif_init(void)
{
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}
/* Initialize Wi-Fi as sta and set scan method */
uint16_t innotech_wifi_scan(uint8_t* ssid)
{
    // uint16_t number = 30;
    // wifi_ap_record_t ap_info[30];
    uint16_t ap_count = 0;
    // memset(ap_info, 0, sizeof(ap_info));
    
    wifi_scan_config_t scan_config = { 0 };
    scan_config.ssid = ssid;
    esp_wifi_scan_start(&scan_config, true);
    // esp_wifi_scan_start(NULL, true);
    // ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    ESP_LOGI(TAG, "Total APs scanned = %u", ap_count);
    // for (int i = 0; (i < 30) && (i < ap_count); i++) {
    //     ESP_LOGI(TAG, "SSID \t\t%s", ap_info[i].ssid);
    //     ESP_LOGI(TAG, "RSSI \t\t%d", ap_info[i].rssi);
    // }
    return ap_count;
}

void innotech_wifi_restore(void)
{
    wifi_connect_state = 0;
    innotech_wifi_config_reset();
    esp_mqtt_client_disconnect(client);
    esp_wifi_restore();
    wifi_restore_state = 1;
}

