#ifndef INC_SX1278_H
#define INC_SX1278_H

#include "stm32l4xx.h"

#define TRANSMIT_TIMEOUT	2000
#define RECEIVE_TIMEOUT		2000

//--------- MODES ---------//
#define SLEEP_MODE				0x00
#define	STNBY_MODE				0x01
#define	FSTX_MODE				0x02
#define TRANSMIT_MODE			0x03
#define FSRX_MODE			    0x04
#define RXCONTIN_MODE			0x05
#define RXSINGLE_MODE			0x06
#define CAD_MODE			    0x07

#define LORA_MODE               0x80
#define FSK_MODE                0x00
#define LOW_FREQ_MODE           0x08

//------- BANDWIDTH -------//
#define BW_7_8KHz					0
#define BW_10_4KHz					1
#define BW_15_6KHz					2
#define BW_20_8KHz					3
#define BW_31_25KHz					4
#define BW_41_7KHz					5
#define BW_62_5KHz					6
#define BW_125KHz					7
#define BW_250KHz					8
#define BW_500KHz					9

//------ CODING RATE ------//
#define CR_4_5						1
#define CR_4_6						2
#define CR_4_7						3
#define CR_4_8						4

//--- SPREADING FACTORS ---//
#define SF_7						7
#define SF_8						8
#define SF_9						9
#define SF_10						10
#define SF_11  						11
#define SF_12						12

//------ POWER GAIN ------//
#define POWER_11db					0xF6
#define POWER_14db					0xF9
#define POWER_17db					0xFC
#define POWER_20db					0xFF

//------- REGISTERS -------//
#define RegFiFo						0x00
#define RegOpMode					0x01
#define RegFrMsb					0x06
#define RegFrMid					0x07
#define RegFrLsb					0x08
#define RegPaConfig					0x09
#define RegOcp						0x0B
#define RegLna						0x0C
#define RegFiFoAddPtr				0x0D
#define RegFiFoTxBaseAddr			0x0E
#define RegFiFoRxBaseAddr			0x0F
#define RegFiFoRxCurrentAddr		0x10
#define RegIrqFlags					0x12
#define RegRxNbBytes				0x13
#define RegModemStat				0x18
#define RegPktRssiValue				0x1A
#define	RegModemConfig1				0x1D
#define RegModemConfig2				0x1E
#define RegSymbTimeoutL				0x1F
#define RegPreambleMsb				0x20
#define RegPreambleLsb				0x21
#define RegPayloadLength			0x22
#define RegModemConfig3				0x26
#define RegDioMapping1				0x40
#define RegDioMapping2				0x41
#define RegVersion					0x42

#define RegSyncWord					0x39

// --------------- IRQ FLAGS ---------------
#define LORA_IRQ_RX_TIMEOUT         0x80
#define LORA_IRQ_RX_DONE            0x40
#define LORA_IRQ_PAYLOAD_CRC_ERR    0x20
#define LORA_IRQ_VALID_HEADER       0x10
#define LORA_IRQ_TX_DONE            0x08
#define LORA_IRQ_CAD_DONE           0x04
#define LORA_IRQ_FHSS_CHANGE_CH     0x02
#define LORA_IRQ_CAD_DETECTED       0x01

//------ LORA STATUS ------//
#define LORA_OK						200
#define LORA_NOT_FOUND				404
#define LORA_LARGE_PAYLOAD			413
#define LORA_UNAVAILABLE			503

typedef struct LoRa_setting{

	// Hardware setings:
	uint8_t CS_pin;
	uint8_t reset_pin;
	uint8_t DIO0_pin;
	SPI_TypeDef* LoRaSPI;

	// Module settings:
	uint8_t			current_mode;
	uint32_t 		freq_mhz;
	uint8_t			spredingFactor;
	uint8_t			bandWidth;
	uint8_t			codingRate;
	uint16_t		preamble;
	uint8_t			power;
	uint8_t			overCurrentProtection;
    uint8_t         ldro;

    uint8_t         got_new_packet;
    uint8_t         rx_buffer[255];
    uint8_t         tx_buffer[255];
    void (*delay)(uint32_t milli);
} LoRa;


void LoRa_gotoMode(LoRa* _LoRa, uint8_t mode);

void LoRa_setFrequency(LoRa* _LoRa, uint32_t freq_mhz);
void LoRa_setSpreadingFactor(LoRa* _LoRa, uint8_t SP);
void LoRa_setPower(LoRa* _LoRa, uint8_t power);
void LoRa_setOCP(LoRa* _LoRa, uint8_t current);
void LoRa_set_LDRO(LoRa* _LoRa, uint8_t ldro);
void LoRa_setTOMsb_setCRCon(LoRa* _LoRa);
uint8_t LoRa_transmit(LoRa* _LoRa, uint8_t *data, uint8_t length);
void LoRa_startReceiving(LoRa* _LoRa);
uint8_t LoRa_receive(LoRa* _LoRa, uint8_t* data, uint8_t length);
int LoRa_getRSSI(LoRa* _LoRa);

uint16_t LoRa_init(LoRa* _LoRa);

extern LoRa sx127x;

#endif /* INC_SX1278_H_ */