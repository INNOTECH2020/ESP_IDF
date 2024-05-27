/******************** (C) COPYRIGHT 2021 INNOTECH **************************
* COMPANY:			INNOTECH
* DATE:				2023/12
* AUTHOR:			qiang.zhang
* IC:				ESP32S3
* DESCRIPTION:	    Lcd Control.
*____________________________________________________________________________
* REVISION  Date		    User            Description
* 1.0		2023/12/20	    qiang.zhang		First release
*
*____________________________________________________________________________

*****************************************************************************/

#include <inttypes.h>
#include "esp_timer.h"
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_io_additions.h"
#include "esp_io_expander_tca9554.h"
#include "innotech_config.h"
#include "innotech_factory.h"
#include "innotech_rtc.h"

#include "esp_lcd_st7701.h"
#include "lvgl.h"
#include "ui/ui.h"
#include "api_bridge.h"


#define TEST_LCD_H_RES              (480)
#define TEST_LCD_V_RES              (480)
#define TEST_LCD_BIT_PER_PIXEL      (18)
#define TEST_RGB_BIT_PER_PIXEL      (16)
#define TEST_RGB_DATA_WIDTH         (16)

#define MIN_LED_LUMI                7
#define TEST_LCD_IO_RGB_DISP        (GPIO_NUM_NC)
#define TEST_LCD_IO_RGB_VSYNC       (GPIO_NUM_47)
#define TEST_LCD_IO_RGB_HSYNC       (GPIO_NUM_48)
#define TEST_LCD_IO_RGB_DE          (GPIO_NUM_38)
#define TEST_LCD_IO_RGB_PCLK        (GPIO_NUM_21)

#define TEST_LCD_IO_RGB_DATA0       (GPIO_NUM_18) //R3
#define TEST_LCD_IO_RGB_DATA1       (GPIO_NUM_17) //R4
#define TEST_LCD_IO_RGB_DATA2       (GPIO_NUM_16) //R5
#define TEST_LCD_IO_RGB_DATA3       (GPIO_NUM_15) //R6
#define TEST_LCD_IO_RGB_DATA4       (GPIO_NUM_14) //R7
#define TEST_LCD_IO_RGB_DATA5       (GPIO_NUM_13) //G2
#define TEST_LCD_IO_RGB_DATA6       (GPIO_NUM_12) //G3
#define TEST_LCD_IO_RGB_DATA7       (GPIO_NUM_11) //G4
#define TEST_LCD_IO_RGB_DATA8       (GPIO_NUM_10) //G5
#define TEST_LCD_IO_RGB_DATA9       (GPIO_NUM_9) //G6
#define TEST_LCD_IO_RGB_DATA10      (GPIO_NUM_8) //G7
#define TEST_LCD_IO_RGB_DATA11      (GPIO_NUM_7) //B3
#define TEST_LCD_IO_RGB_DATA12      (GPIO_NUM_6) //B4
#define TEST_LCD_IO_RGB_DATA13      (GPIO_NUM_5) //B5
#define TEST_LCD_IO_RGB_DATA14      (GPIO_NUM_19)  //B6
#define TEST_LCD_IO_RGB_DATA15      (GPIO_NUM_20) //B7

#define TEST_LCD_IO_SPI_CS_1        (GPIO_NUM_46)
#define TEST_LCD_IO_SPI_SCL         (GPIO_NUM_13)
#define TEST_LCD_IO_SPI_SDA         (GPIO_NUM_14)
#define TEST_LCD_IO_RST             (GPIO_NUM_39)
#define TEST_LCD_IO_BL              (GPIO_NUM_1)

#define TEST_DELAY_TIME_MS              (3000)
#define EXAMPLE_LVGL_TICK_PERIOD_MS     2
#define EXAMPLE_LVGL_TASK_MAX_DELAY_MS  500
#define EXAMPLE_LVGL_TASK_MIN_DELAY_MS  1
#define EXAMPLE_LVGL_TASK_STACK_SIZE    (5 * 1024)
#define EXAMPLE_LVGL_TASK_PRIORITY      5

static char *TAG = "st7701_test";

static SemaphoreHandle_t    lvgl_mux    = NULL;
static TaskHandle_t lvgl_task_handle = NULL;

