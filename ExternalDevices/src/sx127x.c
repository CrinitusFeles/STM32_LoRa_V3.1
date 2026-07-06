#include "sx127x.h"
#include "sx127x_misc.h"
#include "iwdg.h"
#include "string.h"



void SX127x_gotoMode(SX127x* driver, SX127x_Mode mode){
    driver->status.status = SX127x_Read(driver, RegOpMode);
    while(driver->status.mode != mode){
        driver->status.long_range_mode = 1;
        driver->status.low_freq_mode_on = 1;
        driver->status.mode = mode;
        SX127x_Write(driver, RegOpMode, &driver->status.status, 1);
        driver->status.status = SX127x_Read(driver, RegOpMode);
    }
}


void SX127x_setFrequency(SX127x* driver, uint32_t freq_hz){
	uint8_t  data = 0;
	uint32_t F;
    float freq_step = 0.016384;
	// F = (uint32_t)(freq_hz / 32000000 * 524288);
	F = (uint32_t)(freq_hz * freq_step);

	// write Msb:
	data = (uint8_t) (F >> 16);
	SX127x_Write(driver, RegFrMsb, &data, 1);
	// write Mid:
	data = (uint8_t) (F >> 8);
	SX127x_Write(driver, RegFrMid, &data, 1);
	// write Lsb:
	data = (uint8_t) (F >> 0);
	SX127x_Write(driver, RegFrLsb, &data, 1);
}


void SX127x_setSpreadingFactor(SX127x* driver, uint8_t SF){
	uint8_t	data;
	uint8_t	read;

	if(SF>12)
		SF = 12;
	if(SF<7)
		SF = 7;

	read = SX127x_Read(driver, RegModemConfig2);
	data = (SF << 4) + (read & 0x0F);
	SX127x_Write(driver, RegModemConfig2, &data, 1);
}

void SX127x_setPower(SX127x* driver, uint8_t power_dbm){
	SX127x_Write(driver, RegPaConfig, &power_dbm, 1);
}

void SX127x_setOCP(SX127x* driver, uint8_t current){
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
	SX127x_Write(driver, RegOcp, &OcpTrim, 1);
}

void SX127x_setTOMsb_setCRCon(SX127x* driver){
	uint8_t read, data;

	read = SX127x_Read(driver, RegModemConfig2);

	data = read | 0x07;
	SX127x_Write(driver, RegModemConfig2, &data, 1);
}

void SX127x_set_LDRO(SX127x* driver, uint8_t ldro){
	uint8_t read, data;

	read = SX127x_Read(driver, RegModemConfig3);

	data = (read & 0x08) | ((ldro & 0x01) << 3);
	SX127x_Write(driver, RegModemConfig3, &data, 1);
}

uint8_t SX127x_transmit(SX127x* driver, uint8_t *data, uint16_t length){
	uint8_t read = 0;
	// int mode = driver->base->config.current_mode;
	SX127x_gotoMode(driver, STNBY_MODE);
	SX127x_Write(driver, RegIrqFlags, (uint8_t[1]){0xFF}, 1);
	// read = SX127x_Read(driver, RegOpMode);
	read = SX127x_Read(driver, RegFiFoTxBaseAddr);
    uint16_t counter = 0;
    uint8_t chunk_size = 255;
    while(counter < length){
        if ((uint16_t)chunk_size > (length - counter)){
            chunk_size = length - counter;
        }
        SX127x_Write(driver, RegFiFoAddPtr, &read, 1);
        SX127x_Write(driver, RegPayloadLength, &chunk_size, 1);
        SX127x_Write(driver, RegFiFo, data, chunk_size);
        driver->base->transmitting_progress = 1;
        SX127x_gotoMode(driver, TRANSMIT_MODE);
        while(!driver->irq_status.TxDone){
            SX127x_ReadIRQ(driver);
        }
        driver->base->transmitting_progress = 0;
        driver->base->tx_data.payload_len = 0;
        IWDG_refresh();
        SX127x_Write(driver, RegIrqFlags, (uint8_t[1]){0xFF}, 1);
        SX127x_ReadIRQ(driver);
        counter += chunk_size;
    }
	// модуль ненадолго переходит в режим отправки, после чего возвращается в режим ожидания или приема.
	// Поэтому нет смысла пытаться прочитать из регистра режим отправки.
    SX127x_gotoMode(driver, RXCONTIN_MODE);
	return 1;
}


void SX127x_startReceiving(SX127x* driver){
	SX127x_gotoMode(driver, RXCONTIN_MODE);
}

