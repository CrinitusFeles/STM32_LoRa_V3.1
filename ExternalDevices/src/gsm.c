#include "main.h"
#include "gsm.h"
#include "global_variables.h"
#include "string.h"
#include "stdlib.h"
#include "sx126x.h"
#include "xprintf.h"


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
    driver->delay_ms(5000);
}
void GSM_PowerOFF(GSM *driver){
    GSM_SendCMD(driver, "AT+CPOWD=1");
}
uint8_t GSM_InitGPRS(GSM *driver){
    GSM_CheckGPRS(driver);
    if(driver->status.gprs_connected && driver->status.gsm_reg_status){
        GSM_CheckIPstatus(driver);
        if(driver->ip_status == GPRS_INITIAL){
            GSM_SetAPN(driver, "internet.tele2.ru");
            driver->delay_ms(500);
            GSM_CheckIPstatus(driver);
            if(driver->ip_status == GPRS_START){
                GSM_SendCMD(driver, "AT+CIICR");
                driver->delay_ms(600);
                GSM_CheckIPstatus(driver);
                if(driver->ip_status == GPRS_GPRSACT){
                    GSM_SendCMD(driver, "AT+CIFSR");
                    driver->delay_ms(500);
                    GSM_CheckIPstatus(driver);
                    if(driver->ip_status == GPRS_STATUS){
                        return 0;  // можно открывать TCP соединение
                    }
                }
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

void GSM_OpenConnection(GSM *driver, char *ip, char *port){
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
        UART_tx_string(driver->uart, ip);
        UART_tx_string(driver->uart, "\",");
        UART_tx_string(driver->uart, port);
        UART_tx(driver->uart, '\r');
        driver->tx_counter++;
        driver->status.waiting_for_answer = 1;
        driver->status.tcp_server_answer = 0;
        GSM_wait_for_answer(driver, 2000);
        driver->delay_ms(250);
        GSM_CheckIPstatus(driver);
    }
}

void GSM_SendTCP(GSM *driver, char *data, uint16_t data_len){
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
    if(driver->ip_status == GPRS_CLOSED){
        GSM_SendCMD(driver, "AT+CIPSHUT");
    }
    GSM_CheckIPstatus(driver);
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

void GSM_AnswerParser(){
    sim7000g.status.waiting_for_answer = 0;
    // uint8_t parsed_flag = 0;
    if(strstr(sim7000g.rx_buf, "\nOK\r") != 0){
        sim7000g.status.last_answer = 0;
    }
    else if(strstr(sim7000g.rx_buf, "\nERROR\r") != 0){
        sim7000g.status.last_answer = 1;
        sim7000g.error_answer_counter++;
    }
    if(strstr(sim7000g.rx_buf, "STATE:") != 0){
        if(strstr(sim7000g.rx_buf, "IP INITIAL") != 0){
            sim7000g.ip_status = GPRS_INITIAL;
        }
        else if(strstr(sim7000g.rx_buf, "IP START") != 0){
            sim7000g.ip_status = GPRS_START;
        }
        else if(strstr(sim7000g.rx_buf, "IP GPRSACT") != 0){
            sim7000g.ip_status = GPRS_GPRSACT;
        }
        else if(strstr(sim7000g.rx_buf, "IP STATUS") != 0){
            sim7000g.ip_status = GPRS_STATUS;
        }
        else if(strstr(sim7000g.rx_buf, "TCP CLOSED") != 0){
             sim7000g.ip_status = GPRS_CLOSED;
        }
        else if(strstr(sim7000g.rx_buf, "TCP CONNECTING") != 0){
            sim7000g.ip_status = GPRS_CONNECT_OK;
        }
        else if(strstr(sim7000g.rx_buf, "CONNECT OK") != 0){
            sim7000g.ip_status = GPRS_CONNECT_OK;
        }
    }
    else if(strstr(sim7000g.rx_buf, "RDY\r\n") != 0){
        sim7000g.status.pwr_status = 1;
    }
    else if(strstr(sim7000g.rx_buf, "cmd") != 0){  // Custom answer from TCP server
        sim7000g.status.last_answer = 0;
        sim7000g.status.tcp_server_answer = 1;
        char *cmd_ptr = strstr(sim7000g.rx_buf, "#");
        if(cmd_ptr != 0){
            // CommandStruct *pkt = (CommandStruct *)(cmd_ptr + 1);
            // CMD_Parser(&SX1268, pkt);
        }
        // gpio_toggle(LED);
    }
    else if(strstr(sim7000g.rx_buf, "start") != 0){
        sim7000g.status.tcp_server_connected = 1;
    }
    else if(strstr(sim7000g.rx_buf, "POWER DOWN\r\n") != 0){
        sim7000g.status.pwr_status = 0;
    }
    else if(strstr(sim7000g.rx_buf, "+CPIN:") != 0){
        if(strstr(sim7000g.rx_buf, "+CPIN: READY") != 0) sim7000g.status.sim_status = 1;
        else sim7000g.status.sim_status = 0;
    }
    else if(strstr(sim7000g.rx_buf, "+CREG:") != 0){
        if(strstr(sim7000g.rx_buf, "+CREG: 0,1") != 0) sim7000g.status.gsm_reg_status = 1;
        else sim7000g.status.gsm_reg_status = 0;
    }
    else if(strstr(sim7000g.rx_buf, "+CGREG:") != 0){
        if(strstr(sim7000g.rx_buf, "+CGREG: 0,1") != 0) sim7000g.status.gprs_reg_status = 1;
        else sim7000g.status.gprs_reg_status = 0;
    }
    else if(strstr(sim7000g.rx_buf, "+CEREG:") != 0){
        if(strstr(sim7000g.rx_buf, "+CEREG: 0,1") != 0) sim7000g.status.lte_reg_status = 1;
        else sim7000g.status.lte_reg_status = 0;
    }
    else if(strstr(sim7000g.rx_buf, "+CGATT:") != 0){
        if(strstr(sim7000g.rx_buf, "+CGATT: 1") != 0) sim7000g.status.gprs_connected = 1;
        else sim7000g.status.gprs_connected = 0;
    }
    else if(strstr(sim7000g.rx_buf, "+PDP: DEACT") != 0){
        sim7000g.status.gprs_connected = 0;
    }
    else if(strstr(sim7000g.rx_buf, "+CNMP:") != 0){
        if(strstr(sim7000g.rx_buf, "+CNMP: 51") != 0) sim7000g.mode = GSM_and_LTE_only;
        else if(strstr(sim7000g.rx_buf, "+CNMP: 38") != 0) sim7000g.mode = LTE_only;
        else if(strstr(sim7000g.rx_buf, "+CNMP: 13") != 0) sim7000g.mode = GSM_only;
        else if(strstr(sim7000g.rx_buf, "+CNMP: 2") != 0) sim7000g.mode = Automatic;
    }
    else if(strstr(sim7000g.rx_buf, "+CSQ:") != 0){
        uint32_t bit_error = 0;
        xsprintf(sim7000g.rx_buf, "\r\n+CSQ: %d,%d", &sim7000g.signal_level, &bit_error);
    }
    else if(strstr(sim7000g.rx_buf, "CLOSE OK") != 0){
        sim7000g.ip_status = GPRS_INITIAL;
    }
    else if(strstr(sim7000g.rx_buf, "SEND OK") != 0){
        // если сервер быстро отвечает, то ответ приходит вместе с SEND OK и до сюда обработчик не доходит
        sim7000g.status.last_answer = 0;
    }
    else if(strstr(sim7000g.rx_buf, "+CBC:") != 0){
        uint32_t charger_status = 0;
        uint32_t percent = 0;
        uint32_t vbat = 0;
        xsprintf(sim7000g.rx_buf, "\r\n+CBC: %d,%d,%d", &charger_status, &percent, &vbat);
        sim7000g.vbat = (uint16_t)vbat;
    }
    sim7000g.rx_counter = 0;
    memset(sim7000g.rx_buf, 0, sizeof(sim7000g.rx_buf));
}

void GSM_RX_Handler(){
    if (sim7000g.uart->ISR & USART_ISR_RXNE) {
        sim7000g.rx_buf[sim7000g.rx_counter] = sim7000g.uart->RDR;
        sim7000g.rx_counter++;
        if(sim7000g.rx_counter >= sizeof(sim7000g.rx_buf)){
            sim7000g.rx_counter = 0;
            sim7000g.status.buffer_filled = 1;
        }
	}
    if(sim7000g.uart->ISR & USART_ISR_IDLE){
        sim7000g.uart->ICR |= USART_ICR_IDLECF;
        for(uint32_t i = 0; i < 40000; i++){
            if (sim7000g.uart->ISR & USART_ISR_RXNE) return;
        }
        GSM_AnswerParser(); // TODO: вызывать парсер после каждой функции GSM_wait_for_answer(), а не в прерывании
                            // тогда не нужно будет тупить десятки тысяч циклов в самом начале парсера
    }
	if(sim7000g.uart->ISR & USART_ISR_ORE){
        (void)sim7000g.uart->RDR;
        sim7000g.overrun_counter++;
		sim7000g.uart->ICR |= USART_ICR_ORECF;
		// UART_tx_array(USART1, "USART3 OVERRUN ERROR!\r\n");
	}
    if(sim7000g.uart->ISR & USART_ISR_FE){
        sim7000g.uart->ICR |= USART_ICR_FECF;
        sim7000g.status.pwr_status = 0;
        sim7000g.frame_error_counter++;
    }
}