static uint8_t last_power_switch = 0;
static uint8_t power_switch_change = 1;
static uint8_t last_lcd_switch = 0;
static uint8_t first_power_on = 1;

IRAM_ATTR static bool rgb_lcd_on_vsync_event(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx)
{
    // esp_rom_printf(DRAM_STR("%d\r\n"), (int)esp_log_timestamp());

    BaseType_t need_yield = pdFALSE;
    xTaskNotifyFromISR(lvgl_task_handle, ULONG_MAX, eNoAction, &need_yield);
    return (need_yield == pdTRUE);
}

#if 0
static void example_lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    /* Copy a buffer's content to a specific area of the display */
    // ESP_LOGI(TAG, "first offsetx1 = %d offsetx2 = %d offsety1 = %d offsety2 = %d, color_map:%p\n", offsetx1, offsetx2, offsety1, offsety2, color_map);
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
    lv_disp_flush_ready(drv);
}
#else
typedef struct {
    uint16_t inv_p;
    uint8_t inv_area_joined[LV_INV_BUF_SIZE];
    lv_area_t inv_areas[LV_INV_BUF_SIZE];
} lv_port_dirty_area_t;

static lv_port_dirty_area_t dirty_area;

static void flush_dirty_save(lv_port_dirty_area_t *dirty_area)
{
    lv_disp_t *disp = _lv_refr_get_disp_refreshing();
    dirty_area->inv_p = disp->inv_p;
    for (int i = 0; i < disp->inv_p; i++) {
        dirty_area->inv_area_joined[i] = disp->inv_area_joined[i];
        dirty_area->inv_areas[i] = disp->inv_areas[i];
    }
}

typedef enum {
    FLUSH_STATUS_PART,
    FLUSH_STATUS_FULL
} lv_port_flush_status_t;

typedef enum {
    FLUSH_PROBE_PART_COPY,
    FLUSH_PROBE_SKIP_COPY,
    FLUSH_PROBE_FULL_COPY,
} lv_port_flush_probe_t;

/**
 * @brief Probe dirty area to copy
 *
 * @note This function is used to avoid tearing effect, and only work with LVGL direct-mode.
 *
 */
static lv_port_flush_probe_t flush_copy_probe(lv_disp_drv_t *drv)
{
    static lv_port_flush_status_t prev_status = FLUSH_PROBE_PART_COPY;
    lv_port_flush_status_t cur_status;
    uint8_t probe_result;
    lv_disp_t *disp_refr = _lv_refr_get_disp_refreshing();

    uint32_t flush_ver = 0;
    uint32_t flush_hor = 0;
    for (int i = 0; i < disp_refr->inv_p; i++) {
        if (disp_refr->inv_area_joined[i] == 0) {
            flush_ver = (disp_refr->inv_areas[i].y2 + 1 - disp_refr->inv_areas[i].y1);
            flush_hor = (disp_refr->inv_areas[i].x2 + 1 - disp_refr->inv_areas[i].x1);
            break;
        }
    }
    /* Check if the current full screen refreshes */
    cur_status = ((flush_ver == drv->ver_res) && (flush_hor == drv->hor_res)) ? (FLUSH_STATUS_FULL) : (FLUSH_STATUS_PART);

    if (prev_status == FLUSH_STATUS_FULL) {
        if ((cur_status == FLUSH_STATUS_PART)) {
            probe_result = FLUSH_PROBE_FULL_COPY;
        } else {
            probe_result = FLUSH_PROBE_SKIP_COPY;
        }
    } else {
        probe_result = FLUSH_PROBE_PART_COPY;
    }
    prev_status = cur_status;

    return probe_result;
}

static inline void *flush_get_next_buf(void *buf)
{
    lv_disp_t *disp = _lv_refr_get_disp_refreshing();
    lv_disp_draw_buf_t *draw_buf = disp->driver->draw_buf;
    return (buf == draw_buf->buf1) ? draw_buf->buf2 : draw_buf->buf1;
}

/**
 * @brief Copy dirty area
 *
 * @note This function is used to avoid tearing effect, and only work with LVGL direct-mode.
 *
 */
