#include "sdio.h"
#include "gpio.h"


SDCard_TypeDef SDCard;                 // SD card parameters
SDResult sd_last_result;
void SDMMC_INIT(){
	gpio_init(PC8, PC8_SDMMC1_D0, Push_pull, no_pull, High_speed);
	gpio_init(PC9, PC9_SDMMC1_D1, Push_pull, no_pull, High_speed);
	gpio_init(PC10, PC10_SDMMC1_D2, Push_pull, no_pull, High_speed);
	gpio_init(PC11, PC11_SDMMC1_D3, Push_pull, no_pull, High_speed);
	gpio_init(PC12, PC12_SDMMC1_CK, Push_pull, no_pull, High_speed);
	gpio_init(PD2, PD2_SDMMC1_CMD, Push_pull, no_pull, High_speed);


	RCC->APB2ENR |= RCC_APB2ENR_SDMMC1EN;
	RCC->PLLCFGR |= RCC_PLLCFGR_PLLQEN;
	RCC->CCIPR |= RCC_CCIPR_CLK48SEL; //clock from PLLQ

	//clock from HSI48
	SDMMC1->CLKCR = 0x86 | SDMMC_CLKCR_CLKEN | SDMMC_CLKCR_WIDBUS_0 | SDMMC_CLKCR_HWFC_EN;

//	SDMMC1->POWER = 0x03;
}


// Send command to the SD card
// input:
//   cmd - SD card command
//   arg - 32-bit argument for SD card command
//   resp_type - response type (SDMMC_RESP_xxx)
// return: SDResult value
SDResult SD_Cmd(uint8_t cmd, uint32_t arg, uint16_t resp_type) {
	uint32_t wait = SDMMC_CMD_TIMEOUT;

	// Clear the command flags
	SDMMC1->ICR = SDMMC_ICR_CCRCFAILC | SDMMC_ICR_CTIMEOUTC | SDMMC_ICR_CMDRENDC | SDMMC_ICR_CMDSENTC;

	// Command argument value
	SDMMC1->ARG = arg;

	// Write to SDIO CMD
	SDMMC1->CMD = resp_type | cmd | SDMMC_CMD_CPSMEN;

	// Block till get a response
	if (resp_type == SDMMC_RESP_NONE) {
		// Wait for timeout or CMD sent flag
		while (!(SDMMC1->STA & (SDMMC_STA_CTIMEOUT | SDMMC_STA_CMDSENT)) && --wait);
	} else {
		// Wait for CMDSENT or CRCFAIL
		while (!(SDMMC1->STA & (SDMMC_STA_CTIMEOUT | SDMMC_STA_CMDREND | SDMMC_STA_CCRCFAIL)) && --wait);
	}

	// Check response
	if ((SDMMC1->STA & SDMMC_STA_CTIMEOUT) || !wait) return SDR_Timeout;
	if (SDMMC1->STA & SDMMC_STA_CCRCFAIL) return SDR_CRCError; // CRC fail will be always for R3 response

	return SDR_Success;
}

// Get SDResult code from card status information
// input:
//   cs - card status (32-bit value)
// return: SDResult code
SDResult SD_GetError(uint32_t cs) {
	SDResult result = SDR_Success;

	if (cs & SD_CS_ERROR_BITS) {
		if (cs & SD_CS_OUT_OF_RANGE)       result = SDR_AddrOutOfRange;
		if (cs & SD_CS_ADDRESS_ERROR)      result = SDR_AddrMisaligned;
		if (cs & SD_CS_BLOCK_LEN_ERROR)    result = SDR_BlockLenError;
		if (cs & SD_CS_ERASE_SEQ_ERROR)    result = SDR_EraseSeqError;
		if (cs & SD_CS_ERASE_PARAM)        result = SDR_EraseParam;
		if (cs & SD_CS_WP_VIOLATION)       result = SDR_WPViolation;
		if (cs & SD_CS_LOCK_UNLOCK_FAILED) result = SDR_LockUnlockFailed;
		if (cs & SD_CS_COM_CRC_ERROR)      result = SDR_ComCRCError;
		if (cs & SD_CS_ILLEGAL_COMMAND)    result = SDR_IllegalCommand;
		if (cs & SD_CS_CARD_ECC_FAILED)    result = SDR_CardECCFailed;
		if (cs & SD_CS_CC_ERROR)           result = SDR_CCError;
		if (cs & SD_CS_ERROR)              result = SDR_GeneralError;
		if (cs & SD_CS_STREAM_R_UNDERRUN)  result = SDR_StreamUnderrun;
		if (cs & SD_CS_STREAM_W_OVERRUN)   result = SDR_StreamOverrun;
		if (cs & SD_CS_CSD_OVERWRITE)      result = SDR_CSDOverwrite;
		if (cs & SD_CS_WP_ERASE_SKIP)      result = SDR_WPEraseSkip;
		if (cs & SD_CS_CARD_ECC_DISABLED)  result = SDR_ECCDisabled;
		if (cs & SD_CS_ERASE_RESET)        result = SDR_EraseReset;
		if (cs & SD_CS_AKE_SEQ_ERROR)      result = SDR_AKESeqError;
	}

	return result;
}

