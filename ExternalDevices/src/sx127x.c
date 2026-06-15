#include "sx127x.h"
#include "sx127x_misc.h"
#include "iwdg.h"




void SX127x_gotoMode(SX127x* _LoRa, uint8_t mode){
	uint8_t read = 0;
	uint8_t data = 0;

	read = SX127x_Read(_LoRa, RegOpMode);
	data = (read & 0xF8);

	if(mode == SLEEP_MODE){
		_LoRa->base.config.mode = SLEEP_MODE;
		SX127x_Write(_LoRa, RegOpMode, (uint8_t[1]){0x08}, 1);
		return;
	}else if (mode == STNBY_MODE){
		data |= 0x01;
		_LoRa->base.config.mode = STNBY_MODE;
	}else if (mode == TRANSMIT_MODE){
		data |= 0x03;
		_LoRa->base.config.mode = TRANSMIT_MODE;
	}else if (mode == RXCONTIN_MODE){
		data |= 0x05;
		_LoRa->base.config.mode = RXCONTIN_MODE;
	}else if (mode == RXSINGLE_MODE){
		data |= 0x06;
		_LoRa->base.config.mode = RXSINGLE_MODE;
	}

	SX127x_Write(_LoRa, RegOpMode, &data, 1);
}


void SX127x_setFrequency(SX127x* _LoRa, uint32_t freq_hz){
	uint8_t  data = 0;
	uint32_t F;
    float freq_step = 0.016384;
	// F = (uint32_t)(freq_hz / 32000000 * 524288);
	F = (uint32_t)(freq_hz * freq_step);

	// write Msb:
	data = (uint8_t) (F >> 16);
	SX127x_Write(_LoRa, RegFrMsb, &data, 1);
	// write Mid:
	data = (uint8_t) (F >> 8);
	SX127x_Write(_LoRa, RegFrMid, &data, 1);
	// write Lsb:
	data = (uint8_t) (F >> 0);
	SX127x_Write(_LoRa, RegFrLsb, &data, 1);
}


void SX127x_setSpreadingFactor(SX127x* _LoRa, uint8_t SF){
	uint8_t	data;
	uint8_t	read;

	if(SF>12)
		SF = 12;
	if(SF<7)
		SF = 7;

	read = SX127x_Read(_LoRa, RegModemConfig2);
	data = (SF << 4) + (read & 0x0F);
	SX127x_Write(_LoRa, RegModemConfig2, &data, 1);
}

void SX127x_setPower(SX127x* _LoRa, uint8_t power_dbm){
	SX127x_Write(_LoRa, RegPaConfig, &power_dbm, 1);
}

void SX127x_setOCP(SX127x* _LoRa, uint8_t current){
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
	SX127x_Write(_LoRa, RegOcp, &OcpTrim, 1);
}

void SX127x_setTOMsb_setCRCon(SX127x* _LoRa){
	uint8_t read, data;

	read = SX127x_Read(_LoRa, RegModemConfig2);

	data = read | 0x07;
	SX127x_Write(_LoRa, RegModemConfig2, &data, 1);
}

void SX127x_set_LDRO(SX127x* _LoRa, uint8_t ldro){
	uint8_t read, data;

	read = SX127x_Read(_LoRa, RegModemConfig3);

	data = (read & 0x08) | ((ldro & 0x01) << 3);
	SX127x_Write(_LoRa, RegModemConfig3, &data, 1);
}

uint8_t SX127x_transmit(SX127x* _LoRa, uint8_t *data, uint16_t length){
	uint8_t read = 0;
	// int mode = _LoRa->base.config.current_mode;
	SX127x_gotoMode(_LoRa, STNBY_MODE);
	SX127x_Write(_LoRa, RegIrqFlags, (uint8_t[1]){0xFF}, 1);
	read = SX127x_Read(_LoRa, RegOpMode);
	read = SX127x_Read(_LoRa, RegFiFoTxBaseAddr);
    uint16_t counter = 0;
    uint8_t chunk_size = 255;
    while(counter < length){
        if ((uint16_t)chunk_size > (length - counter)){
            chunk_size = length - counter;
        }
        SX127x_Write(_LoRa, RegFiFoAddPtr, &read, 1);
        SX127x_Write(_LoRa, RegPayloadLength, &chunk_size, 1);
        SX127x_Write(_LoRa, RegFiFo, data, chunk_size);
        read = SX127x_Read(_LoRa, RegOpMode);
        _LoRa->base.transmitting_progress = 1;
        SX127x_gotoMode(_LoRa, TRANSMIT_MODE);
        while(1){
            SX127x_ReadIRQ(_LoRa);
            read = SX127x_Read(_LoRa, RegOpMode);
            if(read != TRANSMIT_MODE && (_LoRa->irq_status.TxDone))
                break;
        }
        _LoRa->base.transmitting_progress = 0;
        _LoRa->base.tx_data.dlen = 0;
        IWDG_refresh();
        counter += chunk_size;
    }
	// модуль ненадолго переходит в режим отправки, после чего возвращается в режим ожидания или приема.
	// Поэтому нет смысла пытаться прочитать из регистра режим отправки.
    SX127x_gotoMode(_LoRa, RXCONTIN_MODE);
	return 1;
}


