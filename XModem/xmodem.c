#include "xmodem.h"

#include "FreeRTOS.h"
#include "fifo.h"
#include "flash.h"
#include "task.h"
#include "string.h"
#include "xprintf.h"
#include "System.h"
#include "system_select.h"

/* Global variables. */
static uint8_t xmodem_packet_number = 1;         /**< Packet number counter. */
static uint32_t xmodem_actual_flash_address = 0; /**< Address where we have to write. */
static bool x_first_packet_received = false;     /**< First packet or not. */
char data_buffer[1024] = {0};
/* Local functions. */
static uint16_t xmodem_calc_crc(uint8_t *data, uint16_t length);
static xmodem_status xmodem_handle_packet(uint8_t size);
static xmodem_status xmodem_error_handler(uint8_t *error_number, uint8_t max_error_number);

void delay(uint32_t delay_ms) { vTaskDelay(delay_ms); }

uint8_t read_data(uint8_t *buffer, uint16_t size, uint32_t timeout_ms) {
    for (uint16_t i = 0; i < size && timeout_ms; i++) {
        while (FIFO_IS_EMPTY(fifo) && --timeout_ms) {
            delay(1);
        }
        if (timeout_ms) {
            buffer[i] = FIFO_FRONT(fifo);
            FIFO_POP(fifo);
        }
    }
    return timeout_ms ? 0 : 1;
}

void xmodem_receive(uint32_t write_addr) {
    volatile xmodem_status status = X_OK;
    uint8_t error_number = 0;

    x_first_packet_received = false;
    xmodem_packet_number = 1;
    xmodem_actual_flash_address = write_addr;

    /* Loop until there isn't any error (or until we jump to the user application). */
    xprintf(
        "\nStarting XModem flashing procedure.\n"
        "Open ExtraPutty or Minicom terminal with XModem mode to send binary "
        "file to MCU.\nrPress CTRL-C to abort procedure\n");
    while (X_OK == status) {
        uint8_t header = 0x00;

        /* Get the header from UART. */
        uint8_t comm_status = read_data(&header, 1, X_HEADER_TIMEOUT_MS);

        /* Spam the host (until we receive something) with ACSII "C", to notify it, we want to use CRC-16. */
        if ((0 != comm_status) && (false == x_first_packet_received)) {
            xprintf("%c", X_C);
        }
        /* Uart timeout or any other errors. */
        else if ((0 != comm_status) && (true == x_first_packet_received)) {
            status = xmodem_error_handler(&error_number, X_MAX_ERRORS);
        } else {
            /* Do nothing. */
        }

        /* The header can be: SOH, STX, EOT and CAN. */
        xmodem_status packet_status = X_ERROR;
        switch (header) {
            /* 128 or 1024 bytes of data. */
            case X_SOH:
            case X_STX:
                /* If the handling was successful, then send an ACK. */
                packet_status = xmodem_handle_packet(header);
                if (X_OK == packet_status) {
                    xprintf("%c", X_ACK);
                }
                /* If the error was flash related, then immediately set the error counter to max (graceful abort). */
                else if (X_ERROR_FLASH == packet_status) {
                    error_number = X_MAX_ERRORS;
                    status = xmodem_error_handler(&error_number, X_MAX_ERRORS);
                }
                /* Error while processing the packet, either send a NAK or do graceful abort. */
                else {
                    status = xmodem_error_handler(&error_number, X_MAX_ERRORS);
                }
                break;
            /* End of Transmission. */
            case X_EOT:
                /* ACK, feedback to user (as a text), then jump to user application. */
                xprintf("%c", X_ACK);
                delay(10);
                xprintf("\nFirmware updated!\n");
                xprintf("Jumping to user application...\n");
                // FLASH_jump_to_app();
                return;
            /* Abort from host. */
            case X_CAN:
                status = X_ERROR;
                break;
            case CTRL_C:
                xprintf("\nXModem aborted\n");
                return;
            default:
                /* Wrong header. */
                if (0 == comm_status) {
                    status = xmodem_error_handler(&error_number, X_MAX_ERRORS);
                }
                break;
        }
    }
}

/**
 * @brief   Calculates the CRC-16 for the input package.
 * @param   *data:  Array of the data which we want to calculate.
 * @param   length: Size of the data, either 128 or 1024 bytes.
 * @return  status: The calculated CRC.
 */
static uint16_t xmodem_calc_crc(uint8_t *data, uint16_t length) {
    uint16_t crc = 0;
    while (length) {
        length--;
        crc = crc ^ ((uint16_t)*data++ << 8);
        for (uint8_t i = 0; i < 8; i++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc = crc << 1;
            }
        }
    }
    return crc;
}

/**
 * @brief   This function handles the data packet we get from the xmodem protocol.
 * @param   header: SOH or STX.
 * @return  status: Report about the packet.
 */
