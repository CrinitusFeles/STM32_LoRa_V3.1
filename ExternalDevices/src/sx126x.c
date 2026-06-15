#include "sx126x.h"
#include "sx126x_misc.h"
#include "string.h"

static uint8_t *DUMMY_PTR = 0;


uint8_t SX126x_Init(SX126x *driver){
    SX126x_SetStandby(driver, 0);
    SX126x_SetPacketType(driver, driver->base.config.mode);
    SX126x_SetDIO2AsRfSwitchCtr(driver, 1);
    SX126x_GetPacketType(driver);
    SX126x_SetRfFrequency(driver, driver->base.config.frequency);
    // SX126x_SetTxParams(driver, driver->base.config.power_dbm, driver->base.config.ramping_time);
    SX126x_SetModulationParams(
        driver, driver->base.config.spreadingFactor, driver->base.config.bandWidth,
        driver->base.config.coding_rate, driver->base.config.ldro
    );
    SX126x_GetDeviceErrors(driver);
    SX126x_SetDioIrqParams(driver, 0x3F, 1 << 1, 0, 0);
    // SX126x_SetOCP(driver,  driver->base.overCurrentProtection);
    uint8_t extra_tx_params[3] = {0x08, 0x89, 0x01};  // 0 if LoRa BW = 500kHz
    SX126x_WriteRegisters(driver, extra_tx_params, 3);
    SX126x_SetSyncWord(driver, driver->base.config.sync_word);
    SX126x_SetBufferBaseAddress(driver, 0, 0);
    return 0;
}

void SX126x_SendData(SX126x *driver, uint8_t *data, uint16_t data_len){
    SX126x_GetIrqStatus(driver);
    while(driver->irq_status.TxDone)
        SX126x_GetIrqStatus(driver);

    SX126x_SetStandby(driver, 0);
    uint16_t counter = 0;
    uint8_t chunk_size = 255;
    while(counter < data_len){
        if(data_len < chunk_size){
            chunk_size = (uint8_t)data_len;
        } else if(counter + chunk_size > data_len){
            chunk_size = data_len - counter;
        }
        if(driver->base.tx_data.buffer != data)
            memcpy(driver->base.tx_data.buffer, data + counter, chunk_size);
        SX126x_SetPacketParams(
            driver, driver->base.config.preamble, driver->base.config.implicit_header,
            chunk_size, driver->base.config.crc_enable, driver->base.config.iq_polarity
        );
        SX126x_WriteBuffer(driver, driver->base.tx_data.buffer, chunk_size);
        SX126x_GetIrqStatus(driver);
        driver->base.transmitting_progress = 1;
        SX126x_SetTx(driver, 0);
        SX126x_GetStatus(driver);
        while(!driver->irq_status.TxDone || driver->status.mode == SX126x_TX_Mode)
            SX126x_GetIrqStatus(driver);
        driver->base.transmitting_progress = 0;
        driver->base.tx_data.dlen = 0;
        SX126x_GetStatus(driver);
        driver->base.restart_watchdog();
        counter += chunk_size;
    }
    SX126x_SetRx(driver, 0);
    SX126x_ClearIrqStatus(driver, 0x01);
    SX126x_GetIrqStatus(driver);
    SX126x_GetStatus(driver);
}

void SX126x_CalculateMode(SX126x *driver, uint8_t data){
    driver->status.mode = (data & 0x70) >> 4;
    driver->status.cmd = (data & 0x0E) >> 1;
}

void SX126x_GetStatus(SX126x *driver){
    uint8_t data = 0;
    SX126x_SendOpcode(driver, OPCODE_GET_STATUS, DUMMY_PTR, 0, &data, 1);
    SX126x_CalculateMode(driver, data);
}

void SX126x_ReadRegisters(SX126x *driver, uint16_t address, uint8_t *answer, uint8_t len){
    uint8_t data[3] = {address >> 8, address & 0xFF, 0x00};
    SX126x_SendOpcode(driver, OPCODE_READ_REGISTER, data, 3, answer, len);
}

void SX126x_WriteRegisters(SX126x *driver, uint8_t *addr_and_regs, uint8_t len){
    SX126x_SendOpcode(driver, OPCODE_WRITE_REGISTER, addr_and_regs, len, DUMMY_PTR, 0);
}

void SX126x_WriteBuffer(SX126x *driver, uint8_t *data, uint8_t len){
    SX126x_SendOpcode(driver, OPCODE_WRITE_BUFFER, data, len, DUMMY_PTR, 0);
}
void SX126x_ReadBuffer(SX126x *driver, uint8_t offset, uint8_t *data, uint8_t len){
    uint8_t status[2] = {offset, 0};
    SX126x_SendOpcode(driver, OPCODE_READ_BUFFER, status, 2, data, len);
    driver->status.mode = status[1];
}