static void flush_dirty_copy(void *dst, void *src, lv_port_dirty_area_t *dirty_area)
{
    lv_coord_t x_start, x_end, y_start, y_end;
    uint32_t copy_bytes_per_line;
    uint32_t h_res = LV_HOR_RES;
    uint32_t bytes_per_line = h_res * 2;
    uint8_t *from, *to;
    for (int i = 0; i < dirty_area->inv_p; i++) {
        /* Refresh the unjoined areas*/
        if (dirty_area->inv_area_joined[i] == 0) {
            x_start = dirty_area->inv_areas[i].x1;
            x_end = dirty_area->inv_areas[i].x2 + 1;
            y_start = dirty_area->inv_areas[i].y1;
            y_end = dirty_area->inv_areas[i].y2 + 1;

            copy_bytes_per_line = (x_end - x_start) * 2;
            from = src + (y_start * h_res + x_start) * 2;
            to = dst + (y_start * h_res + x_start) * 2;
            for (int y = y_start; y < y_end; y++) {
                memcpy(to, from, copy_bytes_per_line);
                from += bytes_per_line;
                to += bytes_per_line;
            }
        }
    }
}

static void example_lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;
    const int offsetx1 = area->x1;
    const int offsetx2 = area->x2;
    const int offsety1 = area->y1;
    const int offsety2 = area->y2;

    // printf("[%d,%d]-[%d,%d]\r\n", area->x1, area->y1, area->x2, area->y2);

    lv_port_flush_probe_t probe_result;
    /* Action after last area refresh */
    if (lv_disp_flush_is_last(drv)) {
        /* Check if the `full_refresh` flag has been triggered */
        if (drv->full_refresh) {
            /* Reset flag */
            drv->full_refresh = 0;

            /* Switch the current RGB frame buffer to `color_map` */
            esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);

            /* Waiting for the last frame buffer to complete transmission */
            ulTaskNotifyValueClear(NULL, ULONG_MAX);
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

            /* Synchronously update the dirty area for another frame buffer */
            flush_dirty_copy(flush_get_next_buf(color_map), color_map, &dirty_area);
            drv->draw_buf->buf_act = (color_map == drv->draw_buf->buf1) ? drv->draw_buf->buf2 : drv->draw_buf->buf1;
        } else {
            /* Probe the copy method for the current dirty area */
            probe_result = flush_copy_probe(drv);

            if (probe_result == FLUSH_PROBE_FULL_COPY) {
                /* Save current dirty area for next frame buffer */
                flush_dirty_save(&dirty_area);

                /* Set LVGL full-refresh flag and set flush ready in advance */
                drv->full_refresh = 1;
                lv_disp_flush_ready(drv);

                /* Force to refresh whole screen, and will invoke `flush_callback` recursively */
                lv_refr_now(_lv_refr_get_disp_refreshing());
            } else {
                /* Switch the current RGB frame buffer to `color_map` */
                esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);

                /* Waiting for the last frame buffer to complete transmission */
                ulTaskNotifyValueClear(NULL, ULONG_MAX);
                ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

                if (probe_result == FLUSH_PROBE_PART_COPY) {
                    /* Synchronously update the dirty area for another frame buffer */
                    flush_dirty_save(&dirty_area);
                    flush_dirty_copy(flush_get_next_buf(color_map), color_map, &dirty_area);
                }
            }
        }
    }
    // printf("end [%d,%d]-[%d,%d]\r\n", area->x1, area->y1, area->x2, area->y2);

    lv_disp_flush_ready(drv);
}
#endif

