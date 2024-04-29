#include "sx127x.h"
#include "sx127x_misc.h"
#include <math.h>

#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

uint16_t Calc_TOA(uint8_t payload_size, uint8_t preamble_size,
                  uint8_t sf, float bw_kHz, uint8_t cr,
                  uint8_t explicit_header, uint8_t crc, uint8_t ldro) {
    float symbol_time_ms = (1 << sf) / bw_kHz;
    float preamble_time_ms = (preamble_size + 4.25) * symbol_time_ms;
    float tmp_poly = MAX(ceil(8 * payload_size - 4 * sf + 28 + 16 * crc - 20 * explicit_header), 0);
    float payload_symbol_nb = 8 + (tmp_poly / (4 * (sf - 2 * ldro))) * (4 + cr);  // Сколько символов занимает основная часть в пакете.
    return (uint16_t)(payload_symbol_nb * symbol_time_ms + preamble_time_ms);  // payload time + preamble_time
}

void LoRa_gotoMode(LoRa* _LoRa, uint8_t mode){
	volatile uint8_t read;
	volatile uint8_t data;

	read = LoRa_readRegister(_LoRa, RegOpMode);
	data = (read & 0xF8);

	if(mode == SLEEP_MODE){
		data |= 0x00;
		_LoRa->current_mode = SLEEP_MODE;
		LoRa_writeRegister(_LoRa, RegOpMode, 0x08);
		return;
	}else if (mode == STNBY_MODE){
		data |= 0x01;
		_LoRa->current_mode = STNBY_MODE;
	}else if (mode == TRANSMIT_MODE){
		data |= 0x03;
		_LoRa->current_mode = TRANSMIT_MODE;
	}else if (mode == RXCONTIN_MODE){
		data |= 0x05;
		_LoRa->current_mode = RXCONTIN_MODE;
	}else if (mode == RXSINGLE_MODE){
		data |= 0x06;
		_LoRa->current_mode = RXSINGLE_MODE;
	}

	LoRa_writeRegister(_LoRa, RegOpMode, data);
}


void LoRa_setFrequency(LoRa* _LoRa, uint32_t freq_hz){
	volatile uint8_t  data;
	volatile uint32_t F;
    float freq_step = 0.016384;
	// F = (uint32_t)(freq_hz / 32000000 * 524288);
	F = (uint32_t)(freq_hz * freq_step);

	// write Msb:
	data = (uint8_t) (F >> 16);
	LoRa_writeRegister(_LoRa, RegFrMsb, data);
	LoRa_Delay(_LoRa, 5);

	// write Mid:
	data = (uint8_t) (F >> 8);
	LoRa_writeRegister(_LoRa, RegFrMid, data);
	LoRa_Delay(_LoRa, 5);

	// write Lsb:
	data = (uint8_t) (F >> 0);
	LoRa_writeRegister(_LoRa, RegFrLsb, data);
	LoRa_Delay(_LoRa, 5);
}


void LoRa_setSpreadingFactor(LoRa* _LoRa, uint8_t SF){
	uint8_t	data;
	uint8_t	read;

	if(SF>12)
		SF = 12;
	if(SF<7)
		SF = 7;

	read = LoRa_readRegister(_LoRa, RegModemConfig2);
	LoRa_Delay(_LoRa, 10);

	data = (SF << 4) + (read & 0x0F);
	LoRa_writeRegister(_LoRa, RegModemConfig2, data);
	LoRa_Delay(_LoRa, 10);
}

void LoRa_setPower(LoRa* _LoRa, uint8_t power){
	LoRa_writeRegister(_LoRa, RegPaConfig, power);
	LoRa_Delay(_LoRa, 10);
}

void LoRa_setOCP(LoRa* _LoRa, uint8_t current){
	uint8_t	OcpTrim = 0;

	if(current < 45)
		current = 45;
	if(current > 240)
		current = 240;

	if(current <= 120)
		OcpTrim = (current - 45) / 5;
	else if(current <= 240)
		OcpTrim = (current + 30) / 10;

	OcpTrim = OcpTrim + (1 << 5);
	LoRa_writeRegister(_LoRa, RegOcp, OcpTrim);
	LoRa_Delay(_LoRa, 10);
}

void LoRa_setTOMsb_setCRCon(LoRa* _LoRa){
	uint8_t read, data;

	read = LoRa_readRegister(_LoRa, RegModemConfig2);

	data = read | 0x07;
	LoRa_writeRegister(_LoRa, RegModemConfig2, data);
	LoRa_Delay(_LoRa, 10);
}

void LoRa_set_LDRO(LoRa* _LoRa, uint8_t ldro){
	uint8_t read, data;

	read = LoRa_readRegister(_LoRa, RegModemConfig3);

	data = (read & 0x08) | ((ldro & 0x01) << 3);
	LoRa_writeRegister(_LoRa, RegModemConfig3, data);
	LoRa_Delay(_LoRa, 10);
}

uint8_t LoRa_transmit(LoRa* _LoRa, uint8_t *data, uint8_t length){
	volatile uint8_t read;
	// int mode = _LoRa->current_mode;
	LoRa_gotoMode(_LoRa, STNBY_MODE);
	LoRa_writeRegister(_LoRa, RegIrqFlags, 0xFF);
	read = LoRa_readRegister(_LoRa, RegOpMode);
	read = LoRa_readRegister(_LoRa, RegFiFoTxBaseAddr);
	LoRa_writeRegister(_LoRa, RegFiFoAddPtr, read);
	LoRa_writeRegister(_LoRa, RegPayloadLength, length);
	LoRa_writeRegisters(_LoRa, RegFiFo, data, length);
	read = LoRa_readRegister(_LoRa, RegOpMode);
	LoRa_gotoMode(_LoRa, TRANSMIT_MODE);
	// модуль ненадолго переходит в режим отправки, после чего возвращается в режим ожидания или приема.
	// Поэтому нет смысла пытаться прочитать из регистра режим отправки.
    uint16_t delay = Calc_TOA(length, 8, 10, 125, 1, 1, 1, 1);
    LoRa_Delay(_LoRa, delay);
    LoRa_gotoMode(_LoRa, RXCONTIN_MODE);
	return 1;
}