void SX126x_SetRfFrequency(SX126x *driver, uint32_t freq_hz){
    uint32_t RF_Freq = (uint32_t)((float)(freq_hz / 32000000.0) * (1 << 25) );  // 0x1b100000
    uint8_t data[4] = {RF_Freq >> 24, (RF_Freq >> 16) & 0xFF, (RF_Freq >> 8) & 0xFF, RF_Freq & 0xFF};  // [0x1b, 0x10, 0, 0]
    // uint8_t *reversed_data = (uint8_t *)(&RF_Freq);  // big endian [0, 0, 0x10, 0x1b]
    SX126x_SendOpcode(driver, OPCODE_SET_RF_FREQUENCY, data, 4, DUMMY_PTR, 0);
}

void SX126x_SetPacketType(SX126x *driver, uint8_t mode){
    SX126x_SendOpcode(driver, OPCODE_SET_PACKET_TYPE, &mode, 1, DUMMY_PTR, 0);
}

void SX126x_SetTxParams(SX126x *driver, uint8_t power_dbm, uint8_t ramp_time){
    uint8_t data[2] = {power_dbm, ramp_time};
    SX126x_SendOpcode(driver, OPCODE_SET_TX_PARAMS, data, 2, DUMMY_PTR, 0);
}

void SX126x_SetBufferBaseAddress(SX126x *driver, uint8_t tx_buf_addr, uint8_t rx_buf_addr){
    uint8_t data[2] = {tx_buf_addr, rx_buf_addr};
    SX126x_SendOpcode(driver, OPCODE_SET_BUFFER_BASE_ADDR, data, 2, DUMMY_PTR, 0);
}

void SX126x_SetModulationParams(SX126x *driver, uint8_t sf, uint8_t bw, uint8_t cr, uint8_t ldro){
    uint8_t data[4] = {sf, bw, cr, ldro};
    // uint8_t data[8] = {sf, bw, cr, ldro, 0, 0, 0, 0};
    SX126x_SendOpcode(driver, OPCODE_SET_MODULATION_PARAMS, data, 4, DUMMY_PTR, 0);
}

void SX126x_SetPacketParams(SX126x *driver, uint16_t preamble, uint8_t header_type,
                            uint8_t payload_len, uint8_t crc_type, uint8_t invert_iq){
    uint8_t data[6] = {
        (uint8_t)(preamble >> 8), (uint8_t)(preamble & 0xFF),
        header_type, payload_len, crc_type, invert_iq
    };
    // uint8_t data[9] = {(uint8_t)(preamble >> 8), (uint8_t)(preamble & 0xFF), header_type,
    //                    payload_len, crc_type, invert_iq, 0, 0, 0};
    SX126x_SendOpcode(driver, OPCODE_SET_PACKET_PARAMS, data, 6, DUMMY_PTR, 0);
}

void SX126x_SetTx(SX126x *driver, uint16_t timeout_ms){
    // max timeout 262000 ms
    uint32_t timeout = timeout_ms / 0.015625;
    uint8_t data[3] = {timeout >> 16, (timeout >> 8) & 0xFF, timeout & 0xFF};
    SX126x_SendOpcode(driver, OPCODE_SET_TX, data, 3, DUMMY_PTR, 0);
}

void SX126x_SetRx(SX126x *driver, uint16_t timeout_ms){
    // max timeout 262000 ms ; 0xFFFFFF - continuous mode
    SX126x_SetPacketParams(
        driver, driver->base.config.preamble, driver->base.config.implicit_header, 0,
        driver->base.config.crc_enable, driver->base.config.iq_polarity
    );
    uint32_t timeout = timeout_ms / 0.015625;
    uint8_t data[3] = {timeout >> 16, (timeout >> 8) & 0xFF, timeout & 0xFF};
    SX126x_SendOpcode(driver, OPCODE_SET_RX, data, 3, DUMMY_PTR, 0);
}

void SetRxTxFallbackMode(SX126x *driver){
    uint8_t mode = 0x20;  // STBY_RC
    SX126x_SendOpcode(driver, OPCODE_SET_FALLBACK_MODE, &mode, 1, DUMMY_PTR, 0);
}

void SX126x_SetStandby(SX126x *driver, uint8_t config){
    SX126x_SendOpcode(driver, OPCODE_SET_STANDBY, &config, 1, DUMMY_PTR, 0);
}

void SX126x_SetSleep(SX126x *driver){
    SX126x_SendOpcode(driver, OPCODE_SET_SLEEP, DUMMY_PTR, 0, DUMMY_PTR, 0);
}