// Get response from the SD card
// input:
//   resp_type - response type (SD_Rxx values)
//   pResp - pointer to the array for response (1..4 32-bit values)
// return:
//   for R1 or R1b responses:
//     SDR_Success if no error or SDR_XXX in case of some error bits set
//     pResp contains a card status value
//   for R2 response:
//     result always is SDR_Success
//     pResp contains a 128-bit CSD or CID register value
//   for R3 response:
//     SDR_Success if no error or SDR_BadResponse in case of bad OCR register header
//     pResp contains a 32-bit OCR register value
//   for R6 response:
//     SDR_Success if no error or SDR_BadResponse in case of bad RCA response
//     pResp contains a 32-bit RCA value
//   for R7 response:
//     SDR_Success if no error or SDR_BadResponse in case of bad R7 response header
//     pResp contains a 32-bit value of R7 response
SDResult SD_Response(uint16_t resp_type, uint32_t *pResp) {
	SDResult result = SDR_Success;

	// Get first 32-bit value, it similar for all types of response except R2
	*pResp = SDMMC1->RESP1;

	switch (resp_type) {
		case SD_R1:
		case SD_R1b:
			// RESP1 contains card status information
			// Check for error bits in card status
			result = SD_GetError(*pResp);
			break;
		case SD_R2:
			// RESP1..4 registers contain the CID/CSD register value
#ifdef __GNUC__
			// Use GCC built-in intrinsics (fastest, less code) (GCC v4.3 or later)
			*pResp++ = __builtin_bswap32(SDMMC1->RESP1);
			*pResp++ = __builtin_bswap32(SDMMC1->RESP2);
			*pResp++ = __builtin_bswap32(SDMMC1->RESP3);
			*pResp   = __builtin_bswap32(SDMMC1->RESP4);
#else
			// Use ARM 'REV' instruction (fast, a bit bigger code than GCC intrinsics)
			*pResp++ = __REV(SDMMC1->RESP1);
			*pResp++ = __REV(SDMMC1->RESP2);
			*pResp++ = __REV(SDMMC1->RESP3);
			*pResp   = __REV(SDMMC1->RESP4);
			// Use SHIFT, AND and OR (slower, biggest code)
//			*pResp++ = SWAP_UINT32(SDMMC1->RESP1);
//			*pResp++ = SWAP_UINT32(SDMMC1->RESP2);
//			*pResp++ = SWAP_UINT32(SDMMC1->RESP3);
//			*pResp   = SWAP_UINT32(SDMMC1->RESP4);
#endif
			break;
		case SD_R3:
			// RESP1 contains the OCR register value
			// Check for correct OCR header
			if (SDMMC1->RESPCMD != 0x3f) result = SDR_BadResponse;
			break;
		case SD_R6:
			// RESP1 contains the RCA response value
			// Only CMD3 generates R6 response, so RESPCMD must be 0x03
			if (SDMMC1->RESPCMD != 0x03) result = SDR_BadResponse;
			break;
		case SD_R7:
			// RESP1 contains 'Voltage accepted' and echo-back of check pattern
			// Only CMD8 generates R7 response, so RESPCMD must be 0x08
			if (SDMMC1->RESPCMD != 0x08) result = SDR_BadResponse;
			break;
		default:
			// Unknown response
			result = SDR_BadResponse;
			break;
	}

	return result;
}

// Set block size of the SD card
// input:
//   block_size - block length
// return: SDResult value
SDResult SD_SetBlockSize(uint32_t block_size) {
	uint32_t response;

	// Send SET_BLOCKLEN command
	SD_Cmd(SD_CMD_SET_BLOCKLEN, 8, SDMMC_RESP_SHORT); // CMD16

	return SD_Response(SD_R1, &response);
}

