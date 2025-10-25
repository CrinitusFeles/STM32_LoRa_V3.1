#include "main.h"
#include "gsm.h"
#include "string.h"
#include "stdlib.h"
#include "sx126x.h"
#include "xprintf.h"
#include "periph_handlers.h"
#include "system_config.h"


uint8_t GSM_Init(GSM *driver){
    // gpio_init(driver->gpio.tx, driver->gpio.__tx_af_pin, Push_pull, no_pull, Very_high_speed);
    // gpio_init(driver->gpio.rx, driver->gpio.__rx_af_pin, Open_drain, no_pull, Input);
    // gpio_init(driver->gpio.pwr, General_output, Push_pull, pull_up, Low_speed);
    // UART_init(driver->uart, 9600, FULL_DUPLEX);

    driver->uart->CR1 |= USART_CR1_IDLEIE;
    driver->delay_ms(100);
    if(sim7000g.frame_error_counter > 0)
        GSM_TogglePower(&sim7000g);
    driver->delay_ms(10000);
    memset(sim7000g.rx_buf, 0, sizeof(sim7000g.rx_buf));
    if(GSM_isAlive(driver) != 0){
        driver->delay_ms(1000);
        GSM_isAlive(driver);
    }
    if(driver->status.pwr_status){
        driver->delay_ms(500);
        GSM_CheckSIM(driver);
        driver->delay_ms(500);
        GSM_CheckGSM(driver);
        driver->delay_ms(200);
        GSM_DisableEcho(driver);
        if(driver->status.gsm_reg_status && driver->status.sim_status)
            return 0;
    }
    return 1;
}
void GSM_wait_for_answer(GSM *driver, int32_t timeout_ms){
    while((timeout_ms--) && (driver->status.waiting_for_answer))
        driver->delay_ms(1);
    if(timeout_ms == 0) driver->status.timeout_event = 1;
}

void GSM_SendCMD(GSM *driver, char *cmd){
    UART_tx_string(driver->uart, cmd);
    UART_tx(driver->uart, '\r');
    driver->tx_counter++;
    driver->status.waiting_for_answer = 1;
    GSM_wait_for_answer(driver, 2000);
}

void GSM_SetAPN(GSM *driver, char *apn){
    // char *cmd = "AT+CSTT=\"";
    // size_t apn_len = strlen(apn);
    // size_t cmd_len = strlen(cmd);
    // char *buf = malloc(apn_len + cmd_len + 2); // один дополнительный байт для \", а второй для \0
    // memcpy(buf, cmd, cmd_len);
    // memcpy(buf + cmd_len, apn, apn_len);
    // memcpy(buf + cmd_len + apn_len, "\"", 2);
    // GSM_SendCMD(driver, buf);
    // free(buf);
    UART_tx_string(driver->uart, "AT+CSTT=\"");
    UART_tx_string(driver->uart, apn);
    UART_tx_string(driver->uart, "\"\r");
    driver->tx_counter++;
    driver->status.waiting_for_answer = 1;
    GSM_wait_for_answer(driver, 2000);
}

void GSM_CheckSignal(GSM *driver){
    GSM_SendCMD(driver, "AT+CSQ");
}

uint16_t GSM_GetVBAT(GSM *driver){
    GSM_SendCMD(driver, "AT+CBC");
    return driver->vbat;
}
uint8_t GSM_isAlive(GSM *driver){
    GSM_SendCMD(driver, "AT");
    if(driver->status.timeout_event){
        driver->status.pwr_status = 0;
        return 1;
    }
    if(driver->status.last_answer == 0)
        driver->status.pwr_status = 1;
    return 0;
}
void GSM_TogglePower(GSM *driver){
    gpio_state(driver->gpio.pwr, LOW);
    driver->delay_ms(900);
    gpio_state(driver->gpio.pwr, HIGH);
    // driver->delay_ms(5000);
}
void GSM_PowerOFF(GSM *driver){
    GSM_SendCMD(driver, "AT+CPOWD=1");
}
uint8_t GSM_InitGPRS(GSM *driver){
    GSM_CheckGPRS(driver);
    if(driver->status.gprs_connected && driver->status.gsm_reg_status){
        GSM_CheckIPstatus(driver);
        driver->delay_ms(500);
        for (uint8_t i = 0; i < 4; i++){
            switch(driver->ip_status){
                case(GPRS_INITIAL):
                    GSM_SetAPN(driver, system_config.apn);  // AT+CSTT=internet.tele2.ru
                    GSM_CheckIPstatus(driver);
                    driver->delay_ms(500);
                    break;
                case(GPRS_START):
                    GSM_SendCMD(driver, "AT+CIICR");
                    GSM_CheckIPstatus(driver);
                    driver->delay_ms(600);
                    break;
                case(GPRS_GPRSACT):
                    GSM_SendCMD(driver, "AT+CIFSR");
                    GSM_CheckIPstatus(driver);
                    driver->delay_ms(600);
                    break;
                case(GPRS_STATUS):
                    return 0;  // можно открывать TCP соединение
                // case(GPRS_CLOSED):
                //     return 0;
                default:
                    return 1;
            }
        }
    }
    return 1;
}