uint8_t SX126x_GetPacketType(SX126x *driver){
    uint8_t buf[2] = {0};
    SX126x_SendOpcode(driver, OPCODE_GET_PACKET_TYPE, DUMMY_PTR, 0, buf, 2);
    SX126x_CalculateMode(driver, buf[0]);
    driver->base.config.mode = buf[1];
    return buf[1];
}

uint16_t SX126x_GetDeviceErrors(SX126x *driver){
    uint8_t buf[3] = {0};
    SX126x_SendOpcode(driver, OPCODE_GET_DEVICE_ERRORS, DUMMY_PTR, 0, buf, 3);
    SX126x_CalculateMode(driver, buf[0]);
    return (uint16_t)((buf[1] << 8) | buf[2]);
}

void SX126x_GetIrqStatus(SX126x *driver){
    uint8_t buf[3] = {0};
    SX126x_SendOpcode(driver, OPCODE_GET_IRQ_STATUS, DUMMY_PTR, 0, buf, 3);
    SX126x_CalculateMode(driver, buf[0]);
    driver->irq_status = *(SX126x_IRQ_Status*)((buf[1] << 8) | (buf[2] & 0x03));
}

uint8_t SX126x_IsTransmitting(SX126x *driver){
    SX126x_GetStatus(driver);
    if(driver->status.mode == SX126x_TX_Mode)
        return 1;
    SX126x_GetIrqStatus(driver);
    return driver->irq_status.TxDone;
}

void SX126x_SetDioIrqParams(SX126x *driver, uint16_t irq_mask, uint16_t dio1_mask, uint16_t dio2_mask, uint16_t dio3_mask){
    /*
    0 TxDone
    1 RxDone
    2 PreambleDetected
    3 SyncWordValid
    4 HeaderValid
    5 HeaderErr
    6 CrcErr
    7 CadDone
    8 CadDetected
    9 Timeout
    */

    uint8_t data[8] = {
        irq_mask >> 8, irq_mask & 0xFF, dio1_mask >> 8, dio1_mask & 0xFF,
        dio2_mask >> 8, dio2_mask & 0xFF, dio3_mask >> 8, dio3_mask & 0xFF
    };
    SX126x_SendOpcode(driver, OPCODE_SET_DIO_IRQ_PARAMS, data, 8, DUMMY_PTR, 0);
}

void SX126x_SetDIO2AsRfSwitchCtr(SX126x *driver, uint8_t enable){
    SX126x_SendOpcode(driver, OPCODE_SET_DIO2_AS_RFSWITCH, &enable, 1, DUMMY_PTR, 0);
}

void SX126x_SetOCP(SX126x *driver, uint8_t ocp){
    uint8_t data[3] = {0x08, 0xE7, ocp};
    SX126x_WriteRegisters(driver, data, 3);
}

void SX126x_SetSyncWord(SX126x *driver, uint16_t sync_word){
    uint8_t data[4] = {0x07, 0x40, sync_word >> 8, sync_word & 0xFF};
    SX126x_WriteRegisters(driver, data, 4);
}
void SX126x_GetRxBufferStatus(SX126x *driver){
    uint8_t buf[3] = {0};
    SX126x_SendOpcode(driver, OPCODE_GET_RX_BUFFER_STATUS, DUMMY_PTR, 0, buf, 3);
    SX126x_CalculateMode(driver, buf[0]);
    driver->base.rx_pkt_len = buf[1];
    driver->base.rx_buf_ptr = buf[2];
}
void SX126x_GetPacketStatus(SX126x *driver){
    uint8_t buf[4] = {0};
    SX126x_SendOpcode(driver, OPCODE_GET_PACKET_STATUS, DUMMY_PTR, 0, buf, 4);
    SX126x_CalculateMode(driver, buf[0]);
    driver->base.rssi = buf[1];
    driver->base.snr = buf[2];
    // driver->base.signal_rssi = buf[3];
}
void SX126x_ClearIrqStatus(SX126x *driver, uint16_t param){
    uint8_t data[2] = {param >> 8, param & 0xff};
    SX126x_SendOpcode(driver, OPCODE_CLEAR_IRQ_STATUS, data, 2, DUMMY_PTR, 0);
}


void SX126x_RxHandler(SX126x *driver){
    SX126x_GetIrqStatus(driver);
    memset(driver->base.rx_data.buffer, 0, 255);
    SX126x_ClearIrqStatus(driver, 0x3F);
    SX126x_GetIrqStatus(driver);
    SX126x_GetPacketStatus(driver);
    SX126x_GetRxBufferStatus(driver);
    SX126x_ReadBuffer(
        driver, driver->base.rx_buf_ptr, driver->base.rx_data.buffer, driver->base.rx_pkt_len
    );
    SX126x_SetRx(driver, 0);
    SX126x_GetStatus(driver);
    driver->base.new_rx_data_flag = 1;
}