// Find the SD card SCR register value
// input:
//   pSCR - pointer to the SCR register value
// return: SDResult value
// note: card must be in transfer mode, not supported by MMC
SDResult SD_GetSCR(uint32_t *pSCR) {
	SDResult cmd_res;
	uint32_t response;

	// Set block size to 8 bytes
	SD_Cmd(SD_CMD_SET_BLOCKLEN,8,SDMMC_RESP_SHORT); // CMD16
	cmd_res = SD_Response(SD_R1,&response);
	if (cmd_res != SDR_Success) return cmd_res;

	// Send leading command for ACMD<n> command
	SD_Cmd(SD_CMD_APP_CMD,SDCard.RCA << 16,SDMMC_RESP_SHORT); // CMD55
	cmd_res = SD_Response(SD_R1,&response);
	if (cmd_res != SDR_Success) return cmd_res;

	// Clear the data flags
	SDMMC1->ICR = SDMMC_ICR_RXOVERRC | SDMMC_ICR_DCRCFAILC | SDMMC_ICR_DTIMEOUTC | SDMMC_ICR_DBCKENDC | SDMMC_ICR_STBITERRC;

	// Configure the SDIO data transfer
	SDMMC1->DTIMER = SDMMC_DATA_R_TIMEOUT; // Data read timeout
	SDMMC1->DLEN   = 8; // Data length in bytes
	// Data transfer: block, card -> controller, size: 2^3 = 8bytes, enable data transfer
	SDMMC1->DCTRL  = SDMMC_DCTRL_DTDIR | (3 << 4) | SDMMC_DCTRL_DTEN;

	// Send SEND_SCR command
	SD_Cmd(SD_CMD_SEND_SCR,0,SDMMC_RESP_SHORT); // ACMD51
	cmd_res = SD_Response(SD_R1,&response);
	if (cmd_res != SDR_Success) return cmd_res;

	// Read a SCR register value from SDIO FIFO
	while (!(SDMMC1->STA & (SDMMC_STA_RXOVERR | SDMMC_STA_DCRCFAIL | SDMMC_STA_DTIMEOUT | SDMMC_STA_DBCKEND | SDMMC_STA_STBITERR))) {
		// Read word when data available in receive FIFO
		if (SDMMC1->STA & SDMMC_STA_RXDAVL) *pSCR++ = SDMMC1->FIFO;
	}

	// Check for errors
	if (SDMMC1->STA & SDMMC_STA_DTIMEOUT) cmd_res = SDR_DataTimeout;
	if (SDMMC1->STA & SDMMC_STA_DCRCFAIL) cmd_res = SDR_DataCRCFail;
	if (SDMMC1->STA & SDMMC_STA_RXOVERR)  cmd_res = SDR_RXOverrun;
	if (SDMMC1->STA & SDMMC_STA_STBITERR) cmd_res = SDR_StartBitError;

	// Clear the static SDIO flags
	SDMMC1->ICR = SDMMC_ICR_STATIC;

	return cmd_res;
}