void SX127x_startReceiving(SX127x* _LoRa){
	SX127x_gotoMode(_LoRa, RXCONTIN_MODE);
}

void SX127x_ReadIRQ(SX127x* _LoRa){
    uint8_t result = SX127x_Read(_LoRa, RegIrqFlags);
    _LoRa->irq_status = *(SX127x_IRQ_Status*)(&result);
}

uint8_t SX127x_receive(SX127x* _LoRa, uint8_t* data, uint8_t length){
	uint8_t min = 0;
	// volatile uint8_t stat = 0;
	for(uint8_t i = 0; i < length; i++){
		data[i] = 0;
    }
	SX127x_gotoMode(_LoRa, STNBY_MODE);
	// stat = SX127x_Read(_LoRa, RegOpMode);
	SX127x_ReadIRQ(_LoRa);
	if((_LoRa->irq_status.RxDone) != 0){
		SX127x_Write(_LoRa, RegIrqFlags, (uint8_t[1]){0xFF}, 1);
		_LoRa->base.rx_pkt_len = SX127x_Read(_LoRa, RegRxNbBytes);
		_LoRa->base.rx_buf_ptr = SX127x_Read(_LoRa, RegFiFoRxCurrentAddr);
		SX127x_Write(_LoRa, RegFiFoAddPtr, &(_LoRa->base.rx_buf_ptr), 1);
		min = length >= _LoRa->base.rx_pkt_len ? _LoRa->base.rx_pkt_len : length;
		for(uint8_t i = 0; i < min; i++){
			data[i] = SX127x_Read(_LoRa, RegFiFo);
        }
	}
	SX127x_Write(_LoRa, RegIrqFlags, (uint8_t[1]){0xFF}, 1);
	SX127x_gotoMode(_LoRa, RXCONTIN_MODE);
    return min;
}

int SX127x_getRSSI(SX127x* _LoRa){
	uint8_t read = 0;
	read = SX127x_Read(_LoRa, RegPktRssiValue);
	return -164 + read;
}

uint8_t SX127x_init(SX127x* _LoRa){
	uint8_t  data;
	uint8_t  read;
	for(uint8_t i = 0; i < 4; i++){
		read = SX127x_Read(_LoRa, RegOpMode);
		if((read & 0x07) != 0x00){
			LoRa_reset(_LoRa);
			SX127x_gotoMode(_LoRa, SLEEP_MODE);
		}
		else{
			break;
		}
		if(i == 3){
			while(1){
			}
		}
	}

// turn on lora mode:
	read = SX127x_Read(_LoRa, RegOpMode);
	SX127x_Write(_LoRa, RegOpMode, (uint8_t[1]){0x88}, 1);
	read = SX127x_Read(_LoRa, RegOpMode);
	if(read != 0x88){
		SX127x_Write(_LoRa, RegOpMode, (uint8_t[1]){0x88}, 1);
		read = SX127x_Read(_LoRa, RegOpMode);
		if(read != 0x88){
			return 2;
		}
	}
// set frequency:
	SX127x_setFrequency(_LoRa, _LoRa->base.config.frequency);

// set output power gain:
	SX127x_setPower(_LoRa, _LoRa->base.config.power_dbm);

// set over current protection:
	SX127x_setOCP(_LoRa, _LoRa->base.config.overCurrentProtection);

// set LNA gain:
	SX127x_Write(_LoRa, RegLna, (uint8_t[1]){0x23}, 1);

// set spreading factor, CRC on, and Timeout Msb:
	SX127x_setTOMsb_setCRCon(_LoRa);
	SX127x_setSpreadingFactor(_LoRa, _LoRa->base.config.spreadingFactor);

// set Timeout Lsb:
	SX127x_Write(_LoRa, RegSymbTimeoutL, (uint8_t[1]){0xFF}, 1);

// set bandwidth, coding rate and expilicit mode:
	// 8 bit RegModemConfig --> | X | X | X | X | X | X | X | X |
	//       bits represent --> |   bandwidth   |     CR    |I/E|
	data = 0;
	data = (_LoRa->base.config.bandWidth << 4) + (_LoRa->base.config.coding_rate << 1);
	SX127x_Write(_LoRa, RegModemConfig1, &data, 1);

// set preamble:
	SX127x_Write(_LoRa, RegPreambleMsb, (uint8_t[1]){_LoRa->base.config.preamble >> 8}, 1);
	SX127x_Write(_LoRa, RegPreambleLsb, (uint8_t[1]){_LoRa->base.config.preamble >> 0}, 1);

    SX127x_set_LDRO(_LoRa, _LoRa->base.config.ldro);

// DIO mapping:   --> DIO: RxDone
	read = SX127x_Read(_LoRa, RegDioMapping1);
	data = read | 0x3F;
	SX127x_Write(_LoRa, RegDioMapping1, &data, 1);

	SX127x_Write(_LoRa, RegSyncWord, (uint8_t[1]){0x12}, 1);

// goto standby mode:
	SX127x_gotoMode(_LoRa, STNBY_MODE);
	_LoRa->base.config.mode = STNBY_MODE;

	read = SX127x_Read(_LoRa, RegVersion);
	if(read != 0x12){
		return 1;
	}
    return 0;
}