void GSM_SendSMS(GSM *driver, char *data, char *phone_num){
    GSM_SendCMD(driver, "AT+CMGF=1");
    char *cmd = "AT+CMGS=\"";
    size_t cmd_len = strlen(cmd);
    size_t phone_len = strlen(phone_num);

    char *buf = malloc(phone_len + cmd_len + 2);
    memcpy(buf, cmd, cmd_len);
    memcpy(buf + cmd_len, phone_num, phone_len);
    memcpy(buf + cmd_len + phone_len, "\"", 2);
    GSM_SendCMD(driver, buf);
    UART_tx_string(driver->uart, data);
    char end[2] = {0x1A, 0x00};
    GSM_SendCMD(driver, end);
    free(buf);
}

void GSM_OpenConnection(GSM *driver, const char *ip, const char *port){
    if(driver->ip_status == GPRS_STATUS){
        // char *cmd = "AT+CIPSTART=\"TCP\",\"";
        // size_t ip_len = strlen(ip);
        // size_t port_len = strlen(port);
        // size_t cmd_len = strlen(cmd);
        // char *buf = malloc(cmd_len + ip_len + port_len + 2);  // "AT+CIPSTART=\"TCP\",\"10.6.1.4\",80");
        // memcpy(buf, cmd, cmd_len);
        // memcpy(buf + cmd_len, ip, ip_len);
        // memcpy(buf + cmd_len + ip_len, "\"", 1);
        // memcpy(buf + cmd_len + ip_len + 1, port, port_len + 1);
        // GSM_SendCMD(driver, buf);
        // free(buf);
        UART_tx_string(driver->uart, "AT+CIPSTART=\"TCP\",\"");
        UART_tx_string(driver->uart, (char*)ip);
        UART_tx_string(driver->uart, "\",");
        UART_tx_string(driver->uart, (char*)(port));
        UART_tx(driver->uart, '\r');
        driver->tx_counter++;
        driver->status.waiting_for_answer = 1;
        driver->status.tcp_server_answer = 0;
        GSM_wait_for_answer(driver, 2000);
        driver->delay_ms(250);
        GSM_CheckIPstatus(driver);
    }
}

void GSM_SendTCP(GSM *driver, const char *data, uint16_t data_len){
    UART_tx_string(driver->uart, "AT+CIPSEND");
    UART_tx(driver->uart, '\r');
    driver->delay_ms(500);
    for(uint16_t i = 0; i < data_len; i++){
        UART_tx(driver->uart, data[i]);
    }
    UART_tx(driver->uart, 0x1A);
    UART_tx(driver->uart, '\r');
    GSM_wait_for_answer(driver, 2000);
}

void GSM_SetDNS(GSM *driver){
    GSM_SendCMD(driver, "AT+CDNSCFG=\"8.8.8.8\",\"8.8.4.4\"");
}

void GSM_CloseConnections(GSM *driver){
    GSM_SendCMD(driver, "AT+CIPCLOSE");
    driver->delay_ms(50);
    GSM_CheckIPstatus(driver);
    driver->delay_ms(200);
    GSM_SendCMD(driver, "AT+CIPSHUT");
    GSM_CheckIPstatus(driver);
    driver->delay_ms(200);
    GSM_SendCMD(driver, "AT+CGATT=0");
    GSM_CheckIPstatus(driver);
    driver->delay_ms(500);
    driver->status.gprs_connected = 0;
    driver->status.tcp_server_answer = 0;
    driver->status.tcp_server_connected = 0;
}
uint8_t GSM_WaitTCPServerAcknowledge(GSM *driver, uint16_t timeout_ms){
    uint16_t timeout = timeout_ms;
    while(!driver->status.tcp_server_answer && timeout > 0) {
        driver->delay_ms(1);
        timeout--;
    }
    if(timeout == 0) return 1;
    return 0;
}
uint8_t GSM_WaitTCPServerConnection(GSM *driver, uint16_t timeout_ms){
    uint16_t timeout = timeout_ms;
    while(!driver->status.tcp_server_connected && timeout > 0) {
        driver->delay_ms(1);
        timeout--;
    }
    if(timeout == 0) return 1;
    return 0;
}
// void GSM_SetTime(GSM *driver, char *time_data){
//     char *buf[20] = {"AT+CCLK="};
//     GSM_SendCMD(driver, buf);
// }
void GSM_DisableEcho(GSM *driver){
    GSM_SendCMD(driver, "ATE0");
}
void GSM_SaveSettings(GSM *driver){
    GSM_SendCMD(driver, "AT&W");
}
void GSM_CheckSIM(GSM *driver){
    GSM_SendCMD(driver, "AT+CPIN?");
}

