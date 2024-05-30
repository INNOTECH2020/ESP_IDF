/******************** (C) COPYRIGHT 2024 INNOTECH **************************
* COMPANY:			INNOTECH
* DATE:				2024/02
* AUTHOR:			qiang.zhang
* IC:				ESP32C3
* DESCRIPTION:	    Device Network Weather Get.
*____________________________________________________________________________
* REVISION  Date		    User            Description
* 1.0		2024/03/06	    qiang.zhang		First release
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
#include "zlib.h"
#include "zutil.h"
#include "inftrees.h"
#include "inflate.h"
#include "innotech_weather.h"
#include "esp_http_client.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_crt_bundle.h"
#include "cJSON.h"
#include "innotech_utils.h"
#include "innotech_rtc.h"

#define TAG "INNOTECH_WEATHER"
#define MAX_RETRY 3

/**
 * @brief See more at https://dev.qweather.com/docs/api/
 *
 */
#define WEATHER_SERVER      "devapi.qweather.com"

#define MAX_HTTP_RECV_BUFFER            (1024)

static weather_t weather_info;
static uint8_t location_flag = 0;
static location_t device;

void innotech_location_set(location_t location)
{
    device.city_id = location.city_id;
    device.latitude = location.latitude;
    device.longitude = location.longitude;
    location_flag = 1;
    printf("city_id: %d latitude: %f longitude:%f\r\n", device.city_id, device.latitude, device.longitude);
}

weather_t innotech_weather_info_get(void)
{
    return weather_info;
}

static int network_gzip_decompress(void *in_buf, size_t in_size, void *out_buf, size_t *out_size, size_t out_buf_size)
{
    int err = 0;
    z_stream d_stream = {0}; /* decompression stream */
    d_stream.zalloc = NULL;
    d_stream.zfree = NULL;
    d_stream.opaque = NULL;
    d_stream.next_in  = in_buf;
    d_stream.avail_in = 0;
    d_stream.next_out = out_buf;

    if ((err = inflateInit2(&d_stream, 47)) != Z_OK) {
        return err;
    }
    while (d_stream.total_out < out_buf_size - 1 && d_stream.total_in < in_size) {
        d_stream.avail_in = d_stream.avail_out = 1;
        if ((err = inflate(&d_stream, Z_NO_FLUSH)) == Z_STREAM_END) {
            break;
        }
        if (err != Z_OK) {
            return err;
        }
    }

    if ((err = inflateEnd(&d_stream)) != Z_OK) {
        return err;
    }

    *out_size = d_stream.total_out;
    ((char *)out_buf)[*out_size] = '\0';

    return Z_OK;
}

uint8_t lcd_icon_id_get(uint8_t weather_id)
{
    uint8_t icon_id = 0;
    struct tm time = innotech_time_get();
    printf("hour: %d\r\n", time.tm_hour);
    if(weather_id == 1 || weather_id == 2 || weather_id == 3 || weather_id == 4 || weather_id == 5 || weather_id == 6 || weather_id == 7 || weather_id == 12)
    {
        if(time.tm_hour > 6 && time.tm_hour < 18)
        {
            icon_id = 2;
        }
        else
        {
            icon_id = 1;
        }
    }
    else if(weather_id == 8 || weather_id == 9 || weather_id == 10 || weather_id == 11 || weather_id == 80 || weather_id == 81 || weather_id == 82)
    {
        if(time.tm_hour > 6 && time.tm_hour < 18)
        {
            icon_id = 4;
        }
        else
        {
            icon_id = 3;
        }
    }
    else if(weather_id == 13 || weather_id == 14 || weather_id == 85)
    {
        icon_id = 5;
    }
    else if(weather_id == 15 || weather_id == 16 || weather_id == 17 || weather_id == 18 || weather_id == 19 || weather_id == 86 || weather_id == 20 || weather_id == 21 || weather_id == 22 || weather_id == 23)
    {
        if(time.tm_hour > 6 && time.tm_hour < 18)
        {
            icon_id = 13;
        }
        else
        {
            icon_id = 11;
        }
    }
    else if(weather_id == 24 || weather_id == 25)
    {
        if(time.tm_hour > 6 && time.tm_hour < 18)
        {
            icon_id = 19;
        }
        else
        {
            icon_id = 17;
        }
    }
    else if(weather_id == 26 || weather_id == 27 || weather_id == 28 || weather_id == 83 || weather_id == 84)
    {
        icon_id = 6;
    }
    else if(weather_id == 29)
    {
        icon_id = 25;
    }
    else if(weather_id == 30 || weather_id == 31 || weather_id == 32 || weather_id == 33)
    {
        icon_id = 24;
    }
    else if(weather_id == 34 || weather_id == 35 || weather_id == 79)
    {
        icon_id = 8;
    }
    else if(weather_id == 36)
    {
        icon_id = 5;
    }
    else if(weather_id == 37 || weather_id == 38 || weather_id == 39 || weather_id == 40 || weather_id == 41  || weather_id == 42 || weather_id == 87 || weather_id == 88 || weather_id == 89 || weather_id == 90)
    {
        icon_id = 9;
    }
    else if(weather_id == 43)
    {
        icon_id = 7;
    }
    else if(weather_id == 44 || weather_id == 45 || weather_id == 46 || weather_id == 47 || weather_id == 48)
    {
        icon_id = 16;
    }
    else if(weather_id == 49 || weather_id == 50 || weather_id == 64 || weather_id == 65)
    {
        icon_id = 18;
    }
    else if(weather_id == 51 || weather_id == 52 || weather_id == 66)
    {
        icon_id = 15;
    }
    else if(weather_id == 53 || weather_id == 67 || weather_id == 78  || weather_id == 91)
    {
        icon_id = 14;
    }
    else if(weather_id == 54 || weather_id == 68 || weather_id == 92)
    {
        icon_id = 12;
    }
    else if(weather_id == 55 || weather_id == 56 || weather_id == 57 || weather_id == 69 || weather_id == 70 || weather_id == 93)
    {
        icon_id = 10;
    }
    else if(weather_id == 58 || weather_id == 59 || weather_id == 71 || weather_id == 72 || weather_id == 73)
    {
        icon_id = 20;
    }
    else if(weather_id == 60 || weather_id == 61 || weather_id == 77 || weather_id == 94)
    {
        icon_id = 21;
    }
    else if(weather_id == 62 || weather_id == 74 || weather_id == 75 || weather_id == 76)
    {
        icon_id = 22;
    }
    else if(weather_id == 63)
    {
        icon_id = 23;
    }

    return  icon_id;
}

