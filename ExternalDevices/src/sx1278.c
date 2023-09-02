#include "sx1278.h"
#include "gpio.h"
#include "delay.h"
#include "spi.h"


LoRa newLoRa(){
	LoRa new_LoRa;

	new_LoRa.frequency             = 433       ;
	new_LoRa.spredingFactor        = SF_7      ;
	new_LoRa.bandWidth			   = BW_125KHz ;
	new_LoRa.crcRate               = CR_4_5    ;
	new_LoRa.power				   = POWER_20db;
	new_LoRa.overCurrentProtection = 100       ;
	new_LoRa.preamble			   = 8         ;

	return new_LoRa;
}

void LoRa_reset(LoRa* _LoRa){
	gpio_state(_LoRa->reset_pin, LOW);
	Delay(1);
	gpio_state(_LoRa->reset_pin, HIGH);
	Delay(6);
}

void LoRa_gotoMode(LoRa* _LoRa, int mode){
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

uint8_t LoRa_readRegister(LoRa* _LoRa, uint8_t address){
	address &= ~(1 << 7);
	gpio_state(_LoRa->CS_pin, LOW);
	// Delay(1);
	// spi_send8(_LoRa->LoRaSPI, address);
	// uint8_t output = spi_recieve8(_LoRa->LoRaSPI);
	Delay(1);
	spi_txrx(_LoRa->LoRaSPI, address);
	volatile uint8_t output = spi_txrx(_LoRa->LoRaSPI, 0x00);
	Delay(1);
	gpio_state(_LoRa->CS_pin, HIGH);
	Delay(2);
	return output;
}

void LoRa_writeRegister(LoRa* _LoRa, uint8_t address, volatile uint8_t data){
	address |= (1 << 7);
	gpio_state(_LoRa->CS_pin, LOW);
	// Delay(1);
	// spi_send8(_LoRa->LoRaSPI, address);
	// spi_send8(_LoRa->LoRaSPI, data);
	Delay(1);
	spi_txrx(_LoRa->LoRaSPI, address);
	volatile uint8_t output = spi_txrx(_LoRa->LoRaSPI, data);
	Delay(1);
	gpio_state(_LoRa->CS_pin, HIGH);
	Delay(2);
}

void LoRa_writeRegisters(LoRa* _LoRa, uint8_t address, uint8_t* values, uint8_t w_length){
    address |= (1 << 7);
	gpio_state(_LoRa->CS_pin, LOW);
	Delay(1);
	spi_txrx(_LoRa->LoRaSPI, address);
	for(uint8_t i = 0; i < w_length; i++){
	    spi_txrx(_LoRa->LoRaSPI, values[i]);
    }
	Delay(1);
	gpio_state(_LoRa->CS_pin, HIGH);
	Delay(2);
}


void LoRa_setFrequency(LoRa* _LoRa, int freq){
	volatile uint8_t  data;
	volatile uint32_t F;
	F = (uint32_t)(freq * 524288) >> 5;

	// write Msb:
	data = (uint8_t) (F >> 16);
	LoRa_writeRegister(_LoRa, RegFrMsb, data);
	Delay(5);

	// write Mid:
	data = (uint8_t) (F >> 8);
	LoRa_writeRegister(_LoRa, RegFrMid, data);
	Delay(5);

	// write Lsb:
	data = (uint8_t) (F >> 0);
	LoRa_writeRegister(_LoRa, RegFrLsb, data);
	Delay(5);
}


void LoRa_setSpreadingFactor(LoRa* _LoRa, int SF){
	uint8_t	data;
	uint8_t	read;

	if(SF>12)
		SF = 12;
	if(SF<7)
		SF = 7;

	read = LoRa_readRegister(_LoRa, RegModemConfig2);
	Delay(10);

	data = (SF << 4) + (read & 0x0F);
	LoRa_writeRegister(_LoRa, RegModemConfig2, data);
	Delay(10);
}

void LoRa_setPower(LoRa* _LoRa, uint8_t power){
	LoRa_writeRegister(_LoRa, RegPaConfig, power);
	Delay(10);
}

void LoRa_setOCP(LoRa* _LoRa, uint8_t current){
	uint8_t	OcpTrim = 0;

	if(current<45)
		current = 45;
	if(current>240)
		current = 240;

	if(current <= 120)
		OcpTrim = (current - 45)/5;
	else if(current <= 240)
		OcpTrim = (current + 30)/10;

	OcpTrim = OcpTrim + (1 << 5);
	LoRa_writeRegister(_LoRa, RegOcp, OcpTrim);
	Delay(10);
}

void LoRa_setTOMsb_setCRCon(LoRa* _LoRa){
	uint8_t read, data;

	read = LoRa_readRegister(_LoRa, RegModemConfig2);

	data = read | 0x07;
	LoRa_writeRegister(_LoRa, RegModemConfig2, data);
	Delay(10);
}


uint8_t LoRa_transmit(LoRa* _LoRa, uint8_t *data, uint8_t length, uint16_t timeout){
	volatile uint8_t read;
	int mode = _LoRa->current_mode;
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
	return 1;
	// read = LoRa_readRegister(_LoRa, RegOpMode);
	// while(1){
	// 	read = LoRa_readRegister(_LoRa, RegIrqFlags);
	// 	if((read & 0x08)!=0){
	// 		LoRa_writeRegister(_LoRa, RegIrqFlags, 0xFF);
	// 		LoRa_gotoMode(_LoRa, mode);
	// 		return 1;
	// 	}
	// 	else{
	// 		if(--timeout==0){
	// 			LoRa_gotoMode(_LoRa, mode);
	// 			return 0;
	// 		}
	// 	}
	// 	Delay(1);
	// }

}


void LoRa_startReceiving(LoRa* _LoRa){
	LoRa_gotoMode(_LoRa, RXCONTIN_MODE);
}


uint8_t LoRa_receive(LoRa* _LoRa, uint8_t* data, uint8_t length){
	volatile uint8_t read;
	volatile uint8_t number_of_bytes;
	volatile uint8_t min = 0;
	// volatile uint8_t stat = 0;

	for(uint8_t i=0; i<length; i++)
		data[i]=0;

	LoRa_gotoMode(_LoRa, STNBY_MODE);
	// stat = LoRa_readRegister(_LoRa, RegOpMode);
	read = LoRa_readRegister(_LoRa, RegIrqFlags);
	if((read & 0x40) != 0){
		LoRa_writeRegister(_LoRa, RegIrqFlags, 0xFF);
		number_of_bytes = LoRa_readRegister(_LoRa, RegRxNbBytes);
		read = LoRa_readRegister(_LoRa, RegFiFoRxCurrentAddr);
		LoRa_writeRegister(_LoRa, RegFiFoAddPtr, read);
		min = length >= number_of_bytes ? number_of_bytes : length;
		for(uint8_t i=0; i<min; i++)
			data[i] = LoRa_readRegister(_LoRa, RegFiFo);
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
			Delay(50);
			LoRa_gotoMode(_LoRa, SLEEP_MODE);
			Delay(50);
		}
		else{
			break;
		}
		if(i == 3){
			while(1){
				Delay(1);
			}
		}
	}
	Delay(10);

// turn on lora mode:
	read = LoRa_readRegister(_LoRa, RegOpMode);
	LoRa_writeRegister(_LoRa, RegOpMode, 0x88);
	Delay(50);
	read = LoRa_readRegister(_LoRa, RegOpMode);
	if(read != 0x88){
		LoRa_writeRegister(_LoRa, RegOpMode, 0x88);
		read = LoRa_readRegister(_LoRa, RegOpMode);
		if(read != 0x88){
			while(1){
				Delay(1);
			}
		}
	}
// set frequency:
	LoRa_setFrequency(_LoRa, _LoRa->frequency);

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
	data = (_LoRa->bandWidth << 4) + (_LoRa->crcRate << 1);
	LoRa_writeRegister(_LoRa, RegModemConfig1, data);

// set preamble:
	LoRa_writeRegister(_LoRa, RegPreambleMsb, _LoRa->preamble >> 8);
	LoRa_writeRegister(_LoRa, RegPreambleLsb, _LoRa->preamble >> 0);

// DIO mapping:   --> DIO: RxDone
	read = LoRa_readRegister(_LoRa, RegDioMapping1);
	data = read | 0x3F;
	LoRa_writeRegister(_LoRa, RegDioMapping1, data);

	LoRa_writeRegister(_LoRa, RegSyncWord, 0x12);

// goto standby mode:
	LoRa_gotoMode(_LoRa, STNBY_MODE);
	_LoRa->current_mode = STNBY_MODE;
	Delay(10);

	read = LoRa_readRegister(_LoRa, RegVersion);
	if(read != 0x12){
		return LORA_NOT_FOUND;
	}
	else{
		return LORA_OK;
	}
}