void GSM_CheckGSM(GSM *driver){
    GSM_SendCMD(driver, "AT+CREG?");
}

void GSM_CheckGPRS(GSM *driver){
    GSM_SendCMD(driver, "AT+CGREG?");
    driver->delay_ms(300);
    GSM_SendCMD(driver, "AT+CGATT?");
    if(!driver->status.gprs_connected){
        GSM_SendCMD(driver, "AT+CGATT=1");
        driver->delay_ms(500);
        GSM_SendCMD(driver, "AT+CGATT?");
        driver->delay_ms(200);
    }
}

void GSM_CheckIPstatus(GSM *driver){
    GSM_SendCMD(driver, "AT+CIPSTATUS");
}

void GSM_ActivateContext(GSM *driver){
    GSM_SendCMD(driver, "AT+CIICR");
}
void GSM_RequestIP(GSM *driver){
    GSM_SendCMD(driver, "AT+CIFSR");
}

void GSM_SetMode(GSM *driver){
    GSM_SendCMD(driver, "AT+CNMP=");
}

void GSM_CheckMode(GSM *driver){
    GSM_SendCMD(driver, "AT+CNMP?");
}

#define GSM_SHUT_OK                 2802156163          //"SHUT OK",
#define GSM_HINT                    5861987          //"CONNECT OK",
#define GSM_PDP_DEACT               3440200933      //"STATE: PDP DEACT",
#define GSM_CONNECT_OK              1292179753      //"CONNECT_OK",
#define GSM_EMPTY_APN               3557573663      //"+CSTT: "CMNET","",""",
#define GSM_ATI                     1103507907      //"SIM7000G R1529",
#define GSM_APN                     3518297380      //"+CSTT: "internet.tele2.ru","",""",
#define GSM_ate0                    2090089199      //"ate0",
#define GSM_ATE0                    2088903311      //"ATE0",
#define GSM_OK                      5862591         //"OK",
#define GSM_ERROR                   219019599       //"ERROR",
#define GSM_RDY                     193468628       //"RDY",
#define GSM_SMS_READY               202236397       //"SMS Ready",
#define GSM_CFUN                    4189516231      //"+CFUN: 1",
#define GSM_POWER_DOWN              1969610611      //"NORMAL POWER DOWN",
#define GSM_CPIN                    2968058089      //"+CPIN: READY",
#define GSM_CREG                    1985516344      //"+CREG: 0,1",
#define GSM_CGREG                   3314961631      //"+CGREG: 0,1",
#define GSM_CGREG2                  1985516345      //"+CGREG: 0,2",
#define GSM_CGATT                   1331919950      //"+CGATT: 1"",
#define GSM_CGATT0                  1331919949      //"+CGATT: 0"",
#define GSM_PDP                     456551535       //"+PDP: DEACT"",
#define GSM_CNMP_LTE_GSM            2246180190      //"+CNMP: 51"",
#define GSM_CNMP_LTE                2246180131      //"+CNMP: 38"",
#define GSM_CNMP_GSM                2246180060      //"+CNMP: 13"",
#define GSM_CNMP_AUTO               198216586       //"+CNMP: 2"",
#define GSM_SENDED                  2675873545      //"SEND OK"",
#define GSM_CLOSED                  3917737589      //"CLOSE OK"",
#define GSM_STATE_INITIAL           3886319939      //"STATE: IP INITIAL",
#define GSM_STATE_START             1526169639      //"STATE: IP START",
#define GSM_STATE_GPRSACT           1392283501      //"STATE: IP GPRSACT",
#define GSM_STATE_STATUS            3118960125      //"STATE: IP STATUS",
#define GSM_STATE_TCP_CLOSED        144060321       //"STATE: TCP CLOSED",
#define GSM_STATE_TCP_CONNECTING    2984550063      //"STATE: TCP CONNECTING",
#define GSM_STATE_TCP_CONNECT_OK    1392283501      //"STATE: TCP CONNECT OK"