// Initialize the SD card
// return: SDResult value
// note: SDIO peripheral clock must be on and SDIO GPIO configured
SDResult SD_Init(void) {
	uint32_t trials;
	uint32_t response[4];
	uint32_t sd_type = SD_STD_CAPACITY; // SD card capacity
	SDResult cmd_res;

	// Populate SDCard structure with default values
	SDCard.Capacity = 0;
	SDCard.MaxBusClkFreq = 0;
	SDCard.BlockSize = 0;
	SDCard.CSDVer = 0;
	SDCard.Type = SDCT_UNKNOWN;
	SDCard.RCA = 0;

	SDMMC1->POWER = SDMMC_PWR_ON; // Enable SDIO clock

	SD_Cmd(SD_CMD_GO_IDLE_STATE, 0x00, SDMMC_RESP_NONE);

	// CMD8: SEND_IF_COND. Send this command to verify SD card interface operating condition
	// Argument: - [31:12]: Reserved (shall be set to '0')
	//           - [11:8]: Supply Voltage (VHS) 0x1 (Range: 2.7-3.6 V)
	//           - [7:0]: Check Pattern (recommended 0xAA)
	cmd_res = SD_Cmd(SD_CMD_HS_SEND_EXT_CSD, SD_CHECK_PATTERN, SDMMC_RESP_SHORT); // CMD8
	if (cmd_res == SDR_Success) {
		// SD v2.0 or later

		// Get and check R7 response
		if (SD_Response(SD_R7, response) != SDR_Success) return SDR_BadResponse;
		// Check echo-back of check pattern
		if ((response[0] & 0x01ff) != (SD_CHECK_PATTERN & 0x01ff)) return SDR_Unsupported;
		sd_type = SD_HIGH_CAPACITY; // SD v2.0 or later

		// Issue ACMD41 command
		trials = SDMMC_ACMD41_TRIALS;
		while (--trials) {
			// Send leading command for ACMD<n> command
			SD_Cmd(SD_CMD_APP_CMD, 0, SDMMC_RESP_SHORT); // CMD55 with RCA 0
			if (SD_Response(SD_R1, response) != SDR_Success)
                return SDR_BadResponse;
			// ACMD41 - initiate initialization process.
			// Set 3.0-3.3V voltage window (bit 20)
			// Set HCS bit (30) (Host Capacity Support) to inform card what host support high capacity
			// Set XPC bit (28) (SDXC Power Control) to use maximum performance (SDXC only)
			SD_Cmd(SD_CMD_SD_SEND_OP_COND, SD_OCR_VOLTAGE | sd_type, SDMMC_RESP_SHORT);
			if (SD_Response(SD_R3, response) != SDR_Success)
                return SDR_BadResponse;
			// Check if card finished power up routine
			if (response[0] & (1 << 31)) break;
		}
		if (!trials) return SDR_InvalidVoltage; // Unsupported voltage range
		// Check if card is SDHC/SDXC
		SDCard.Type = (response[0] & SD_HIGH_CAPACITY) ? SDCT_SDHC : SDCT_SDSC_V2;
	} else if (cmd_res == SDR_Timeout) {
		// SD v1.x or MMC

		// Issue CMD55 to reset 'Illegal command' bit of the SD card
		SD_Cmd(SD_CMD_APP_CMD, 0, SDMMC_RESP_SHORT); // CMD55 with RCA 0

		// Issue ACMD41 command with zero argument
		trials = SDMMC_ACMD41_TRIALS;
		while (--trials) {
			// Send leading command for ACMD<n> command
			SD_Cmd(SD_CMD_APP_CMD, 0, SDMMC_RESP_SHORT); // CMD55 with RCA 0
			if (SD_Response(SD_R1, response) != SDR_Success)
                return SDR_BadResponse;
			// ACMD41 - initiate initialization process (bit HCS = 0)
			// R3 response do not protected with CRC and here will be CRC error
			cmd_res = SD_Cmd(SD_CMD_SD_SEND_OP_COND, SD_OCR_VOLTAGE, SDMMC_RESP_SHORT);
			if (cmd_res == SDR_Timeout) break; // MMC will not respond to this command
			if (SD_Response(SD_R3,response) != SDR_Success)
                return SDR_BadResponse;
			// Check if card finished power up routine
			if (response[0] & (1 << 31)) break;
		}
		if (!trials) return SDR_UnknownCard; // Unsupported card
		if (cmd_res != SDR_Timeout) {
			// SD v1.x
			SDCard.Type = SDCT_SDSC_V1; // SDv1
		} else {
			// MMC or not SD memory card

			///////////////////////////////////////////////////////////
			// This part is not checked due to lack of MMCmicro card //
			///////////////////////////////////////////////////////////

			trials = SDMMC_ACMD41_TRIALS;
			while (--trials) {
				// Issue CMD1: initiate initialization process.
				SD_Cmd(SD_CMD_SEND_OP_COND, SD_OCR_VOLTAGE, SDMMC_RESP_SHORT); // CMD1
				if (SD_Response(SD_R3, response) != SDR_Success) return SDR_BadResponse;
				// Check if card finished power up routine
				if (response[0] & (1 << 31)) break;
			}
			if (!trials) return SDR_UnknownCard;
			SDCard.Type = SDCT_MMC; // MMC
		}
	} else return cmd_res;

	// Now the CMD2 and CMD3 commands should be issued in cycle until timeout to enumerate all cards on the bus.
	// Since this module suitable to work with single card, issue this commands one time only.

	// Send ALL_SEND_CID command
	cmd_res = SD_Cmd(SD_CMD_ALL_SEND_CID, 0, SDMMC_RESP_LONG); // CMD2
	if (cmd_res != SDR_Success) return cmd_res;
	SD_Response(SD_R2, (uint32_t *)SDCard.CID); // Retrieve CID register from the card

	// Send SEND_REL_ADDR command (ask the card to publish a new RCA (Relative Card Address)
	// Once the RCA is received the card state changes to the stand-by state
	if (SDCard.Type != SDCT_MMC) {
		// SD
		cmd_res = SD_Cmd(SD_CMD_SEND_REL_ADDR, 0, SDMMC_RESP_SHORT); // CMD3
		if (cmd_res != SDR_Success) return cmd_res;
		SD_Response(SD_R6,response);
	} else {
		// MMC
		cmd_res = SD_Cmd(SD_CMD_SEND_REL_ADDR, 1 << 16, SDMMC_RESP_SHORT); // CMD3
		if (cmd_res != SDR_Success) return cmd_res;
		SD_Response(SD_R2, response);
	}
	SDCard.RCA = response[0] >> 16;

	// Send SEND_CSD command
	cmd_res = SD_Cmd(SD_CMD_SEND_CSD, SDCard.RCA << 16, SDMMC_RESP_LONG); // CMD9
	if (cmd_res != SDR_Success) return cmd_res;
	SD_Response(SD_R2, (uint32_t *)SDCard.CSD); // Retrieve CSD register from the card

	// Parse CID/CSD registers
	SD_GetCardInfo();

	// Now card must be in stand-by mode, from this point it is possible to increase bus speed.

	// Configure SDIO peripheral clock
	// HW flow control disabled, Rising edge of SDIOCLK, 1-bit bus, Power saving disabled, SDIOCLK bypass disabled
//	SDMMC1->CLKCR = SD_BUS_1BIT | SDMMC_CLK_DIV_TRAN | SDMMC_CLKCR_CLKEN; // 1-bit bus, data transfer speed, clock enabled
	// HW flow control disabled, Rising edge of SDIOCLK, 1-bit bus, Power saving enabled, SDIOCLK bypass disabled
	SDMMC1->CLKCR = SD_BUS_1BIT | SDMMC_CLK_DIV_TRAN | SDMMC_CLKCR_CLKEN | SDMMC_CLKCR_PWRSAV;

	// Put the SD card in transfer mode
	SD_Cmd(SD_CMD_SEL_DESEL_CARD, SDCard.RCA << 16, SDMMC_RESP_SHORT); // CMD7
	cmd_res = SD_Response(SD_R1b, response);
	if (cmd_res != SDR_Success) return cmd_res;

	// Disable the pull-up resistor on CD/DAT3 pin of card
	// Send leading command for ACMD<n> command
	SD_Cmd(SD_CMD_APP_CMD, SDCard.RCA << 16, SDMMC_RESP_SHORT); // CMD55
	cmd_res = SD_Response(SD_R1, response);
	if (cmd_res != SDR_Success) return cmd_res;
	// Send SET_CLR_CARD_DETECT command
	SD_Cmd(SD_CMD_SET_CLR_CARD_DETECT, 0, SDMMC_RESP_SHORT); // ACMD42
	cmd_res = SD_Response(SD_R1, response);
	if (cmd_res != SDR_Success) return cmd_res;

	// Read the SCR register
	if (SDCard.Type != SDCT_MMC) {
		// MMC card doesn't support this feature
		// Warning: this function set block size to 8 bytes
		SD_GetSCR((uint32_t *)SDCard.SCR);

	}

	// For SDv1,SDv2 and MMC card must set block size (SDHC/SDXC always have fixed size 512bytes)
	if ((SDCard.Type == SDCT_SDSC_V1) || (SDCard.Type == SDCT_SDSC_V2) || (SDCard.Type == SDCT_MMC)) {
		SD_Cmd(SD_CMD_SET_BLOCKLEN, 512, SDMMC_RESP_SHORT); // CMD16
		cmd_res = SD_Response(SD_R1, response);
		if (cmd_res != SDR_Success) return SDR_SetBlockSizeFailed;
	}

/*
	// Turn off CRC checks - will it work in SDIO mode?
	SD_Cmd(SD_CMD_CRC_ON_OFF,0,SD_R1,resp); // CMD59
	// Turn on CRC checks
	SD_Cmd(SD_CMD_CRC_ON_OFF,1,SD_R1,resp); // CMD59
*/

	return SDR_Success;
}