void SX127x_ReadIRQ(SX127x* driver){
    uint8_t result = SX127x_Read(driver, RegIrqFlags);
    driver->irq_status.irq = result;
}

uint8_t SX127x_receive(SX127x* driver, uint8_t* data, uint8_t length){
	uint8_t min = 0;
	memset(driver->base->rx_data.payload, 0, RADIO_PROTOCOL_PAYLOAD_SIZE);
	// SX127x_gotoMode(driver, STNBY_MODE);
	// stat = SX127x_Read(driver, RegOpMode);
	SX127x_ReadIRQ(driver);
	if((driver->irq_status.RxDone) != 0){
		driver->base->rx_pkt_len = SX127x_Read(driver, RegRxNbBytes);
		driver->base->rx_buf_ptr = SX127x_Read(driver, RegFiFoRxCurrentAddr);
		SX127x_Write(driver, RegFiFoAddPtr, &(driver->base->rx_buf_ptr), 1);
		min = length >= driver->base->rx_pkt_len ? driver->base->rx_pkt_len : length;
		for(uint8_t i = 0; i < min; i++){
			data[i] = SX127x_Read(driver, RegFiFo);
        }
	}
	SX127x_Write(driver, RegIrqFlags, (uint8_t[1]){0xFF}, 1);
	SX127x_gotoMode(driver, RXCONTIN_MODE);
    return min;
}

int SX127x_getRSSI(SX127x* driver){
	uint8_t read = 0;
	read = SX127x_Read(driver, RegPktRssiValue);
	return -164 + read;
}

void SX127x_RxHandler(SX127x *driver){
    SX127x_receive(driver, driver->base->rx_data.buffer,
                   sizeof(driver->base->rx_data.buffer));
}

uint8_t SX127x_init(SX127x* driver){
	uint8_t  data;
	uint8_t  read;
	for(uint8_t i = 0; i < 4; i++){
		read = SX127x_Read(driver, RegOpMode);
		if((read & 0x07) != 0x00){
			LoRa_reset(driver);
			SX127x_gotoMode(driver, SLEEP_MODE);
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
	read = SX127x_Read(driver, RegOpMode);
	SX127x_Write(driver, RegOpMode, (uint8_t[1]){0x88}, 1);
	read = SX127x_Read(driver, RegOpMode);
	if(read != 0x88){
		SX127x_Write(driver, RegOpMode, (uint8_t[1]){0x88}, 1);
		read = SX127x_Read(driver, RegOpMode);
		if(read != 0x88){
			return 2;
		}
	}
// set frequency:
	SX127x_setFrequency(driver, driver->base->config.frequency);

// set output power gain:
	SX127x_setPower(driver, driver->base->config.power_dbm);

// set over current protection:
	SX127x_setOCP(driver, driver->base->config.overCurrentProtection);

// set LNA gain:
	SX127x_Write(driver, RegLna, (uint8_t[1]){0x23}, 1);

// set spreading factor, CRC on, and Timeout Msb:
	SX127x_setTOMsb_setCRCon(driver);
	SX127x_setSpreadingFactor(driver, driver->base->config.spreadingFactor);

// set Timeout Lsb:
	SX127x_Write(driver, RegSymbTimeoutL, (uint8_t[1]){0xFF}, 1);

// set bandwidth, coding rate and expilicit mode:
	// 8 bit RegModemConfig --> | X | X | X | X | X | X | X | X |
	//       bits represent --> |   bandwidth   |     CR    |I/E|
	data = 0;
	data = (driver->base->config.bandWidth << 4) + (driver->base->config.coding_rate << 1);
	SX127x_Write(driver, RegModemConfig1, &data, 1);

// set preamble:
	SX127x_Write(driver, RegPreambleMsb, (uint8_t[1]){driver->base->config.preamble >> 8}, 1);
	SX127x_Write(driver, RegPreambleLsb, (uint8_t[1]){driver->base->config.preamble >> 0}, 1);

    SX127x_set_LDRO(driver, driver->base->config.ldro);

// DIO mapping:   --> DIO: RxDone
	read = SX127x_Read(driver, RegDioMapping1);
	data = read | 0x3F;
	SX127x_Write(driver, RegDioMapping1, &data, 1);

	SX127x_Write(driver, RegSyncWord, (uint8_t[1]){0x12}, 1);

// goto standby mode:
	SX127x_gotoMode(driver, STNBY_MODE);
	driver->base->config.mode = STNBY_MODE;

	read = SX127x_Read(driver, RegVersion);
	if(read != 0x12){
		return 1;
	}
    return 0;
}