uint32_t gsm_hash(const char *str) {
    uint32_t hash = 5381;
    char c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    return hash;
}


void GSM_AnswerParser(){
    switch (gsm_hash(sim7000g.rx_buf))
    {
    case GSM_OK:
        sim7000g.status.waiting_for_answer = 0;
        sim7000g.status.last_answer = 0;
        sim7000g.error_answer_counter++;
        break;
    case GSM_ERROR:
        sim7000g.status.waiting_for_answer = 0;
        sim7000g.status.last_answer = 1;
        sim7000g.error_answer_counter++;
        break;
    case GSM_RDY:
        sim7000g.status.pwr_status = 1;
        break;
    case GSM_SMS_READY:
        sim7000g.status.sim_status = 1;
        break;
    case GSM_POWER_DOWN:
        sim7000g.status.waiting_for_answer = 0;
        break;
    case GSM_EMPTY_APN:
        // sim7000g.ip_status = GPRS_INITIAL;
        break;
    case GSM_APN:
        // sim7000g.ip_status = GPRS_START;
        break;
    case GSM_CGREG:

        break;
    case GSM_CONNECT_OK:

        break;
    case GSM_HINT:

        break;
    case GSM_ate0:

        break;
    case GSM_ATI:
        break;
    case GSM_ATE0:

        break;
    case GSM_CFUN:

        break;
    case GSM_CPIN:
        sim7000g.status.sim_status = 1;
        break;
    case GSM_CREG:
        sim7000g.status.gsm_reg_status = 1;
        break;
    case GSM_CGREG2:
        break;
    case GSM_CGATT:
        sim7000g.status.gprs_connected = 1;
        break;
    case GSM_CGATT0:
        sim7000g.status.gprs_connected = 0;
        break;

    case GSM_PDP:
        sim7000g.status.gprs_connected = 0;
        break;

    case GSM_CNMP_LTE_GSM:
        sim7000g.mode = GSM_and_LTE_only;
        break;

    case GSM_CNMP_LTE:
        sim7000g.mode = LTE_only;
        break;

    case GSM_CNMP_GSM:
        sim7000g.mode = GSM_only;
        break;
    case GSM_CNMP_AUTO:
        sim7000g.mode = Automatic;
        break;
    case GSM_SENDED:
        sim7000g.status.waiting_for_answer = 0;
        break;
    case GSM_CLOSED:

        break;
    case GSM_SHUT_OK:

        break;
    case GSM_STATE_INITIAL:
        sim7000g.ip_status = GPRS_INITIAL;
        break;
    case GSM_STATE_START:
        sim7000g.ip_status = GPRS_START;
        break;
    case GSM_STATE_GPRSACT:
        sim7000g.ip_status = GPRS_GPRSACT;
        break;
    case GSM_STATE_STATUS:
        sim7000g.ip_status = GPRS_STATUS;
        break;
    case GSM_STATE_TCP_CLOSED:
        sim7000g.ip_status = GPRS_CLOSED;
        break;
    case GSM_PDP_DEACT:
        sim7000g.ip_status = GPRS_INITIAL;
        break;
    case GSM_STATE_TCP_CONNECTING:
        sim7000g.ip_status = GPRS_CONNECT_OK;
        break;

    default:
        if(strstr(sim7000g.rx_buf, "+CBC:") != 0){
            uint32_t charger_status = 0;
            uint32_t percent = 0;
            uint32_t vbat = 0;
            xsprintf(sim7000g.rx_buf, "\r\n+CBC: %d,%d,%d", &charger_status, &percent, &vbat);
            sim7000g.vbat = (uint16_t)vbat;
        }
        else if(strstr(sim7000g.rx_buf, "+CSQ:") != 0){
            uint32_t bit_error = 0;
            xsprintf(sim7000g.rx_buf, "+CSQ: %d,%d", &sim7000g.signal_level, &bit_error);
        }
        else {
            xprintf("Unknown answer\n");
        }
        break;
    }
    memset(sim7000g.rx_buf, 0, sizeof(sim7000g.rx_buf));
    sim7000g.rx_counter = 0;
}


