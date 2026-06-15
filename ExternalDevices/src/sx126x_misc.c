#include "sx126x_misc.h"
#include "sx126x.h"
#include "spi.h"
#include "main.h"

// static inline void SX126x_wait_busy(){
// }

void SX126x_SendOpcode(SX126x *driver, uint8_t opcode, uint8_t *parameters,
                       uint8_t param_len, uint8_t *answer, uint8_t answer_len){
    uint16_t timeout = 10000;
    while(gpio_read(LoRa_BUSY) && timeout--);
    if(timeout == 0){
        driver->base.busy_issues = 1;
    }
    gpio_state(LoRa_NSS, LOW);
    // SX126x_CalculateMode(driver, spi_txrx(driver->spi, opcode));
    spi_txrx(driver->base.spi, opcode);
    if(opcode == OPCODE_WRITE_BUFFER)
        spi_txrx(driver->base.spi, 0);
    if(param_len > 0){
        for(uint8_t i = 0; i < param_len; i++){
            spi_txrx(driver->base.spi, parameters[i]);
        }
    }
    if(answer_len > 0){
        for(uint8_t i = 0; i < answer_len; i++)
            answer[i] = (uint8_t)(spi_txrx(driver->base.spi, 0x00));
    }
    gpio_state(LoRa_NSS, HIGH);
}