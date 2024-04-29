#include "one_wire.h"
#include "uart.h"

// uint8_t check_flag(uint32_t flag){
//     uint8_t counter = 20000;
//     while((--counter) && flag);
//     if(counter == 0)
//         return 1;
//     return 0;
// }

OW_Status OW_SendBit(OneWire *ow, uint8_t data, uint8_t *rx_data){
    int16_t timeout = 1000;
    if(ow->uart->ISR & USART_ISR_ORE) ow->uart->ICR |= USART_ICR_ORECF;
    if(ow->uart->ISR & USART_ISR_FE) ow->uart->ICR |= USART_ICR_FECF;
    if(ow->uart->ISR & USART_ISR_NE) ow->uart->ICR |= USART_ICR_NECF;
	while(!(ow->uart->ISR & USART_ISR_TC) && --timeout);
    if(timeout <= 0) return OW_TIMEOUT;
	ow->uart->TDR = (data == 1) ? OneWire_bit_1 : data;
	while(!(ow->uart->ISR & USART_ISR_TC) && --timeout);
    if(timeout <= 0) return OW_TIMEOUT;
	while(!(ow->uart->ISR & USART_ISR_RXNE) && --timeout);
    if(timeout <= 0) return OW_TIMEOUT;
	*rx_data = ow->uart->RDR;
	return OW_OK;
}

OW_Status ReadBit(OneWire *ow, uint8_t *rx_data){
    OW_Status status = OW_SendBit(ow, OneWire_read_bit, rx_data);
    if(status != OW_OK) return status;
    *rx_data = (*rx_data == OneWire_bit_1) ? 1 : 0;
    return status;
}

OW_Status OW_Reset(OneWire *ow){
    UART_init(ow->uart, 9600, HALF_DUPLEX);
    uint8_t answer = 0;
    OW_Status status = OW_SendBit(ow, OneWire_reset_cmd, &answer);
    if(status != OW_OK) return status;
    UART_init(ow->uart, 115200, HALF_DUPLEX);
    if(answer == OneWire_reset_cmd) return OW_EMPTY_BUS;
    return OW_OK;
}

OW_Status OW_Read(OneWire *ow, uint8_t *rx_data){
    uint8_t bit_result = 0;
    OW_Status status = OW_OK;
    *rx_data = 0;
    for(uint8_t i = 0; i < 8; i++){
        status = ReadBit(ow, &bit_result);
        if(status != OW_OK) return status;
        *rx_data |= bit_result << i;
    }
    return status;
}

OW_Status OW_ReadArray(OneWire *ow, uint8_t *array, uint8_t length){
    OW_Status status = OW_OK;
    uint8_t val = 0;
    for(uint8_t i = 0; i < length; i++){
        status = OW_Read(ow, &val);
        array[i] = val;
        if(status != OW_OK) return status;
    }
    return status;
}

OW_Status OW_Write(OneWire *ow, uint8_t byte){
    uint8_t rx_buffer = 0;
    OW_Status status = OW_OK;
    for(uint8_t i = 0; i < 8; i++){
        // Полученное значение интерпретируем также: получили 0xFF - прочитали бит, равный 1.
        uint8_t next_bit = ((byte >> i) & 0x01);
        status = OW_SendBit(ow, next_bit, &rx_buffer);
        if(status != OW_OK) return status;
        rx_buffer |= ((rx_buffer == OneWire_bit_1) ? 1 : 0) << i;
    }
    return status;
}

OW_Status OW_WriteArray(OneWire *ow, uint8_t *array, uint8_t length){
    OW_Status status = OW_OK;
    for(uint8_t i = 0; i < length; i++){
        status = OW_Write(ow, array[i]);
        if(status != OW_OK) return status;
    }
    return status;
}

void InitStruct(OneWire *ow) {
    for (uint8_t i = 0; i < MAXDEVICES_ON_THE_BUS; i++) {
        uint8_t *r = (uint8_t *)(&ow->ids[i]);
        for (uint8_t j = 0; j < 8; j++)
            r[j] = 0;
    }
    for (uint8_t i = 0; i < 8; i++)
        ow->lastROM[i] = 0x00;
    ow->last_diff_bit_position = 64;

}


