/******************** (C) COPYRIGHT 2024 INNOTECH **************************
* COMPANY:			INNOTECH
* DATE:				2024/02
* AUTHOR:			qiang.zhang
* IC:				ESP32C3
* DESCRIPTION:	    Device Factory Handle.
*____________________________________________________________________________
* REVISION  Date		    User            Description
* 1.0		2024/02/29	    qiang.zhang		First release
*
*____________________________________________________________________________

*****************************************************************************/
#pragma once
#ifdef __cplusplus
extern "C"
{
#endif
extern double fix_num;
extern double fix_vol_num;
extern double fix_cur_num;
extern uint8_t fix_flag;
extern uint8_t stop_flag;
extern uint8_t idx;
int power_tick_callback(void);
void innotech_factory_reset(void);
uint8_t innotech_fix_flag_get(void);
void innotech_factory_init(void);
bool innotech_factory_get(void);
void innotech_auto_flag_set(uint8_t flag);
uint8_t innotech_auto_flag_get();

#define start_200     1
#define success_200   2
#define fail_200      3
#define start_400     4
#define success_400   5


#ifdef __cplusplus
}
#endif