static void example_increase_lvgl_tick(void *arg)
{
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

bool example_lvgl_lock(int timeout_ms)
{
    assert(lvgl_mux && "bsp_lvgl_port_init must be called first");

    const TickType_t timeout_ticks = (timeout_ms == 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(lvgl_mux, timeout_ticks) == pdTRUE;
}

void example_lvgl_unlock(void)
{
    assert(lvgl_mux && "bsp_lvgl_port_init must be called first");
    xSemaphoreGiveRecursive(lvgl_mux);
}

static void example_lvgl_port_task(void *arg)
{
    ESP_LOGI(TAG, "Starting LVGL task");

    uint32_t task_delay_ms = EXAMPLE_LVGL_TASK_MAX_DELAY_MS;
    while (1) {
        /* Lock the mutex due to the LVGL APIs are not thread-safe */
        example_lvgl_lock(0);
        task_delay_ms = lv_timer_handler();
        example_lvgl_unlock();
        if (task_delay_ms > EXAMPLE_LVGL_TASK_MAX_DELAY_MS) {
            task_delay_ms = EXAMPLE_LVGL_TASK_MAX_DELAY_MS;
        } else if (task_delay_ms < EXAMPLE_LVGL_TASK_MIN_DELAY_MS) {
            task_delay_ms = EXAMPLE_LVGL_TASK_MIN_DELAY_MS;
        }
        // ESP_LOGI(TAG, "task_delay_ms:%d", (int)task_delay_ms);
        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }
}

static const st7701_lcd_init_cmd_t vendor_specific_init_default[] = {
//  {cmd, { data }, data_size, delay_ms}
    {0xFF, (uint8_t []){0x77, 0x01, 0x00, 0x00, 0x13}, 5, 0},
    {0xEF, (uint8_t []){0x08}, 1, 0},
    {0xFF, (uint8_t []){0x77, 0x01, 0x00, 0x00, 0x10}, 5, 0},
    {0xC0, (uint8_t []){0x3B, 0x00}, 2, 0},
    {0xC1, (uint8_t []){0x10, 0x0C}, 2, 0},
    {0xC2, (uint8_t []){0x17, 0x0A}, 2, 0},
    //{0xC3, (uint8_t []){0x02}, 1, 0},
    {0xC7, (uint8_t []){0x04}, 1, 0},
    {0xCC, (uint8_t []){0x10}, 1, 0},
    //{0xCD, (uint8_t []){0x08}, 1, 0},
    {0xB0, (uint8_t []){0x40, 0x0E, 0x58, 0x0E, 0x12, 0x08, 0x0C, 0x09, 0x09, 0x27, 0x07, 0x18, 0x15, 0x78, 0x26, 0xC7}, 16, 0},
    {0xB1, (uint8_t []){0x40, 0x13, 0x5B, 0x0D, 0x11, 0x06, 0x0A, 0x08, 0x08, 0x26, 0x03, 0x13, 0x12, 0x79, 0x28, 0xC9}, 16, 0},
    {0xFF, (uint8_t []){0x77, 0x01, 0x00, 0x00, 0x11}, 5, 0},
    {0xB0, (uint8_t []){0x6D}, 1, 0},
    {0xB1, (uint8_t []){0x3E}, 1, 0},
    {0xB2, (uint8_t []){0x81}, 1, 0},
    {0xB3, (uint8_t []){0x80}, 1, 0},
    {0xB5, (uint8_t []){0x4E}, 1, 0},
    {0xB7, (uint8_t []){0x85}, 1, 0},
    {0xB8, (uint8_t []){0x20}, 1, 0},
    {0xC1, (uint8_t []){0x78}, 1, 0},
    {0xC2, (uint8_t []){0x78}, 1, 0},
    {0xD0, (uint8_t []){0x88}, 1, 0},
    {0xE0, (uint8_t []){0x00, 0x00, 0x02}, 3, 0},
    {0xE1, (uint8_t []){0x06, 0x30, 0x08, 0x30, 0x05, 0x30, 0x07, 0x30, 0x00, 0x33, 0x33}, 11, 0},
    {0xE2, (uint8_t []){0x11, 0x11, 0x33, 0x33, 0xF4, 0x00, 0x00, 0x00, 0xF4, 0x00, 0x00, 0x00}, 12, 0},
    {0xE3, (uint8_t []){0x00, 0x00, 0x11, 0x11}, 4, 0},
    {0xE4, (uint8_t []){0x44, 0x44}, 2, 0},
    {0xE5, (uint8_t []){0x0D, 0xF5, 0x30, 0xF0, 0x0F, 0xF7, 0x30, 0xF0, 0x09, 0xF1, 0x30, 0xF0, 0x0B, 0xF3, 0x30, 0xF0}, 16, 0},
    {0xE6, (uint8_t []){0x00, 0x00, 0x11, 0x11}, 4, 0},
    {0xE7, (uint8_t []){0x44, 0x44}, 2, 0},
    {0xE8, (uint8_t []){0x0C, 0xF4, 0x30, 0xF0, 0x0E, 0xF6, 0x30, 0xF0, 0x08, 0xF0, 0x30, 0xF0, 0x0A, 0xF2, 0x30, 0xF0}, 16, 0},
    {0xE9, (uint8_t []){0x36, 0x01}, 2, 0},
    {0xEB, (uint8_t []){0x00, 0x01, 0xE4, 0xE4, 0x44, 0x88, 0x40}, 7, 0},
    {0xED, (uint8_t []){0xFF, 0x45, 0x67, 0xFB, 0x01, 0x2A, 0xFC, 0xFF, 0xFF, 0xCF, 0xA2, 0x10, 0xBF, 0x76, 0x54, 0xFF}, 16, 0},
    // {0xFF, (uint8_t []){0x77, 0x01, 0x00, 0x00, 0x12}, 5, 0},
    // {0xd1, (uint8_t []){0x81}, 1, 0},
    // {0xd2, (uint8_t []){0x08}, 1, 0},
    {0xEF, (uint8_t []){0x10, 0x0D, 0x04, 0x08, 0x3F,0x1F}, 6, 0},
    {0x36, (uint8_t []){0x18}, 1, 0},
    // {0x36, (uint8_t []){0x00}, 1, 0},
    {0x3a, (uint8_t []){0x55}, 1, 0},
    {0x11, (uint8_t []){0x00}, 0, 120},
    {0x29, (uint8_t []){0x00}, 0, 0},
    {0x35, (uint8_t []){0x00}, 1, 0},
};
void spi_init(void)
{
    const gpio_config_t back_light_cfg = {
        // .intr_type = GPIO_INTR_POSEDGE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = BIT64(TEST_LCD_IO_BL),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&back_light_cfg);
    innotech_led_pwm_init();
    // innotech_led_pwm_write(50);
    
    // gpio_set_level(TEST_LCD_IO_BL, 1);

    static lv_disp_draw_buf_t   disp_buf;   /* Contains internal graphic buffer(s) called draw buffer(s) */
    static lv_disp_drv_t        disp_drv;   /* Contains callback functions */
    ESP_LOGI(TAG, "Install 3-wire SPI panel IO");
    spi_line_config_t line_config = {
        .cs_io_type = IO_TYPE_GPIO,
        .cs_gpio_num = TEST_LCD_IO_SPI_CS_1,
        .scl_io_type = IO_TYPE_GPIO,
        .scl_gpio_num = TEST_LCD_IO_SPI_SCL,
        .sda_io_type = IO_TYPE_GPIO,
        .sda_gpio_num = TEST_LCD_IO_SPI_SDA,
        .io_expander = NULL,
    };
    esp_lcd_panel_io_3wire_spi_config_t io_config = ST7701_PANEL_IO_3WIRE_SPI_CONFIG(line_config, 0);
    esp_lcd_panel_io_handle_t io_handle = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_3wire_spi(&io_config, &io_handle));

    ESP_LOGI(TAG, "Install ST7701 panel driver");
    esp_lcd_rgb_panel_config_t rgb_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .psram_trans_align = 64,
        .data_width = TEST_RGB_DATA_WIDTH,
        .bits_per_pixel = TEST_RGB_BIT_PER_PIXEL,
        .de_gpio_num = TEST_LCD_IO_RGB_DE,
        .pclk_gpio_num = TEST_LCD_IO_RGB_PCLK,
        .vsync_gpio_num = TEST_LCD_IO_RGB_VSYNC,
        .hsync_gpio_num = TEST_LCD_IO_RGB_HSYNC,
        .disp_gpio_num = TEST_LCD_IO_RGB_DISP,
        .data_gpio_nums = {
            TEST_LCD_IO_RGB_DATA0,
            TEST_LCD_IO_RGB_DATA1,
            TEST_LCD_IO_RGB_DATA2,
            TEST_LCD_IO_RGB_DATA3,
            TEST_LCD_IO_RGB_DATA4,
            TEST_LCD_IO_RGB_DATA5,
            TEST_LCD_IO_RGB_DATA6,
            TEST_LCD_IO_RGB_DATA7,
            TEST_LCD_IO_RGB_DATA8,
            TEST_LCD_IO_RGB_DATA9,
            TEST_LCD_IO_RGB_DATA10,
            TEST_LCD_IO_RGB_DATA11,
            TEST_LCD_IO_RGB_DATA12,
            TEST_LCD_IO_RGB_DATA13,
            TEST_LCD_IO_RGB_DATA14,
            TEST_LCD_IO_RGB_DATA15,
        },
        .timings = ST7701_480_480_PANEL_60HZ_RGB_TIMING(),
        .flags.fb_in_psram = 1,
        .num_fbs = 2,
        .bounce_buffer_size_px = TEST_LCD_H_RES * 20,
    };
    st7701_vendor_config_t vendor_config = {
        .rgb_config = &rgb_config,
        .init_cmds = vendor_specific_init_default,
        .init_cmds_size = sizeof(vendor_specific_init_default) / sizeof(st7701_lcd_init_cmd_t),
        .flags = {
            .auto_del_panel_io = 1,
        },
    };
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = TEST_LCD_IO_RST,
        // .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = TEST_LCD_BIT_PER_PIXEL,
        .vendor_config = &vendor_config,
    };
    esp_lcd_panel_handle_t panel_handle = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7701(io_handle, &panel_config, &panel_handle));
    // ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    esp_lcd_rgb_panel_event_callbacks_t cbs = {
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 1, 2) && CONFIG_BSP_LCD_RGB_BOUNCE_BUFFER_MODE
        .on_bounce_frame_finish = rgb_lcd_on_vsync_event,
