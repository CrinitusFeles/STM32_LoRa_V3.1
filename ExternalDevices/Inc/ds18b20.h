#ifndef INC_DS18B20_H_
#define INC_DS18B20_H_

#include "stm32l4xx.h"
#include "one_wire.h"
#include "stddef.h"

#define DS18B20_CONVERTION_TIMEOUT_MS     1000

// ---------------------- ROM Commands -----------------------------

// When a system is initially powered up, the master must
// identify the ROM codes of all slave devices on the bus,
// which allows the master to determine the number of
// slaves and their device types. The master learns the
// ROM codes through a process of elimination that requires
// the master to perform a Search ROM cycle (i.e., Search
// ROM command followed by data exchange) as many
// times as necessary to identify all of the slave devices.
#define DS18B20_SEARCH_ROM                0xF0

// This command can only be used when there is one slave
// on the bus. It allows the bus master to read the slave’s
// 64-bit ROM code without using the Search ROM procedure.
// If this command is used when there is more than
// one slave present on the bus, a data collision will occur
// when all the slaves attempt to respond at the same time.
#define DS18B20_READ_ROM                  0x33

// The match ROM command followed by a 64-bit ROM
// code sequence allows the bus master to address a
// specific slave device on a multidrop or single-drop bus.
// Only the slave that exactly matches the 64-bit ROM code
// sequence will respond to the function command issued
// by the master; all other slaves on the bus will wait for a
// reset pulse.
#define DS18B20_MATCH_ROM                 0x55

// The master can use this command to address all devices on the bus simultaneously without
// sending out any ROM code information. For example, the master can make all DS18B20s on
// the bus perform simultaneous temperature conversions by issuing a Skip ROM command
// followed by a Convert T [44h] command
#define DS18B20_SKIP_ROM                  0xCC

// The operation of this command is identical to the operation of the Search ROM command
// except that only slaves with a set alarm flag will respond. This command allows the
// master device to determine if any DS18B20s experienced an alarm condition during the
// most recent temperature conversion
#define DS18B20_ALARM_SEARCH              0xEC
// ================================================================

// --------------------- Function Commands ------------------------

// Initiates a single temperature conversion. Following the conversion, the resulting
// thermal data is stored in the 2-byte temperature register in the scratchpad memory
// and the DS18B20 returns to its low-power idle state.
#define DS18B20_CONVERT_TEMP              0x44

// Allows the master to write 3 bytes of data to the DS18B20’s scratchpad. The first
// data byte is written into the TH register (byte 2 of the scratchpad), the second
// byte is written into the TL register (byte 3), and the third byte is written into
// the configuration register (byte 4). Data must be transmitted least significant
// bit first. All three bytes MUST be written before the master issues a reset,
// or the data may be corrupted.
#define DS18B20_WRITE_SCRATCHPAD          0x4E

// Allows the master to read the contents of the scratchpad. The data transfer
// starts with the least significant bit of byte 0 and continues through the scratchpad
// until the 9th byte (byte 8 – CRC) is read. The master may issue a reset to terminate
// reading at any time if only part of the scratchpad data is needed.
#define DS18B20_READ_SCRATCHPAD           0xBE

//Copies the contents of the scratchpad TH, TL and configuration
//registers (bytes 2, 3 and 4) to EEPROM.
#define DS18B20_COPY_SCRATCHPAD           0x48

// Recalls the alarm trigger values (TH and TL) and configuration data from EEPROM and
// places the data in bytes 2, 3, and 4, respectively, in the scratchpad memory.
#define DS18B20_RECALL_E2                 0xB8

// Transmits supply status to master.
#define DS18B20_READ_POWER_SUPPLY         0xB4

// ===============================================================
typedef struct DS18B20_Scratchpad{
    uint16_t temperature;
    uint8_t tempLimitLow;
    uint8_t tempLimitHigh;
    uint8_t configRegister;
    uint8_t reserved[3];
    uint8_t crc;
} DS18B20_Scratchpad;

typedef struct DS18B20{
    OneWire *ow;
    uint8_t isConnected;
    RomCode *serialNumber;
    DS18B20_Scratchpad scratchpad;
    float temperature;
} DS18B20;



void DS18B20_ReadROM(DS18B20 *sensor);
uint8_t DS18B20_Init(DS18B20 *sensor, OneWire *ow);
uint8_t DS18B20_ReadScratchpad(DS18B20 *sensor);
uint8_t DS18B20_ReadTemp(DS18B20 *sensor);
uint8_t DS18B20_StartTempMeas(OneWire *ow);
uint16_t DS18B20_ReadTemperature(DS18B20 *sensor);
uint16_t DS18B20_array_to_str(DS18B20 *sensors, size_t length, char *buf, size_t buf_size, uint16_t offset);

void DS18B20_copy_temperature_list(DS18B20 *sensors, float *buffer, uint8_t size);
void DS18B20_copy_ids_list(DS18B20 *sensors, uint64_t *buffer, uint8_t size);
#endif  //INC_DS18B20_H_