
#ifndef INC_FAT32_H_
#define INC_FAT32_H_
#include "stm32l4xx.h"
#define BytesPerSector              512
#include "fat32_file.h"
#include "fat32_status.h"
#include "sdio.h"
#define BPB_BytsPerSec_offset		11
#define BPB_SecPerClus_offset		13
#define BPB_RsvdSecCnt_offset		14
#define BPB_NumFATs_offset			16
#define BPB_FATSz32_offset			36
#define BPB_RootClus_offset			44
#define BPB_FSInfo_offset			48
#define BPB_BkBootSec_offset		50

#define FSI_FreeCount_offset        488
#define FSI_NextFree_offset         492
#define FSI_Signature               0x41615252

#define EMPTY_CLUSTER               0
#define LAST_CLUSTER                0x0FFFFFFF
#define BAD_CLUSTER                 0x0FFFFFF7

#define FIRST_ROOT_CLUSTER          2  // first root dir offset in fat table

#define EMPTY_CLUSTER_WAS_NOT_FOUND 0x1000000


typedef struct FAT32_BiosPartBlock{
    uint16_t BytesPerSec;	// mupltiple of 512
    uint8_t SecPerClus;		// 1 to 8
    uint16_t RsvdSecCnt;	// Number of reserved sectors in the reserved region of the volume starting at the first sector of the volume. This field is used to align the start of the data area to integral multiples of the cluster size with respect to the start of the partition/media.
    uint8_t NumFATs;		// The count of file allocation tables (FATs) on the volume. A value of 2 is recommended although a value of 1 is acceptable.
    uint32_t FATSz32;		// Sectors per FAT. This field is the FAT32 32-bit count of sectors occupied by one FAT.
    uint32_t RootClus;		// This is set to the cluster number of the first cluster of the root directory. This value should be 2 or the first usable (not bad) cluster available thereafter.
    uint16_t FSInfo;        // Sector number of FSINFO structure in the reserved area of the FAT32 volume. Usually 1.
    uint16_t BkBootSec;     // If non-zero, indicates the sector number in the reserved area of the volume of a copy of the boot record.
} FAT32_BiosPartBlock;


uint8_t sd_read_buf[BytesPerSector];

typedef enum DIR_Attr_type{
    ATTR_READ_ONLY = 0x01,
    ATTR_HIDDEN = 0x02,
    ATTR_SYSTEM = 0x04,
    ATTR_VOLUME_ID = 0x08,
    ATTR_DIRECTORY = 0x10,
    ATTR_ARCHIVE = 0x20,
    ATTR_LONG_NAME = ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID
} DIR_Attr_type;


typedef struct FAT32_ByteDirEntryStruct{
    char DIR_Name[11];  // “Short” file name limited to 11 characters (8.3 format).
    DIR_Attr_type DIR_Attr;
    uint8_t DIR_NTRes;  // Reserved. Must be set to 0.
    uint8_t DIR_CrtTimeTenth;  // Component of the file creation time. Count of tenths of a second. Valid range is: 0 <= DIR_CrtTimeTenth <= 199
    uint16_t DIR_CrtTime;  // Creation time. Granularity is 2 seconds.
    uint16_t DIR_CrtDate;  // Creation date.
    uint16_t DIR_LstAccDate;  // Last access date. Last access is defined as a read or write operation performed on the file/directory described by this entry. This field must be updated on file modification (write operation) and the date value must be equal to DIR_WrtDate.
    uint16_t DIR_FstClusHI;  // High word of first data cluster number for file/directory described by this entry. Only valid for volumes formatted FAT32. Must be set to 0 on volumes formatted FAT12/FAT16.
    uint16_t DIR_WrtTime;  // Last modification (write) time. Value must be equal to DIR_CrtTime at file creation.
    uint16_t DIR_WrtDate;  // Last modification (write) date. Value must be equal to DIR_CrtDate at file creation.
    uint16_t DIR_FstClusLO;  // Low word of first data cluster number for file/directory described by this entry.
    uint32_t DIR_FileSize;  // 32-bit quantity containing size in bytes of file/directory described by this entry.
} FAT32_ByteDirEntryStruct;


typedef struct FAT32_FSInfo{
    uint32_t FreeCount;  // Contains the last known free cluster count on the volume. The value 0xFFFFFFFF indicates the free count is not known. The contents of this field must be validated at volume mount (and subsequently maintained in memory by the file system driver implementation). It is recommended that this field contain an accurate count of the number of free clusters at volume dismount (during controlled dismount/shutdown).
    uint32_t NextFree;  // Contains the cluster number of the first available (free) cluster on the volume.
} FAT32_FSInfo;


typedef struct FAT32_FAT_Table{
    uint32_t content[BytesPerSector >> 2];
    uint16_t sector_num;
} FAT32_FAT_Table;



typedef struct FAT32s{
    FAT32_BiosPartBlock BPB;
    FAT32_FSInfo FSI;
    uint32_t root_sector;
    uint32_t boot_address;
    uint32_t FAT1_sector;  // address
    uint32_t FAT2_sector;  // address
    FAT32_FAT_Table table;

    FAT32_File* (*open)(struct FAT32s*, char *file_path);
    FAT32_Status (*create_file)(struct FAT32s*, char *file_path);
    // uint8_t* (*read_file)(char *filename, uint32_t sector, FAT32t *fat32, FAT32_File *file);
    // void (*write_file)(FAT32_File*);
} FAT32t;


FAT32t* FAT32();
SDResult read_data_cluster(FAT32t *this, uint32_t cluster_num, uint8_t *buffer);
SDResult write_data_cluster(FAT32t *this, uint32_t cluster_num, uint8_t *buffer);
SDResult read_FAT_sector(FAT32t *this, uint32_t sector_num);
SDResult write_FAT_sector(FAT32t *this, uint16_t sector_num, uint32_t *content);
FAT32_Status UpdateFSInfo(FAT32t *this, uint8_t decrement, uint32_t next_free_cluster);
uint8_t check_FSInfo_next_free(FAT32t *this);
uint32_t find_next_empty_cluster_from(FAT32t *this, uint32_t cluster_num);
#endif /* INC_FAT32_H_ */