#else
        .on_vsync = rgb_lcd_on_vsync_event,
#endif
    };
    esp_lcd_rgb_panel_register_event_callbacks(panel_handle, &cbs, NULL);
    /* =============== Initialize LVGL =============== */
    ESP_LOGI(TAG, "Initialize LVGL");

    lv_init();
    void *buf1 = NULL;
    void *buf2 = NULL;
    int buffer_size = 0;

    buffer_size = TEST_LCD_H_RES * TEST_LCD_V_RES;
    esp_lcd_rgb_panel_get_frame_buffer(panel_handle, 2, &buf1, &buf2);
    ESP_LOGI(TAG, "buf1@%p, buf2@%p", buf1, buf2);

    /* Initialize LVGL draw buffers */
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, buffer_size);

    /* =============== Register display driver to LVGL =============== */
    ESP_LOGI(TAG, "Register display driver to LVGL");

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res    = TEST_LCD_H_RES;
    disp_drv.ver_res    = TEST_LCD_V_RES;
    disp_drv.flush_cb   = example_lvgl_flush_cb;
    disp_drv.draw_buf   = &disp_buf;
    disp_drv.user_data  = panel_handle;
    // disp_drv.full_refresh = 1;
    disp_drv.direct_mode = 1;

    lv_disp_t *disp     = lv_disp_drv_register(&disp_drv);

    /* =============== Register touch driver to LVGL =============== */
    ESP_LOGI(TAG, "Install LVGL tick timer");

    /* Tick interface for LVGL (using esp_timer to generate 2ms periodic event) */
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback   = &example_increase_lvgl_tick,
        .name       = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));

    //test_draw_bitmap(panel_handle);
    // vTaskDelay(pdMS_TO_TICKS(TEST_DELAY_TIME_MS));

    // ESP_ERROR_CHECK(esp_lcd_panel_io_del(io_handle));
    // ESP_ERROR_CHECK(esp_lcd_panel_del(panel_handle));
}

