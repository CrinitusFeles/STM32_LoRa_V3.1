#ifndef INC_MBR_H_
#define INC_MBR_H_
#include "stm32l4xx.h"

typedef struct {
    uint8_t state;  // State of partition (0x00=Inactive, 0x80=Active)
    uint32_t CHS_start : 24;  // Beginning of partition: head, cylinder, sector numbers
    uint8_t partition_type;  // Partition type, e.g. FAT32=0x0B
    uint32_t CHS_end: 24;  // End of partition: head, cylinder, sector numbers
    uint32_t BPB_offset; // Number of sectors between the MBR and first sector in partition
    uint32_t partition_length;  // Number of sectors in the partition
} Partition;  // size = 16

typedef struct {
    // uint32_t *exec_code_ptr;  // size = 446
    Partition entries[4];
    uint16_t signature;  // 0x55AA
} MBR;  // Master boot record

#endif