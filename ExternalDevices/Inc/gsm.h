#ifndef CODE_HEADER_GSM_H_
#define CODE_HEADER_GSM_H_

#include "stm32l4xx.h"
#include "uart.h"
#include "gpio.h"

typedef struct GSM_GPIO {
    GPIO_Pin rx;  // SIM_UART_RX  <-> MCU_UART_TX
    GPIO_Pin tx;  // SIM_UART_TX  <-> MCU_UART_RX
    GPIO_Pin rts;  // Request to send (Input)
    GPIO_Pin cts;  // Clear to Send (Output)
    /* The RI pin can be used to interrupt output signal to inform the host controller such as application
    CPU. Before that, users must use AT command “AT+CFGRI=1” to enable this function.
    Normally RI will keep high level until certain conditions such as receiving SMS, or a URC report
    coming, then it will output a low level pulse 120ms, in the end, it will become high level  (Output)*/
    GPIO_Pin ri;
    /* After setting the AT command “AT+CSCLK=1”, and then pulling up the DTR pin, SIM7000 will
    enter sleep mode when module is in idle mode. In sleep mode, the UART is unavailable. When
    SIM7000 enters sleep mode, pulling down DTR can wake up module.
    After setting the AT command “AT+CSCLK=0”, SIM7000 will do nothing when the DTR pin is
    pulling up. (Input)*/
    GPIO_Pin dtr;
    GPIO_Pin dcd;  // Carrier detects (Output)
    GPIO_Pin pwr;
    GPIO_Pin nreset;

    GPIO_Mode __tx_af_pin;
    GPIO_Mode __rx_af_pin;
    GPIO_Mode __cts_af_pin;
    GPIO_Mode __rts_af_pin;

} GSM_GPIO;
typedef enum GSM_IP_STATUS{
    GPRS_UNINITIALIZED,
    GPRS_INITIAL,
    GPRS_START,
    GPRS_GPRSACT,
    GPRS_STATUS,
    GPRS_CONNECT_OK,
    GPRS_CLOSED
} GSM_IP_STATUS;

typedef enum GSM_Mode{
    Automatic,
    GSM_only,
    LTE_only,
    GSM_and_LTE_only
} GSM_Mode;

typedef struct GSM_Status {
    uint8_t waiting_for_answer: 1;  // 0 - have got answer, 1 - wating for answer
    uint8_t gsm_reg_status: 3;          // 0 - GSM not connected, 1 - GSM connected
    uint8_t gprs_reg_status: 3;         // 0 - GPRS not connected, 1 - GPRS connected
    uint8_t gprs_connected: 1;
    uint8_t lte_reg_status: 3;
    uint8_t sim_status: 1;          // 0 - SIM not connected, 1 - SIM connected
    uint8_t pwr_status: 1;          // 0 - PWR OFF, 1 - PWR ON
    uint8_t buffer_filled: 1;       // 0 - buffer OK, 1 - buffer filled
    uint8_t timeout_event: 1;       // 0 - timeout OK, 1 - timeout occured
    uint8_t last_answer: 1;         // 0 - OK, 1 - ERROR
    uint8_t tcp_server_answer: 1;   // server receive data acknoledge
    uint8_t tcp_server_connected: 1;
} GSM_Status;

typedef struct GSM {
    GSM_GPIO gpio;
    USART_TypeDef *uart;
    char operator[10];
    uint16_t vbat;
    uint8_t signal_level;
    char ip[15];  // SIM7000 IP address assigned by the operator
    char rx_buf[64];  // echo must be disabled
    GSM_IP_STATUS ip_status;
    GSM_Status status;
    GSM_Mode mode;
    uint8_t overrun_counter;
    uint16_t tx_counter;
    uint16_t rx_counter;
    uint8_t frame_error_counter;
    uint8_t error_answer_counter;
    uint8_t sms_counter;
    uint8_t call_counter;
    uint8_t timeout_sec;
} GSM;

void GSM_RX_Handler();
uint8_t GSM_Init(GSM *driver);
uint8_t GSM_InitGPRS(GSM *driver);
void GSM_DisableEcho(GSM *driver);
void GSM_CheckSignal(GSM *driver);
void GSM_CheckGPRS(GSM *driver);
void GSM_CheckGSM(GSM *driver);
void GSM_CheckSIM(GSM *driver);
void GSM_SetAPN(GSM *driver, char *apn);
void GSM_SendCMD(GSM *driver, char *cmd);
uint16_t GSM_GetVBAT(GSM *driver);
void GSM_ActivateContext(GSM *driver);
void GSM_OpenConnection(GSM *driver, char *ip, char *port);
void GSM_CloseConnections(GSM *driver);
void GSM_CheckIPstatus(GSM *driver);
void GSM_TogglePower(GSM *driver);
void GSM_PowerOFF(GSM *driver);
void GSM_SendSMS(GSM *driver, char *data, char *phone_num);
void GSM_SendTCP(GSM *driver, char *data, uint16_t data_len);
void GSM_SaveSettings(GSM *driver);
uint8_t GSM_isAlive(GSM *driver);
uint8_t GSM_WaitTCPServerAcknowledge(GSM *driver, uint16_t timeout_ms);
uint8_t GSM_WaitTCPServerConnection(GSM *driver, uint16_t timeout_ms);


extern GSM sim7000g;



#endif