// Set SDIO bus width
// input:
//   BW - bus width (one of SDMMC_BUS_xBIT constants)
// return: SDResult
// note: card must be in TRAN state and not locked, otherwise it will respond with 'illegal command'
SDResult SD_SetBusWidth(uint32_t BW) {
	SDResult cmd_res = SDR_Success;
	uint32_t reg;

	if (SDCard.Type != SDCT_MMC) {
		// Send leading command for ACMD<n> command
		SD_Cmd(SD_CMD_APP_CMD,SDCard.RCA << 16, SDMMC_RESP_SHORT); // CMD55
		cmd_res = SD_Response(SD_R1,&reg);
		if (cmd_res != SDR_Success) return cmd_res;

		// Set SET_BUS_WIDTH command
		SD_Cmd(SD_CMD_SET_BUS_WIDTH, (BW == SD_BUS_1BIT) ? 0x00000000 : 0x00000002, SDMMC_RESP_SHORT); // ACMD6
		cmd_res = SD_Response(SD_R1, &reg);
		if (cmd_res != SDR_Success) return cmd_res;
	} else {
		// MMC supports only 8-bit ?
	}

	// Clear bus width bits
	reg = SDMMC1->CLKCR & ~SDMMC_CLKCR_WIDBUS;

	// Configure bus width
	SDMMC1->CLKCR = reg | BW;

//	printf("------> BUS WIDTH %s-BIT <------\r\n",BW == SD_BUS_1BIT ? "1" : "4");

	return cmd_res;
}

// Set SDIO bus clock
// input:
//   clk_div - bus clock divider (0x00..0xff -> bus_clock = SDIOCLK / (clk_div + 2))
void SD_SetBusClock(uint32_t clk_div) {
	uint32_t reg;

	// Clear clock divider
	reg = SDMMC1->CLKCR & 0xffffff00;

	// Set new clock divider
	SDMMC1->CLKCR = reg | clk_div;
}