void lvgl_init(void)
{
    
}

// static void monitor_task(void *arg)
// {
//     (void) arg;

//     while (true) {
//         ESP_LOGI(TAG, "System Info Trace");
//         // printf("\tDescription\tInternal\tSPIRAM\n");
//         printf("Current Free Memory\t%d\t\t%d\n",
//                heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL),
//                heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
//         printf("Largest Free Block\t%d\t\t%d\n",
//                heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL),
//                heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
//         printf("Min. Ever Free Size\t%d\t\t%d\n",
//                heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL),
//                heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM));

//         // esp_intr_dump(stdout);
//         vTaskDelay(pdMS_TO_TICKS(5 * 1000));
//     }

//     vTaskDelete(NULL);
// }

// static void sys_monitor_start(void)
// {
//     BaseType_t ret_val = xTaskCreatePinnedToCore(monitor_task, "Monitor Task", 4 * 1024, NULL, configMAX_PRIORITIES - 3, NULL, 0);
//     ESP_ERROR_CHECK_WITHOUT_ABORT((pdPASS == ret_val) ? ESP_OK : ESP_FAIL);
// }

void innotech_brightness_switch_set(void)
{
    struct tm time_info = innotech_time_get();
    switch (time_info.tm_hour) {
        case 0:
        case 1:
        case 2:
            innotech_led_pwm_write(1);
            break;
        case 3:
        case 23:
            innotech_led_pwm_write(10);
            break;
        case 4:
        case 22:
            innotech_led_pwm_write(20);
            break;
        case 5:
        case 21:
            innotech_led_pwm_write(30);
            break;
        case 6:
        case 20:
            innotech_led_pwm_write(40);
            break;
        case 7:
        case 19:
            innotech_led_pwm_write(50);
            break;
        case 8:
        case 18:
            innotech_led_pwm_write(60);
            break;
        case 9:
        case 17:
            innotech_led_pwm_write(70);
            break;
        case 10:
        case 16:
            innotech_led_pwm_write(80);
            break;
        case 11:
        case 15:
            innotech_led_pwm_write(90);
            break;
        case 12:
        case 13:
        case 14:
            innotech_led_pwm_write(100);
            break;
    }
}
void innotech_power_switch_change_clear(void)
{
    power_switch_change = 0;
}

