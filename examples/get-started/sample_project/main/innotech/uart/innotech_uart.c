#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"

#include "innotech_uart.h"
#include "innotech_factory.h"
#include "innotech_config.h"

static const char *TAG = "UART TEST";


const char* start_200W = "start 200W calibration\n";
const char* success_200W = "200W calibration successful\n";
const char* fail_200W = "200W calibration fail\n";
const char* start_400W = "start 400W overload test\n";
const char* overload_400W = "400W overload test successful\n";
static int uart_len = 0;
static uint8_t last_auto_flag = 0;
static uint8_t temp_auto_flag = 0;
static uint8_t uart_tick = 0;
static uint8_t get_status = 0;
char *data = NULL;
char *formatted_str = NULL;

extern char *innotech_get_output_buf(void);
void innotech_uart_init(void)
{
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = ECHO_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    int intr_alloc_flags = 0;

#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    ESP_ERROR_CHECK(uart_param_config(ECHO_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(ECHO_UART_PORT_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS));
    ESP_ERROR_CHECK(uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE , BUF_SIZE , 0, NULL, intr_alloc_flags));
    data = (char *) malloc(BUF_SIZE);
    
}

void innotech_uart_process(void)
{
    uart_tick ++;
    if(uart_tick > 5)
    {
        uart_tick = 0;
        char *output_buf = innotech_get_output_buf(); 
        int len = strlen(output_buf) + strlen(PT01_VERSION) + strlen("mac:\nver\n") + 1;
        formatted_str = malloc(len);
        snprintf(formatted_str, len, "mac:%s\nver:%s\n", output_buf, PT01_VERSION);

        ESP_ERROR_CHECK(uart_get_buffered_data_len(ECHO_UART_PORT_NUM, (size_t*)&uart_len));
        uart_len = uart_read_bytes(ECHO_UART_PORT_NUM, data, (BUF_SIZE - 1), 20 / portTICK_PERIOD_MS);
        
        if (uart_len) 
        {
            data[uart_len] = '\0';
            // ESP_LOGI(TAG, "data : %s\n",(char *) data);
            // uart_write_bytes(ECHO_UART_PORT_NUM, (const char*)test1_str, strlen(test1_str));
            if (strcmp("get_dev\n" , data) == 0) 
            {
                get_status = 1;
            }

            if(strcmp("get_state\n" , data) == 0) 
            {
                get_status = 2;
            }
        }

        if(get_status == 1)
        {
            // mac and version
            get_status = 0;
            uart_write_bytes(ECHO_UART_PORT_NUM, (const char*)formatted_str, strlen(formatted_str));
        }
        
        temp_auto_flag = innotech_auto_flag_get();
        if(last_auto_flag < temp_auto_flag)
        {
            last_auto_flag = temp_auto_flag;
        }
        if(get_status == 2)
        {
            get_status = 0;
            switch(last_auto_flag)
            {
                case 0:
                    break;
                case 1:
                    uart_write_bytes(ECHO_UART_PORT_NUM, start_200W, strlen(start_200W));
                    innotech_auto_flag_set(0);
                    break;
                case 2:
                    uart_write_bytes(ECHO_UART_PORT_NUM, success_200W, strlen(success_200W));
                    innotech_auto_flag_set(0);
                    break;
                case 3:
                    uart_write_bytes(ECHO_UART_PORT_NUM, fail_200W, strlen(fail_200W));
                    innotech_auto_flag_set(0);
                    break;
                case 4:
                    uart_write_bytes(ECHO_UART_PORT_NUM, start_400W, strlen(start_400W));
                    innotech_auto_flag_set(0);
                    break;
                case 5:
                    uart_write_bytes(ECHO_UART_PORT_NUM, overload_400W, strlen(overload_400W));
                    innotech_auto_flag_set(0);
                    break;
                default:
                    break;
            }
        }
    }
    
}