// Parse information about specific card
// note: CSD/CID register values already must be in the SDCard structure
void SD_GetCardInfo(void) {
	uint32_t dev_size, dev_size_mul, rd_block_len;

	// Parse the CSD register
	SDCard.CSDVer = SDCard.CSD[0] >> 6; // CSD version
	if (SDCard.Type != SDCT_MMC) {
		// SD
		SDCard.MaxBusClkFreq = SDCard.CSD[3];
		rd_block_len = SDCard.CSD[5] & 0x0f; // Max. read data block length
		if (SDCard.CSDVer == 0) {
			// CSD v1.00 (SDSCv1, SDSCv2)
			dev_size  = (uint32_t)(SDCard.CSD[6] & 0x03) << 10; // Device size
			dev_size |= (uint32_t)SDCard.CSD[7] << 2;
			dev_size |= (SDCard.CSD[8] & 0xc0) >> 6;
			dev_size_mul  = (SDCard.CSD[ 9] & 0x03) << 1; // Device size multiplier
			dev_size_mul |= (SDCard.CSD[10] & 0x80) >> 7;
			SDCard.BlockCount  = (dev_size + 1);
			SDCard.BlockCount *= (1 << (dev_size_mul + 2));
			SDCard.BlockSize =  1 << (rd_block_len);
		} else {
			// CSD v2.00 (SDHC, SDXC)
			dev_size  = (SDCard.CSD[7] & 0x3f) << 16;
			dev_size |=  SDCard.CSD[8] << 8;
			dev_size |=  SDCard.CSD[9];
			SDCard.BlockSize = 512;
			SDCard.BlockCount = dev_size + 1;
		}
		SDCard.Capacity = SDCard.BlockCount * SDCard.BlockSize;
	} else {
		// MMC
		SDCard.MaxBusClkFreq = SDCard.CSD[3];
		dev_size  = (uint32_t)(SDCard.CSD[6] & 0x03) << 8; // C_SIZE
		dev_size += (uint32_t)SDCard.CSD[7];
		dev_size <<= 2;
		dev_size += SDCard.CSD[8] >> 6;
		SDCard.BlockSize = 1 << (SDCard.CSD[5] & 0x0f); // MMC read block length
		dev_size_mul = ((SDCard.CSD[9] & 0x03) << 1) + ((SDCard.CSD[10] & 0x80) >> 7);
		SDCard.BlockCount = (dev_size + 1) * (1 << (dev_size_mul + 2));
		SDCard.Capacity = SDCard.BlockCount * SDCard.BlockSize;
	}

	// Parse the CID register
	if (SDCard.Type != SDCT_MMC) {
		// SD card
		SDCard.MID = SDCard.CID[0];
		SDCard.OID = (SDCard.CID[1] << 8) | SDCard.CID[2];
		SDCard.PNM[0] = SDCard.CID[3];
		SDCard.PNM[1] = SDCard.CID[4];
		SDCard.PNM[2] = SDCard.CID[5];
		SDCard.PNM[3] = SDCard.CID[6];
		SDCard.PNM[4] = SDCard.CID[7];
		SDCard.PRV = SDCard.CID[8];
		SDCard.PSN = (SDCard.CID[9] << 24) | (SDCard.CID[10] << 16) | (SDCard.CID[11] << 8) | SDCard.CID[12];
		SDCard.MDT = ((SDCard.CID[13] << 8) | SDCard.CID[14]) & 0x0fff;
	} else {
		// MMC
		SDCard.MID = 0x00;
		SDCard.OID = 0x0000;
		SDCard.PNM[0] = '*';
		SDCard.PNM[1] = 'M';
		SDCard.PNM[2] = 'M';
		SDCard.PNM[3] = 'C';
		SDCard.PNM[4] = '*';
		SDCard.PRV = 0;
		SDCard.PSN = 0x00000000;
		SDCard.MDT = 0x0000;
	}
}

// Abort an ongoing data transfer
// return: SDResult value
SDResult SD_StopTransfer(void) {
	uint32_t response;

	// Send STOP_TRANSMISSION command
	SD_Cmd(SD_CMD_STOP_TRANSMISSION, 0, SDMMC_RESP_SHORT); // CMD12

	return SD_Response(SD_R1, &response);
}

// Get current SD card state
// input:
//   pStatus - pointer to the variable for current card status (SD_CS_XXX values)
// return: SDResult value
SDResult SD_GetCardState(uint8_t *pStatus) {
	uint32_t response;

	// Send SEND_STATUS command
	SD_Cmd(SD_CMD_SEND_STATUS,SDCard.RCA << 16,SDMMC_RESP_SHORT); // CMD13
	SD_Response(SD_R1,&response);

	// Find out card status
	*pStatus = (response & 0x1e00) >> 9;

	// Check for errors
	return SD_GetError(response);
}