static xmodem_status xmodem_handle_packet(uint8_t header) {
    xmodem_status status = X_OK;
    uint16_t size = 0;
    uint8_t comm_status = 0;
    /* 2 bytes for packet number, 1024 for data, 2 for CRC*/
    uint8_t received_packet_number[X_PACKET_NUMBER_SIZE] = {0};
    uint8_t received_packet_data[X_PACKET_1024_SIZE] = {0};
    uint8_t received_packet_crc[X_PACKET_CRC_SIZE] = {0};

    /* Get the size of the data. */
    if (X_SOH == header) {
        size = X_PACKET_128_SIZE;
    } else if (X_STX == header) {
        size = X_PACKET_1024_SIZE;
    } else {
        /* Wrong header type. This shoudn't be possible... */
        status |= X_ERROR;
        return status;
    }

    /* Get the packet number, data and CRC from UART. */
    comm_status |= read_data(received_packet_number, X_PACKET_NUMBER_SIZE, X_HEADER_TIMEOUT_MS);
    comm_status |= read_data(received_packet_data, size, X_HEADER_TIMEOUT_MS);
    comm_status |= read_data(received_packet_crc, X_PACKET_CRC_SIZE, X_HEADER_TIMEOUT_MS);
    memcpy(data_buffer + strlen(data_buffer), received_packet_data, strlen((char *)received_packet_data));
    /* Merge the two bytes of CRC. */
    uint16_t crc_received = ((uint16_t)received_packet_crc[X_PACKET_CRC_HIGH_INDEX] << 8) |
                            ((uint16_t)received_packet_crc[X_PACKET_CRC_LOW_INDEX]);
    /* We calculate it too. */
    uint16_t crc_calculated = xmodem_calc_crc(&received_packet_data[0], size);
    if (0 != comm_status) {
        status |= X_ERROR_UART;
        return status;
    }
    /* If it is the first packet, then erase the memory. */
    if ((X_OK == status) && (false == x_first_packet_received)) {
        uint8_t curr_block = HaveRunFlashBlockNum();
        if(curr_block){
            for(uint8_t sec_num = 0; sec_num < 31; sec_num++){
                if(FLASH_erase_page(sec_num) != FLASH_OK) {
                    status |= X_ERROR_FLASH;
                    break;
                }
            }
        } else {
            for(uint8_t sec_num = 32; sec_num < 63; sec_num++){
                if(FLASH_erase_page(sec_num) != FLASH_OK) {
                    status |= X_ERROR_FLASH;
                    break;
                }
            }
        }
        if(status == X_OK) {
            x_first_packet_received = true;
        }
    }

    /* Error handling and flashing. */
    if (X_OK == status) {
        if (xmodem_packet_number != received_packet_number[0]) {
            /* Packet number counter mismatch. */
            status |= X_ERROR_NUMBER;
        }
        if (255 != (received_packet_number[X_PACKET_NUMBER_INDEX] +
                    received_packet_number[X_PACKET_NUMBER_COMPLEMENT_INDEX])) {
            /* The sum of the packet number and packet number complement aren't 255. */
            /* The sum always has to be 255. */
            status |= X_ERROR_NUMBER;
        }
        if (crc_calculated != crc_received) {
            /* The calculated and received CRC are different. */
            status |= X_ERROR_CRC;
        }
    }

    /* Do the actual flashing (if there weren't any errors). */
    if ((X_OK == status) && (FLASH_OK != FLASH_write(xmodem_actual_flash_address,
                                                          (uint64_t*)&received_packet_data[0],
                                                          (uint32_t)size / 8))) {
        /* Flashing error. */
        status |= X_ERROR_FLASH;
    }

    /* Raise the packet number and the address counters (if there weren't any errors). */
    if (X_OK == status) {
        xmodem_packet_number++;
        xmodem_actual_flash_address += size;
    }

    return status;
}

/**
 * @brief   Handles the xmodem error.
 *          Raises the error counter, then if the number of the errors reached critical, do a graceful abort, otherwise
 * send a NAK.
 * @param   *error_number:    Number of current errors (passed as a pointer).
 * @param   max_error_number: Maximal allowed number of errors.
 * @return  status: X_ERROR in case of too many errors, X_OK otherwise.
 */
static xmodem_status xmodem_error_handler(uint8_t *error_number, uint8_t max_error_number) {
    xmodem_status status = X_OK;
    /* Raise the error counter. */
    (*error_number)++;
    /* If the counter reached the max value, then abort. */
    if ((*error_number) >= max_error_number) {
        /* Graceful abort. */
        xprintf("%c", X_CAN);
        xprintf("%c", X_CAN);
        xprintf("\nXModem uploading failed\n");
        status = X_ERROR;
    }
    /* Otherwise send a NAK for a repeat. */
    else {
        xprintf("%c", X_NAK);
        status = X_OK;
    }
    return status;
}