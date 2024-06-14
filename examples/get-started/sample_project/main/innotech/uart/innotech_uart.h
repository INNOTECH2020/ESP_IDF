#pragma once
#ifdef __cplusplus
extern "C"
{
#endif
/* Exported macro -----------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/
#define ECHO_TEST_TXD (43)
#define ECHO_TEST_RXD (44)
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)

#define ECHO_UART_PORT_NUM      (0)
#define ECHO_UART_BAUD_RATE     (115200)
#define ECHO_TASK_STACK_SIZE    (2048)

#define BUF_SIZE (1024)
/* Exported functions ------------------------------------------------------- */
void innotech_uart_init(void);
void innotech_uart_process(void);

#ifdef __cplusplus
}
#endif