// Read block of data from the SD card
// input:
//   addr - address of the block to be read
//   pBuf - pointer to the buffer that will contain the received data
//   length - buffer length (must be multiple of 512)
// return: SDResult value
SDResult SD_ReadBlock(uint32_t addr, uint32_t *pBuf, uint32_t length) {
	SDResult cmd_res = SDR_Success;
	uint32_t response;
	uint32_t blk_count = length / 512; // Sectors in block
	register uint32_t STA; // to speed up SDIO flags checking
	uint32_t STA_mask; // mask for SDIO flags checking

	// Initialize the data control register
	SDMMC1->DCTRL = 0;

	// SDSC card uses byte unit address and
	// SDHC/SDXC cards use block unit address (1 unit = 512 bytes)
	// For SDHC card addr must be converted to block unit address
	if (SDCard.Type == SDCT_SDHC) addr >>= 9;

	// Clear the static SDIO flags
	SDMMC1->ICR = SDMMC_ICR_STATIC;

	if (blk_count > 1) {
		// Send READ_MULT_BLOCK command
		SD_Cmd(SD_CMD_READ_MULT_BLOCK, addr, SDMMC_RESP_SHORT); // CMD18
		// Prepare bit checking variable for multiple block transfer
		STA_mask = SDMMC_STA_RXOVERR | SDMMC_STA_DCRCFAIL | SDMMC_STA_DTIMEOUT | SDMMC_STA_STBITERR | SDMMC_STA_DATAEND;
	} else {
		// Send READ_SINGLE_BLOCK command
		SD_Cmd(SD_CMD_READ_SINGLE_BLOCK,addr,SDMMC_RESP_SHORT); // CMD17
		// Prepare bit checking variable for single block transfer
		STA_mask = SDMMC_STA_RXOVERR | SDMMC_STA_DCRCFAIL | SDMMC_STA_DTIMEOUT | SDMMC_STA_STBITERR | SDMMC_STA_DBCKEND;
	}
	cmd_res = SD_Response(SD_R1, &response);
	if (cmd_res != SDR_Success) return cmd_res;

	// Configure the SDIO data transfer
	SDMMC1->DTIMER = SDMMC_DATA_R_TIMEOUT; // Data read timeout
	SDMMC1->DLEN   = length; // Data length
	// Data transfer: block, card -> controller, size: 2^9 = 512bytes, enable transfer
	SDMMC1->DCTRL  = SDMMC_DCTRL_DTDIR | (9 << 4) | SDMMC_DCTRL_DTEN;

	// Receive a data block from the SDIO
	// ----> TIME CRITICAL SECTION BEGIN <----
	do {
		STA = SDMMC1->STA;
		if (STA & SDMMC_STA_RXFIFOHF) {
			// Receive FIFO half full, there are at least 8 words in it
			// This code is 80 bytes more than the 'for' loop, but faster
			*pBuf++ = SDMMC1->FIFO;
			*pBuf++ = SDMMC1->FIFO;
			*pBuf++ = SDMMC1->FIFO;
			*pBuf++ = SDMMC1->FIFO;
			*pBuf++ = SDMMC1->FIFO;
			*pBuf++ = SDMMC1->FIFO;
			*pBuf++ = SDMMC1->FIFO;
			*pBuf++ = SDMMC1->FIFO;
		}
	} while (!(STA & STA_mask));
	// <---- TIME CRITICAL SECTION END ---->

	// Send stop transmission command in case of multiple block transfer
	if ((SDCard.Type != SDCT_MMC) && (blk_count > 1)) cmd_res = SD_StopTransfer();

	// Check for errors
	if (STA & SDMMC_STA_DTIMEOUT) cmd_res = SDR_DataTimeout;
	if (STA & SDMMC_STA_DCRCFAIL) cmd_res = SDR_DataCRCFail;
	if (STA & SDMMC_STA_RXOVERR)  cmd_res = SDR_RXOverrun;
	if (STA & SDMMC_STA_STBITERR) cmd_res = SDR_StartBitError;

	// Read data remnant from RX FIFO (if there is still any data)
	while (SDMMC1->STA & SDMMC_STA_RXDAVL) *pBuf++ = SDMMC1->FIFO;

	// Clear the static SDIO flags
	SDMMC1->ICR = SDMMC_ICR_STATIC;

	return cmd_res;
}