void LoRa_startReceiving(LoRa* _LoRa){
	LoRa_gotoMode(_LoRa, RXCONTIN_MODE);
}


uint8_t LoRa_receive(LoRa* _LoRa, uint8_t* data, uint8_t length){
	volatile uint8_t read = 0;
	volatile uint8_t number_of_bytes = 0;
	volatile uint8_t min = 0;
	// volatile uint8_t stat = 0;

	for(uint8_t i = 0; i < length; i++){
		data[i] = 0;
    }
	LoRa_gotoMode(_LoRa, STNBY_MODE);
	// stat = LoRa_readRegister(_LoRa, RegOpMode);
	read = LoRa_readRegister(_LoRa, RegIrqFlags);
	if((read & LORA_IRQ_RX_DONE) != 0){
		LoRa_writeRegister(_LoRa, RegIrqFlags, 0xFF);
		number_of_bytes = LoRa_readRegister(_LoRa, RegRxNbBytes);
		read = LoRa_readRegister(_LoRa, RegFiFoRxCurrentAddr);
		LoRa_writeRegister(_LoRa, RegFiFoAddPtr, read);
		min = length >= number_of_bytes ? number_of_bytes : length;
		for(uint8_t i = 0; i < min; i++){
			data[i] = LoRa_readRegister(_LoRa, RegFiFo);
        }
	}
	LoRa_writeRegister(_LoRa, RegIrqFlags, 0xFF);
	LoRa_gotoMode(_LoRa, RXCONTIN_MODE);
    return min;
}

int LoRa_getRSSI(LoRa* _LoRa){
	volatile uint8_t read;
	read = LoRa_readRegister(_LoRa, RegPktRssiValue);
	return -164 + read;
}

uint16_t LoRa_init(LoRa* _LoRa){
	volatile uint8_t    data;
	volatile uint8_t    read;
	for(uint8_t i = 0; i < 4; i++){
		read = LoRa_readRegister(_LoRa, RegOpMode);
		if((read & 0x07) != 0x00){
			LoRa_reset(_LoRa);
			LoRa_Delay(_LoRa, 50);
			LoRa_gotoMode(_LoRa, SLEEP_MODE);
			LoRa_Delay(_LoRa, 50);
		}
		else{
			break;
		}
		if(i == 3){
			while(1){
				LoRa_Delay(_LoRa, 1);
			}
		}
	}
	LoRa_Delay(_LoRa, 10);

// turn on lora mode:
	read = LoRa_readRegister(_LoRa, RegOpMode);
	LoRa_writeRegister(_LoRa, RegOpMode, 0x88);
	LoRa_Delay(_LoRa, 50);
	read = LoRa_readRegister(_LoRa, RegOpMode);
	if(read != 0x88){
		LoRa_writeRegister(_LoRa, RegOpMode, 0x88);
		read = LoRa_readRegister(_LoRa, RegOpMode);
		if(read != 0x88){
			return LORA_UNAVAILABLE;
		}
	}
// set frequency:
	LoRa_setFrequency(_LoRa, _LoRa->freq_mhz);

// set output power gain:
	LoRa_setPower(_LoRa, _LoRa->power);

// set over current protection:
	LoRa_setOCP(_LoRa, _LoRa->overCurrentProtection);

// set LNA gain:
	LoRa_writeRegister(_LoRa, RegLna, 0x23);

// set spreading factor, CRC on, and Timeout Msb:
	LoRa_setTOMsb_setCRCon(_LoRa);
	LoRa_setSpreadingFactor(_LoRa, _LoRa->spredingFactor);

// set Timeout Lsb:
	LoRa_writeRegister(_LoRa, RegSymbTimeoutL, 0xFF);

// set bandwidth, coding rate and expilicit mode:
	// 8 bit RegModemConfig --> | X | X | X | X | X | X | X | X |
	//       bits represent --> |   bandwidth   |     CR    |I/E|
	data = 0;
	data = (_LoRa->bandWidth << 4) + (_LoRa->codingRate << 1);
	LoRa_writeRegister(_LoRa, RegModemConfig1, data);

// set preamble:
	LoRa_writeRegister(_LoRa, RegPreambleMsb, _LoRa->preamble >> 8);
	LoRa_writeRegister(_LoRa, RegPreambleLsb, _LoRa->preamble >> 0);

    LoRa_set_LDRO(_LoRa, _LoRa->ldro);

// DIO mapping:   --> DIO: RxDone
	read = LoRa_readRegister(_LoRa, RegDioMapping1);
	data = read | 0x3F;
	LoRa_writeRegister(_LoRa, RegDioMapping1, data);

	LoRa_writeRegister(_LoRa, RegSyncWord, 0x12);

// goto standby mode:
	LoRa_gotoMode(_LoRa, STNBY_MODE);
	_LoRa->current_mode = STNBY_MODE;
	LoRa_Delay(_LoRa, 10);

	read = LoRa_readRegister(_LoRa, RegVersion);
	if(read != 0x12){
		return LORA_NOT_FOUND;
	}
	else{
		return LORA_OK;
	}
}