void innotech_lcd_process(void)
{ 
    innotech_config_t *innotech_config = (innotech_config_t *)innotech_config_get_handle();
    if(innotech_config->power_switch == 1 && last_power_switch == 0)
    {
        last_power_switch = 1;
        power_switch_change = 0;
        first_power_on = 0;
    }
    else if(innotech_config->power_switch == 0 && last_power_switch == 1)
    {
        last_power_switch = 0;
        power_switch_change = 1;
    }
    
    if(last_lcd_switch != innotech_config->lcd_switch && first_power_on == 0)
    {
        last_lcd_switch = innotech_config->lcd_switch;
        power_switch_change = 0;
    }

    if(innotech_config->lcd_switch == 0)
    {
        innotech_led_pwm_write(0);
    }
    else if(innotech_config->power_switch == 0 && power_switch_change == 1)
    {
        innotech_led_pwm_write(0);
    }
    else if(innotech_config->brightness_switch == 0)
    {
        innotech_led_pwm_write(innotech_config->lcd_brightness);
    }
    else if(innotech_config->brightness_switch == 1)
    {
        innotech_brightness_switch_set();
    }
}

void innotech_lcd_init(void)
{
    
    //lvgl_init();

    /* =============== Create LVGL Task =============== */
    ESP_LOGI(TAG, "Run LVGL Task");

    lvgl_mux = xSemaphoreCreateRecursiveMutex();
    assert(lvgl_mux);
    xTaskCreatePinnedToCore(example_lvgl_port_task, "LVGL", EXAMPLE_LVGL_TASK_STACK_SIZE, NULL, EXAMPLE_LVGL_TASK_PRIORITY, &lvgl_task_handle, 1);
    
    spi_init();
    
    ESP_LOGI(TAG, "Display LVGL Scatter Chart");
    //Lock the mutex due to the LVGL APIs are not thread-safe
    example_lvgl_lock(0);
    ui_init();

    example_lvgl_unlock();

    //sys_monitor_start();
}

void innotech_lcd_pre_init(void)
{
    lvgl_mux = xSemaphoreCreateRecursiveMutex();
    assert(lvgl_mux);
    xTaskCreatePinnedToCore(example_lvgl_port_task, "LVGL", EXAMPLE_LVGL_TASK_STACK_SIZE, NULL, EXAMPLE_LVGL_TASK_PRIORITY, &lvgl_task_handle, 1);
    
    spi_init();
    ESP_LOGI(TAG, "Display LVGL Scatter Chart");
    //Lock the mutex due to the LVGL APIs are not thread-safe
    innotech_led_pwm_write(100);
    example_lvgl_lock(0);
    lv_disp_t * dispp = lv_disp_get_default();
    lv_theme_t * theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
                                               true, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    ui_Screen11_screen_init();
    ui____initial_actions0 = lv_obj_create(NULL);
    
    lv_disp_load_scr(ui_Screen11);

    example_lvgl_unlock();
}

