#ifndef INC_SX1278_H
#define INC_SX1278_H

#include "stm32l4xx.h"
#include "radio_protocol.h"
#include "LoRa.h"
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
#define RegSyncWord					0x39
#define RegDioMapping1				0x40
#define RegDioMapping2				0x41
#define RegVersion					0x42


typedef struct SX127x_IRQ_Status{
    uint8_t RxTimeout: 1;
    uint8_t RxDone: 1;
    uint8_t PayloadCrcError: 1;
    uint8_t ValidHeader: 1;
    uint8_t TxDone: 1;
    uint8_t CadDone: 1;
    uint8_t FhssChangeChannel: 1;
    uint8_t CadDetected: 1;
    uint8_t reserve1: 1;
    uint8_t reserve2: 1;
} SX127x_IRQ_Status;

typedef struct SX127x{
    SX127x_IRQ_Status irq_status;
    LoRa_t base;
} SX127x;


void SX127x_gotoMode(SX127x* _LoRa, uint8_t mode);

void SX127x_setFrequency(SX127x* _LoRa, uint32_t frequency);
void SX127x_setSpreadingFactor(SX127x* _LoRa, uint8_t SP);
void SX127x_setPower(SX127x* _LoRa, uint8_t power_dbm);
void SX127x_setOCP(SX127x* _LoRa, uint8_t current);
void SX127x_set_LDRO(SX127x* _LoRa, uint8_t ldro);
void SX127x_setTOMsb_setCRCon(SX127x* _LoRa);
uint8_t SX127x_transmit(SX127x* _LoRa, uint8_t *data, uint16_t length);
void SX127x_startReceiving(SX127x* _LoRa);
uint8_t SX127x_receive(SX127x* _LoRa, uint8_t* data, uint8_t length);
int SX127x_getRSSI(SX127x* _LoRa);
void SX127x_ReadIRQ(SX127x* _LoRa);
uint8_t SX127x_init(SX127x* _LoRa);

extern SX127x SX1278;

#endif