OW_Status hasNextRom(OneWire *ow, uint8_t *ROM, uint8_t *is_go_next) {
    OW_Status status = OW_Reset(ow);
    if (status != OW_OK) return status;
    status = OW_Write(ow, 0xF0);  // OneWire Search cmd
    if (status != OW_OK) return status;

    uint8_t bitNum = 0;
    int8_t zeroFork = -1;
    while(bitNum < OneWire_ROM_ID_lenth){
        uint8_t byteNum = bitNum >> 3;
        uint8_t *current_byte_in_rom = (ROM) + byteNum;
        uint8_t cB, cmp_cB, searchDirection = 0;
        status = ReadBit(ow, &cB);  // чтение прямого бита
        if (status != OW_OK) return status;
        status = ReadBit(ow, &cmp_cB);  // чтение комплементарного бита
        if (status != OW_OK) return status;
        if (cB != cmp_cB) searchDirection = cB;
        else if (cB == cmp_cB && cB == 0) {  // коллизия
            // в прошлой итерации выбрали левую ветку, а теперь при повторном проходе выбираем правую ветку
            if (bitNum == ow->last_diff_bit_position)
                searchDirection = 1;
            else if (bitNum > ow->last_diff_bit_position)  // в этой ветке еще не были, поэтому идем влево
                searchDirection = 0;
            else  // пока не дошли до новой развилки searchDirection = ROM[STEP]
                searchDirection = (uint8_t) ((ow->lastROM[byteNum] >> (bitNum & 0x07)) & 0x01);

            if (searchDirection == 0)  // запоминаем развилку, в которой поворачивали налево
                zeroFork = bitNum;
            // else повернули на развилке вправо
        }
        else return OW_ROM_FINDING_ERROR;  // empty bus or algorythm issue  cB == cmp_cB && cB == 1

        if (searchDirection)
            *(current_byte_in_rom) |= 1 << (bitNum & 0x07); // сохраняем бит
        uint8_t _tmp = 0;
        status = OW_SendBit(ow, searchDirection, &_tmp);
        if (status != OW_OK) return status;
        bitNum++;
    }
    ow->last_diff_bit_position = zeroFork;
    for (uint8_t i = 0; i < 7; i++){
        ow->lastROM[i] = ROM[i];
    }
    *is_go_next = ow->last_diff_bit_position > 0;
    return status;
}

uint8_t CRC8_ROM(uint8_t *data, uint8_t length){
    uint8_t checksum = 0;
    while (length--){
        uint8_t currentByte = *data++;
        for (uint8_t i = 8; i; i--){
            uint8_t temp = (checksum ^ currentByte) & 0x01;
            checksum >>= 1;
            if (temp) checksum ^= 0x8C;
            currentByte >>= 1;
        }
    }
    return checksum;
}


OW_Status OW_SearchDevices(OneWire *ow, uint8_t *sensors_amount) {
    OW_Status status = OW_OK;
    uint8_t nextROM = 1;
    uint8_t device_counter = *sensors_amount = 0;
    InitStruct(ow);
    while (nextROM && device_counter < MAXDEVICES_ON_THE_BUS){
        status = hasNextRom(ow, (uint8_t *)(&ow->ids[device_counter]), &nextROM);
        if (status != OW_OK) return status;
        ow->crc_status[device_counter] = CRC8_ROM((uint8_t *)(&ow->ids[device_counter]), 8);
        if (nextROM != 0)
            device_counter++;
        if(nextROM == 0 && device_counter == 0)
            return OW_EMPTY_BUS;  // no devices
    }
    *sensors_amount = device_counter;
    return status;
}

OW_Status OW_MatchRom(OneWire *ow, RomCode *rom){
    OW_Status status = OW_Reset(ow);
    if (status == OW_EMPTY_BUS) return status;
    status = OW_Write(ow, 0x55);  // Match ROM cmd
    if (status == OW_EMPTY_BUS) return status;
    return OW_WriteArray(ow, (uint8_t *)(rom), 8);;
}