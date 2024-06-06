/******************** (C) COPYRIGHT 2021 INNOTECH **************************
* COMPANY:			INNOTECH
* DATE:				2021/08
* AUTHOR:			qiang.zhang
* IC:				AT32F415KBU7-4
* DESCRIPTION:	    Device Service Handle.
*____________________________________________________________________________
* REVISION  Date		    User            Description
* 1.0		2021/08/24	    qiang.zhang		First release
*
*____________________________________________________________________________

*****************************************************************************/
#pragma once
#ifdef __cplusplus
extern "C"
{
#endif

/* Exported constants --------------------------------------------------------*/
typedef void (*module_start)(void* args);
typedef void (*module_stop)(void* args);
typedef void (*module_update)(void* args);
typedef void (*module_process)(void* args);

typedef struct
{
    unsigned char id;
    module_start start;
    module_stop stop;
    module_update update;
    module_process process;
    void* args;
} device_module_t;

/* Exported macro -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
void innotech_update_save_tick(void);
int innotech_device_module_register(device_module_t* pt_module);
void innotech_device_service_start(void);
void innotech_device_update_state(void);
void innotech_device_service_stop(void);
void innotech_device_init(void);
void innotech_set_ble_tick(void);

#ifdef __cplusplus
}
#endif

