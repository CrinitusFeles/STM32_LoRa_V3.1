#include "one_wire.h"
#include "uart.h"

// uint8_t check_flag(uint32_t flag){
//     uint8_t counter = 20000;
//     while((--counter) && flag);
//     if(counter == 0)
//         return 1;
//     return 0;
// }

uint8_t OneWire_SendBit(OneWire *ow, uint8_t data){
    if(ow->uart->ISR & USART_ISR_ORE) ow->uart->ICR |= USART_ICR_ORECF;
    if(ow->uart->ISR & USART_ISR_FE) ow->uart->ICR |= USART_ICR_FECF;
    if(ow->uart->ISR & USART_ISR_NE) ow->uart->ICR |= USART_ICR_NECF;
	while(!(ow->uart->ISR & USART_ISR_TC));
	ow->uart->TDR = (data == 1) ? OneWire_bit_1 : data;
	while(!(ow->uart->ISR & USART_ISR_TC));
	while(!(ow->uart->ISR & USART_ISR_RXNE));
	uint8_t rx_data = ow->uart->RDR;
	return rx_data;
}

uint8_t OneWire_ReadBit(OneWire *ow){
    return OneWire_SendBit(ow, OneWire_read_bit) == OneWire_bit_1 ? 1 : 0;
}

OneWireStatus OneWire_Reset(OneWire *ow){
    UART_init(ow->uart, 9600, HALF_DUPLEX);
    uint8_t answer = OneWire_SendBit(ow, OneWire_reset_cmd);
    UART_init(ow->uart, 115200, HALF_DUPLEX);
    if(answer == OneWire_reset_cmd) return ONE_WIRE_EMPTY_BUS;
    return ONE_WIRE_OK;
}

uint8_t OneWire_Read(OneWire *ow){
    uint8_t buffer = 0;
    for(uint8_t i = 0; i < 8; i++){
        buffer |= OneWire_ReadBit(ow) << i;
    }
    return buffer;
}

void OneWire_ReadArray(OneWire *ow, uint8_t *array, uint8_t length){
    for(uint8_t i = 0; i < length; i++){
        array[i] = OneWire_Read(ow);
    }
}

uint8_t OneWire_Write(OneWire *ow, uint8_t byte){
    uint8_t buffer = 0;
    for(uint8_t i = 0; i < 8; i++){
        // Полученное значение интерпретируем также: получили 0xFF - прочитали бит, равный 1.
        uint8_t next_bit = ((byte >> i) & 0x01);
        buffer |= (OneWire_SendBit(ow, next_bit) == OneWire_bit_1 ? 1 : 0) << i;
    }
    return buffer;
}

void OneWire_WriteArray(OneWire *ow, uint8_t *array, uint8_t length){
    for(uint8_t i = 0; i < length; i++){
        OneWire_Write(ow, array[i]);
    }
}

void OneWire_InitStruct(OneWire *ow) {
    for (uint8_t i = 0; i < MAXDEVICES_ON_THE_BUS; i++) {
        uint8_t *r = (uint8_t *)(&ow->ids[i]);
        for (uint8_t j = 0; j < 8; j++)
            r[j] = 0;
    }
    for (uint8_t i; i < 8; i++)
        ow->lastROM[i] = 0x00;
    ow->last_diff_bit_position = 64;

}
/*
 * return 1 if has got one more address
 * return 0 if hasn't
 * return -1 if error reading happened
 */
int8_t hasNextRom(OneWire *ow, uint8_t *ROM) {
    if (OneWire_Reset(ow) == ONE_WIRE_EMPTY_BUS) return 0;
    OneWire_Write(ow, 0xF0);  // OneWire Search cmd

    uint8_t bitNum = 0;
    int8_t zeroFork = -1;
    while(bitNum < OneWire_ROM_ID_lenth){
        uint8_t byteNum = bitNum >> 3;
        uint8_t *current_byte_in_rom = (ROM) + byteNum;
        uint8_t cB, cmp_cB, searchDirection = 0;
        cB = OneWire_ReadBit(ow);  // чтение прямого бита
        cmp_cB = OneWire_ReadBit(ow);  // чтение комплементарного бита
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
        else return -1;  // empty bus or algorythm issue  cB == cmp_cB && cB == 1

        if (searchDirection)
            *(current_byte_in_rom) |= 1 << (bitNum & 0x07); // сохраняем бит
        OneWire_SendBit(ow, searchDirection);
        bitNum++;
    }
    ow->last_diff_bit_position = zeroFork;
    for (uint8_t i = 0; i < 7; i++)
        ow->lastROM[i] = ROM[i];
    return ow->last_diff_bit_position > 0;
}

uint8_t OneWire_CRC8_ROM(uint8_t *data, uint8_t length){
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

// Возвращает количество устройств на шине или код ошибки, если значение меньше 0
int OneWire_SearchDevices(OneWire *ow) {
    int8_t device_counter = 0, nextROM = 1;
    OneWire_InitStruct(ow);
    while (nextROM && device_counter < MAXDEVICES_ON_THE_BUS){
        nextROM = hasNextRom(ow, (uint8_t *)(&ow->ids[device_counter]));
        ow->crc_status[device_counter] = OneWire_CRC8_ROM((uint8_t *)(&ow->ids[device_counter]), 8);
        if (nextROM < 0)
            return -1;
        if (nextROM != 0)
            device_counter++;
        if(nextROM == 0 && device_counter == 0)
            return 0;

    }
    return device_counter;
}

OneWireStatus OneWire_MatchRom(OneWire *ow, RomCode *rom){
    OneWireStatus status = OneWire_Reset(ow);
    if (status == ONE_WIRE_EMPTY_BUS) return status;
    OneWire_Write(ow, 0x55);  // Match ROM cmd
    OneWire_WriteArray(ow, (uint8_t *)(rom), 8);
    return status;
}