/******************** (C) COPYRIGHT 2024 INNOTECH **************************
* COMPANY:			INNOTECH
* DATE:				2023/02
* AUTHOR:			qiang.zhang
* IC:				ESP32C3
* DESCRIPTION:	    Device Electric Energy Manage.
*____________________________________________________________________________
* REVISION  Date		    User            Description
* 1.0		2023/02/28	    qiang.zhang		First release
*
*____________________________________________________________________________

*****************************************************************************/
#pragma once
#ifdef __cplusplus
extern "C"
{
#endif
#include "driver/gpio.h"
// Internal voltage reference value
#define V_REF_BL0               1.1

// The factor of a 1mOhm resistor
// as per recomended circuit in datasheet
// A 1mOhm resistor allows a ~30A max measurement
#define R_CURRENT           (0.002)

// This is the factor of a voltage divider of 6x 470K upstream and 1k downstream
// Sonoff Pow has 5x 470K
// Smart DGM outlet has 3x 680K
// as per recomended circuit in datasheet
#define R_VOLTAGE_BL0        ((2000) + 1) //1980 +1

// Frequency of the bl0937 internal clock
#define F_OSC_BL0           (1000000)

// Maximum pulse with in microseconds
// If longer than this pulse width is reset to 0
// This value is purely experimental.
// Higher values allow for a better precission but reduce sampling rate
// and response speed to change
// Lower values increase sampling rate but reduce precission
// Values below 0.5s are not recommended since current and voltage output
// will have no time to stabilise
#define PULSE_TIMEOUT       (1000000l)

// CF1 mode
typedef enum {
    MODE_CURRENT = 0,
    MODE_VOLTAGE
} bl0937_mode_t;


typedef struct {
    gpio_num_t sel_gpio;
    gpio_num_t cf1_gpio;
    gpio_num_t cf_gpio;
    uint8_t pin_mode;
    float sampling_resistor;
    float divider_resistor;
} chip_config_t;

void innotech_meter_init(void);
double innotech_current_get(void);
double innotech_voltage_get(void);
double innotech_power_get(void);
double innotech_consumption_get(void);
void innotech_meter_process(void);
float IRAM_ATTR bl0937_getCurrent();
float IRAM_ATTR bl0937_getActivePower();
void IRAM_ATTR bl0937_expectedActivePower(float value);
void IRAM_ATTR bl0937_expectedCurrent(float value);
void IRAM_ATTR bl0937_expectedVoltage(float value);
void bl0937_multiplier_init();
float IRAM_ATTR bl0937_getVoltage();
void IRAM_ATTR bl0937_checkCF1Signal();
void IRAM_ATTR bl0937_checkCFSignal();
float IRAM_ATTR bl0937_getEnergy();
bl0937_mode_t IRAM_ATTR bl0937_getMode();

bl0937_mode_t IRAM_ATTR bl0937_toggleMode();
void IRAM_ATTR bl0937_setMode(bl0937_mode_t mode);
int fix_power_factory(void);
int fix_vol_factory(void);
float fix_current_factory(void);
void innotech_overload_buzzer(void);
uint8_t inntech_buzzer_timer(uint8_t time);
uint8_t innotech_get_meter_protect(void);
uint8_t innotech_energy_check(void);
void innotech_clear_consume(void);
void innotech_protect_flag_clear(void);

#ifdef __cplusplus
}
#endif