static esp_err_t app_weather_parse_now(char *buffer)
{
    cJSON *http_json = cJSON_Parse(buffer);
    cJSON *data_json = NULL, *daily_json = NULL, *now_json = NULL;

    if (NULL != http_json) 
    {
        if( (data_json = cJSON_GetObjectItem(http_json, "data")) != NULL )
        {
            if( (now_json = cJSON_GetObjectItem(data_json, "current")) != NULL )
            {
                uint8_t weather_id = cJSON_GetObjectItem(now_json, "weather_id")->valueint;
                weather_info.icon_code = lcd_icon_id_get(weather_id);
                weather_info.humid = cJSON_GetObjectItem(now_json, "humidity")->valueint;
                ESP_LOGI(TAG, "Icon : [%d]", weather_info.icon_code);
                ESP_LOGI(TAG, "Humid: [%d]", weather_info.humid);
            }
            if( (daily_json = cJSON_GetObjectItem(data_json, "daily")) != NULL )
            {
                if (cJSON_IsArray(daily_json)) 
                {
                    cJSON_GetArraySize(daily_json);
                    cJSON *daily_item = cJSON_GetArrayItem(daily_json, 1);

                    weather_info.temp_max = cJSON_GetObjectItem(daily_item, "temp_high")->valueint;
                    weather_info.temp_min = cJSON_GetObjectItem(daily_item, "temp_low")->valueint;
                    ESP_LOGI(TAG, "Temp : %d/%d", weather_info.temp_max, weather_info.temp_min);
                } else 
                {
                    ESP_LOGE(TAG, "Error parsing object - [%s] - [%d]", __FILE__, __LINE__);
                    return ESP_FAIL;
                }
            }
        }
        
        cJSON_Delete(http_json);
    } else {
        ESP_LOGE(TAG, "Error parsing object - [%s] - [%d]", __FILE__, __LINE__);
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t response_handler(esp_http_client_event_t *evt)
{
    static char *data = NULL;
    static int data_len = 0;

    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
        printf("HTTP_EVENT_ERROR\n");
        break;

    case HTTP_EVENT_ON_CONNECTED:
        data_len = 0;
        printf("HTTP_EVENT_ON_CONNECTED\n");
        break;

    case HTTP_EVENT_HEADER_SENT:
        printf("HTTP_EVENT_HEADER_SENT\n");
        break;

    case HTTP_EVENT_ON_HEADER:
        if (evt->data_len) {
            printf("HTTP_EVENT_ON_HEADER\n");
            printf("%.*s", evt->data_len, (char *)evt->user_data);
        }
        break;

    case HTTP_EVENT_ON_DATA:
        printf("HTTP_EVENT_ON_DATA (%d +)%d\n", data_len, evt->data_len);
        data = heap_caps_realloc(data, data_len + evt->data_len + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (data == NULL) 
        {
            printf("data realloc failed\n");
            free(data);
            data = NULL;
            break;
        }
        memcpy(data + data_len, (char *)evt->data, evt->data_len);
        data_len += evt->data_len;
        data[data_len] = '\0';
        break;

    case HTTP_EVENT_ON_FINISH:
        // size_t out_size = 0;
        // device.city_id = (int)evt->user_data;
        // size_t decode_maxlen = data_len*4;
        // char *decode_out = heap_caps_malloc(decode_maxlen, MALLOC_CAP_SPIRAM);
        // if (NULL == decode_out) {
        //     printf("Failed allocate mem\n");
        //     break;
        // }
        //network_gzip_decompress(data, data_len, decode_out, &out_size, decode_maxlen);
        //printf("HTTP_EVENT_ON_FINISH, out_size:%d, %s\n", data_len, data);
        app_weather_parse_now(data);

        // if (decode_out) {
        //     free(decode_out);
        //     decode_out = NULL;
        // }
        break;

    case HTTP_EVENT_DISCONNECTED:
        printf("HTTP_EVENT_DISCONNECTED\n");
        if (data) 
        {
            free(data);
            data = NULL;
        }
        break;

    default:
        break;
    }

    return ESP_OK;
}

void innotech_HttpRequest(esp_http_client_handle_t client, void *url_request) {
    esp_err_t err;
    int retry_count = 0;

    while (retry_count < MAX_RETRY) {
        err = esp_http_client_perform(client);
        if (err == ESP_OK) {
            break;
        } else {
            ESP_LOGW(TAG, "HTTP POST request failed: %s. Retrying in 1 second...\n", esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(1000)); 
            retry_count++;
        }
    }

    esp_http_client_cleanup(client);

    if (url_request) {
        heap_caps_free(url_request);
    }

    return ;
}

esp_err_t app_weather_request(void)
{
    char *url_request = heap_caps_malloc(200, MALLOC_CAP_SPIRAM);
    // message = password+timestamp+lat+lon;
    uint8_t input[100] = {0};
    uint8_t key[33] = {0};
    time_t timestamp = innotech_timestamp_get()*1000;

    sprintf((char *)input, "b216bdb3c22f2243e378967a6d87f7c1%lld%f%f", timestamp, device.latitude, device.longitude);
    md5_compute(input, strlen((char *)input), key);
    
    //sprintf(url_request, "https://devapi.qweather.com/v7/weather/now?location=%.2f,%.2f&lang=en&key=82cb41e828d14f4399a12ba5b485ce52", device.longitude, device.latitude);
    //sprintf(url_request, "https://devapi.qweather.com/v7/weather/3d?location=%.2f,%.2f&lang=en&key=82cb41e828d14f4399a12ba5b485ce52", device.longitude, device.latitude);
    sprintf(url_request, "http://coapi.moji.com/whapi/v2/weather?timestamp=%lld&token=321062d15a9f18d44e095da3fc7218ed&lat=%f&lon=%f&key=%s", timestamp, device.latitude, device.longitude, key);
    ESP_LOGI(TAG, "url_request:%s %d\r\n", url_request, strlen(url_request)+1);

    esp_http_client_config_t config = {
        .url = url_request,
        .method = HTTP_METHOD_GET,
        .event_handler = response_handler,
        .buffer_size = MAX_HTTP_RECV_BUFFER,
        .timeout_ms = 5000,
        .user_data = (void *)device.city_id,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    // Set the headers
    esp_http_client_handle_t client = esp_http_client_init(&config);
    // esp_http_client_set_header(client, "Content-Type", "application/json");
    // esp_http_client_set_header(client, "Host", WEATHER_SERVER);
    // esp_http_client_set_header(client, "User-Agent", "Innotech");
    // esp_http_client_set_header(client, "Accept-Encoding", "deflate");
    // esp_http_client_set_header(client, "Cache-Control", "no-cache");
    // esp_http_client_set_header(client, "Accept", "*/*");

    // Send the request
    innotech_HttpRequest(client,url_request);

    return ESP_OK;
}

void innotech_weather_check_handle(void *args)
{
    int sync_tick = 360;

    while(1)
    {
        if(location_flag == 1 && ++sync_tick >= 360)
        {
            sync_tick = 0;
            app_weather_request();
        }
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

void innotech_weather_init(void)
{
    if(xTaskCreate(&innotech_weather_check_handle, "weather_check_handle", 1024 * 4, NULL, 23, NULL) != pdTRUE)
    {
        //ESP_LOGI(TAG, "create device process task failed!!!");
    }
}