// Write block of data to the SD card
// input:
//   addr - address of the block to be written
//   pBuf - pointer to the buffer that will contain the received data
//   length - buffer length (must be multiple of 512)
// return: SDResult value
SDResult SD_WriteBlock(uint32_t addr, uint32_t *pBuf, uint32_t length) {
	SDResult cmd_res = SDR_Success;
	uint32_t response; // SDIO command response
	uint32_t blk_count = length / 512; // Sectors in block
	register uint32_t STA; // To speed up SDIO flags checking
	uint32_t STA_mask; // Mask for SDIO flags checking
	uint32_t bsent = 0; // Counter of transferred bytes
	uint32_t w_left; // Words counter in last portion of data
	uint8_t card_state; // Card state
	uint32_t cntr;

	// Initialize the data control register
	SDMMC1->DCTRL = 0;

	// SDSC card uses byte unit address and
	// SDHC/SDXC cards use block unit address (1 unit = 512 bytes)
	// For SDHC card addr must be converted to block unit address
	if (SDCard.Type == SDCT_SDHC) addr >>= 9;

	if (blk_count > 1) {
		// Send WRITE_MULTIPLE_BLOCK command
		SD_Cmd(SD_CMD_WRITE_MULTIPLE_BLOCK,addr,SDMMC_RESP_SHORT); // CMD25
		// Prepare bit checking variable for multiple block transfer
		STA_mask = SDMMC_STA_TXUNDERR | SDMMC_STA_DCRCFAIL | SDMMC_STA_DTIMEOUT | SDMMC_STA_STBITERR | SDMMC_STA_DATAEND;
	} else {
		// Send WRITE_BLOCK command
		SD_Cmd(SD_CMD_WRITE_BLOCK,addr,SDMMC_RESP_SHORT); // CMD24
		// Prepare bit checking variable for single block transfer
		STA_mask = SDMMC_STA_TXUNDERR | SDMMC_STA_DCRCFAIL | SDMMC_STA_DTIMEOUT | SDMMC_STA_STBITERR | SDMMC_STA_DBCKEND;
	}
	cmd_res = SD_Response(SD_R1,&response);
	if (cmd_res != SDR_Success) return cmd_res;

	// Clear the static SDIO flags
	SDMMC1->ICR = SDMMC_ICR_STATIC;

	// Configure the SDIO data transfer
	SDMMC1->DTIMER = SDMMC_DATA_W_TIMEOUT; // Data write timeout
	SDMMC1->DLEN   = length; // Data length
	// Data transfer: block, controller -> card, size: 2^9 = 512bytes, enable transfer
	SDMMC1->DCTRL  = (9 << 4) | SDMMC_DCTRL_DTEN;

	// Transfer data block to the SDIO
	// ----> TIME CRITICAL SECTION BEGIN <----
	if (!(length % 32)) {
		// The block length is multiple of 32, use simplified and faster transfer
		do {
			STA = SDMMC1->STA;
			if ((STA & SDMMC_STA_TXFIFOHE) && (bsent < length)) {
				// TX FIFO half empty, at least 8 words can be written
				SDMMC1->FIFO = *pBuf++;
				SDMMC1->FIFO = *pBuf++;
				SDMMC1->FIFO = *pBuf++;
				SDMMC1->FIFO = *pBuf++;
				SDMMC1->FIFO = *pBuf++;
				SDMMC1->FIFO = *pBuf++;
				SDMMC1->FIFO = *pBuf++;
				SDMMC1->FIFO = *pBuf++;
				bsent += 32;
			}
		} while (!(STA & STA_mask));
	} else {
		// The block length is not a multiple of 32, so it is necessary to apply additional calculations
		do {
			STA = SDMMC1->STA;
			if ((STA & SDMMC_STA_TXFIFOHE) && (bsent < length)) {
				// TX FIFO half empty, at least 8 words can be written
				if (length - bsent < 32) {
					// Write last portion of data to the TX FIFO
					w_left = ((length - bsent) % 4 == 0) ? ((length - bsent) >> 2) : (((length - bsent) >> 2) + 1);
					for (cntr = 0; cntr < w_left; cntr++) SDMMC1->FIFO = *pBuf++;
					bsent += w_left << 2;
				} else {
					// Write 8 words to the TX FIFO
					SDMMC1->FIFO = *pBuf++;
					SDMMC1->FIFO = *pBuf++;
					SDMMC1->FIFO = *pBuf++;
					SDMMC1->FIFO = *pBuf++;
					SDMMC1->FIFO = *pBuf++;
					SDMMC1->FIFO = *pBuf++;
					SDMMC1->FIFO = *pBuf++;
					SDMMC1->FIFO = *pBuf++;
					bsent += 32;
				}
			}
		} while (!(STA & STA_mask));
	}
	// <---- TIME CRITICAL SECTION END ---->

	// Send stop transmission command in case of multiple block transfer
	if ((SDCard.Type != SDCT_MMC) && (blk_count > 1)) cmd_res = SD_StopTransfer();

	// Check for errors
	if (STA & SDMMC_STA_DTIMEOUT) cmd_res = SDR_DataTimeout;
	if (STA & SDMMC_STA_DCRCFAIL) cmd_res = SDR_DataCRCFail;
	if (STA & SDMMC_STA_TXUNDERR) cmd_res = SDR_TXUnderrun;
	if (STA & SDMMC_STA_STBITERR) cmd_res = SDR_StartBitError;

	// Wait till the card is in programming state
	do {
		if (SD_GetCardState(&card_state) != SDR_Success) break;
	} while ((card_state == SD_STATE_PRG) || (card_state == SD_STATE_RCV));

	// Clear the static SDIO flags
	SDMMC1->ICR = SDMMC_ICR_STATIC;

	return cmd_res;
}

