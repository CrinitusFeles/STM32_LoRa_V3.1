#ifndef INC_SX126X_H_
#define INC_SX126X_H_

#include "stm32l4xx.h"
#include "gpio.h"
/***************************   Opcodes  ************************************/
#define OPCODE_RESET_STATS              0x00
#define OPCODE_CLEAR_IRQ_STATUS         0x02
#define OPCODE_CLEAR_DEVICE_ERRORS      0x07
#define OPCODE_SET_DIO_IRQ_PARAMS       0x08
#define OPCODE_WRITE_REGISTER           0x0d
#define OPCODE_WRITE_BUFFER             0x0e
#define OPCODE_GET_STATS                0x10
#define OPCODE_GET_PACKET_TYPE          0x11
#define OPCODE_GET_IRQ_STATUS           0x12
#define OPCODE_GET_RX_BUFFER_STATUS     0x13
#define OPCODE_GET_PACKET_STATUS        0x14
#define OPCODE_GET_RSSIINST             0x15
#define OPCODE_GET_DEVICE_ERRORS        0x17
#define OPCODE_READ_REGISTER            0x1d
#define OPCODE_READ_BUFFER              0x1e
#define OPCODE_SET_STANDBY              0x80
#define OPCODE_SET_RX                   0x82
#define OPCODE_SET_TX                   0x83
#define OPCODE_SET_SLEEP                0x84
#define OPCODE_SET_RF_FREQUENCY         0x86
#define OPCODE_SET_CAD_PARAM            0x88
#define OPCODE_CALIBRATE                0x89
#define OPCODE_SET_PACKET_TYPE          0x8a
#define OPCODE_SET_MODULATION_PARAMS    0x8b
#define OPCODE_SET_PACKET_PARAMS        0x8c
#define OPCODE_SET_TX_PARAMS            0x8e
#define OPCODE_SET_BUFFER_BASE_ADDR     0x8f
#define OPCODE_SET_FALLBACK_MODE        0x93
#define OPCODE_SET_RX_DUTY_CYCLE        0x94
#define OPCODE_SET_PA_CONFIG            0x95
#define OPCODE_SET_REGULATOR_MODE       0x96
#define OPCODE_SET_DIO3_AS_TCXO_CTRL    0x97
#define OPCODE_CALIBRATE_IMAGE          0x98
#define OPCODE_SET_DIO2_AS_RFSWITCH     0x9d
#define OPCODE_STOP_TIMER_ON_PREAMBLE   0x9f
#define OPCODE_SET_LORA_SYMBOL_TIMEOUT  0xa0
#define OPCODE_GET_STATUS               0xc0
#define OPCODE_SET_FS                   0xc1
#define OPCODE_SET_CAD                  0xc5
#define OPCODE_SET_TX_CARRIER           0xd1
#define OPCODE_SET_TX_PREAMBLE          0xd2

/***************************************************************/
#define SX126X_PACKET_TYPE_GFSK                       0
#define SX126X_PACKET_TYPE_LORA                       1

#define SX126X_HEADER_TYPE_VARIABLE_LENGTH            0
#define SX126X_HEADER_TYPE_FIXED_LENGTH               1

#define SX126X_CRC_OFF                                0
#define SX126X_CRC_ON                                 1

#define SX126X_STANDARD_IQ                            0
#define SX126X_INVERTED_IQ                            1

// SX126X register map
#define SX126X_REG_WHITENING_INITIAL_MSB              0x06B8
#define SX126X_REG_WHITENING_INITIAL_LSB              0x06B9
#define SX126X_REG_CRC_INITIAL_MSB                    0x06BC
#define SX126X_REG_CRC_INITIAL_LSB                    0x06BD
#define SX126X_REG_CRC_POLYNOMIAL_MSB                 0x06BE
#define SX126X_REG_CRC_POLYNOMIAL_LSB                 0x06BF
#define SX126X_REG_SYNC_WORD_0                        0x06C0
#define SX126X_REG_SYNC_WORD_1                        0x06C1
#define SX126X_REG_SYNC_WORD_2                        0x06C2
#define SX126X_REG_SYNC_WORD_3                        0x06C3
#define SX126X_REG_SYNC_WORD_4                        0x06C4
#define SX126X_REG_SYNC_WORD_5                        0x06C5
#define SX126X_REG_SYNC_WORD_6                        0x06C6
#define SX126X_REG_SYNC_WORD_7                        0x06C7
#define SX126X_REG_NODE_ADDRESS                       0x06CD
#define SX126X_REG_BROADCAST_ADDRESS                  0x06CE
#define SX126X_REG_LORA_SYNC_WORD_MSB                 0x0740
#define SX126X_REG_LORA_SYNC_WORD_LSB                 0x0741
#define SX126X_REG_RANDOM_NUMBER_0                    0x0819
#define SX126X_REG_RANDOM_NUMBER_1                    0x081A
#define SX126X_REG_RANDOM_NUMBER_2                    0x081B
#define SX126X_REG_RANDOM_NUMBER_3                    0x081C
#define SX126X_REG_RX_GAIN                            0x08AC
#define SX126X_REG_OCP_CONFIGURATION                  0x08E7
#define SX126X_REG_XTA_TRIM                           0x0911
#define SX126X_REG_XTB_TRIM                           0x0912
/**********************************************/

#define SX126X_RAMP_10U                               0x00
#define SX126X_RAMP_20U                               0x01
#define SX126X_RAMP_40U                               0x02
#define SX126X_RAMP_80U                               0x03
#define SX126X_RAMP_200U                              0x04
#define SX126X_RAMP_800U                              0x05
#define SX126X_RAMP_1700U                             0x06
#define SX126X_RAMP_3400U                             0x07

#define SX126X_BW_7                                   0x00  // 7.81 kHz real
#define SX126X_BW_10                                  0x08  // 10.42 kHz real
#define SX126X_BW_15                                  0x01  // 15.63 kHz real
#define SX126X_BW_20                                  0x09  // 20.83 kHz real
#define SX126X_BW_31                                  0x02  // 31.25 kHz real
#define SX126X_BW_41                                  0x0A  // 41.67 kHz real
#define SX126X_BW_62                                  0x03  // 62.50 kHz real
#define SX126X_BW_125                                 0x04  // 125 kHz real
#define SX126X_BW_250                                 0x05  // 250 kHz real
#define SX126X_BW_500                                 0x06  // 500 kHz real

#define SX126X_CR_4_5                                 1
#define SX126X_CR_4_6                                 2
#define SX126X_CR_4_7                                 3
#define SX126X_CR_4_8                                 4

typedef struct SX126x_GPIO{
    GPIO_Pin MOSI_pin;
    GPIO_Pin MISO_pin;
    GPIO_Pin SCK_pin;
    GPIO_Pin CS_pin;
	GPIO_Pin reset_pin;
    GPIO_Pin busy_pin;
	GPIO_Pin DIO1_pin;
    GPIO_Mode __MOSI_AF_pin;
    GPIO_Mode __MISO_AF_pin;
    GPIO_Mode __SCK_AF_pin;
} SX126x_GPIO;

typedef enum SX126x_ModeStatus{
    SX126x_RFU_Mode = 1,
    SX126x_STBY_RC_Mode,
    SX126x_STBY_XOSC_Mode,
    SX126x_FS_Mode,
    SX126x_RX_Mode,
    SX126x_TX_Mode
} SX126x_ModeStatus;

typedef struct SX126x_IRQ_Status{
    uint8_t TxDone;  // Packet transmission completed
    uint8_t RxDone;  // Packet received
    uint8_t PreambleDetected;  //  Preamble detected
    uint8_t SyncWordValid;  //  Valid sync word detected
    uint8_t HeaderValid;  // Valid LoRa header received
    uint8_t HeaderErr;  // LoRa header CRC error
    uint8_t CRC_Err;  //  Wrong CRC received
    uint8_t CadDone;  // Channel activity detection finished
    uint8_t CadDetected;  //  Channel activity detected
    uint8_t Timeout;  // Rx or Tx timeout
} SX126x_IRQ_Status;

typedef enum SX126x_CMD_Status{
    RFU_CMD = 1,
    Data_available,
    CMD_timeout,
    cmd_processing_error,
    fail_to_exec,
    tx_done
} SX126x_CMD_Status;

typedef struct SX126x_Mode{
    SX126x_ModeStatus mode : 3;
    SX126x_CMD_Status cmd : 3;
} SX126x_Mode;

typedef struct SX126x{

	// Hardware setings:
    SX126x_GPIO gpio;

	SPI_TypeDef *spi;

	// Module  gs:
    SX126x_Mode mode;
    uint8_t self_addr;
    uint8_t packet_type;
	uint32_t frequency;
    uint8_t header_type;
    uint8_t crc_on_off;
	uint8_t	spredingFactor;
	uint8_t	bandWidth;
	uint8_t	crcRate;
	uint16_t preamble_len;
	uint8_t	power_dbm;
    uint8_t ramping_time;
	uint8_t overCurrentProtection;
    uint8_t low_data_rate_optim;
    uint8_t iq_polarity;
    uint16_t sync_word;
    // ------- rx_packet data -----
    uint8_t new_rx_data_flag;
    uint8_t rssi;
    uint8_t snr;
    uint8_t signal_rssi;
    uint8_t rx_pkt_len;
    uint8_t rx_buf_ptr;
    SX126x_IRQ_Status irq_status;
    uint8_t busy_issues;
    uint8_t rx_data[128];
    uint8_t packet_configured;
} SX126x;

void SX126x_Init(SX126x *driver);
void SX126x_SendData(SX126x *driver, uint8_t *data, uint8_t data_len);
void SX126x_RxHandler(SX126x *driver);
void SX126x_RxDataParse(SX126x *driver);

void SX126x_GetStatus(SX126x *driver);
void SX126x_GetRxBufferStatus(SX126x *driver);
void SX126x_GetPacketStatus(SX126x *driver);
uint16_t SX126x_GetDeviceErrors(SX126x *driver);
void SX126x_GetIrqStatus(SX126x *driver);
// 0 - STBY_RC; 1 - STBY_XOSC
void SetRxTxFallbackMode(SX126x *driver);
void SX126x_SetStandby(SX126x *driver, uint8_t config);
void SX126x_SetRx(SX126x *driver, uint16_t timeout_ms);
void SX126x_SetTx(SX126x *driver, uint16_t timeout_ms);
void SX126x_SetSleep(SX126x *driver);
void SX126x_SetPacketParams(SX126x *driver, uint16_t preamble_len, uint8_t header_type,
                            uint8_t payload_len, uint8_t crc_type, uint8_t invert_iq);
void SX126x_SetModulationParams(SX126x *driver, uint8_t sf, uint8_t bw, uint8_t cr, uint8_t ldro);
void SX126x_SetBufferBaseAddress(SX126x *driver, uint8_t tx_buf_addr, uint8_t rx_buf_addr);
// power_dbm: from 14 (0x0E) to 22 (0x16); ramp_time: from 0 to 7
void SX126x_SetTxParams(SX126x *driver, uint8_t power_dbm, uint8_t ramp_time);
// 1 - Lora; 0 - GFSK

void SX126x_SetPacketType(SX126x *driver, uint8_t packet_type);
uint8_t SX126x_GetPacketType(SX126x *driver);
void SX126x_SetRfFrequency(SX126x *driver, uint32_t freq_hz);
void SX126x_ReadBuffer(SX126x *driver, uint8_t offset, uint8_t *data, uint8_t len);
void SX126x_WriteBuffer(SX126x *driver, uint8_t *offset_and_data, uint8_t len);
void SX126x_WriteRegisters(SX126x *driver, uint8_t *addr_and_regs, uint8_t len);
void SX126x_ReadRegisters(SX126x *driver, uint16_t address, uint8_t *answer, uint8_t len);
void SX126x_ClearIrqStatus(SX126x *driver, uint16_t param);
void SX126x_SetOCP(SX126x *driver, uint8_t ocp);
void SX126x_SetSyncWord(SX126x *driver, uint16_t sync_word);
void SX126x_SetDIO2AsRfSwitchCtr(SX126x *driver, uint8_t enable);
void SX126x_SetDioIrqParams(SX126x *driver, uint16_t irq_mask, uint16_t dio1_mask, uint16_t dio2_mask, uint16_t dio3_mask);


SX126x SX1268;

#endif  // INC_SX126X_H_