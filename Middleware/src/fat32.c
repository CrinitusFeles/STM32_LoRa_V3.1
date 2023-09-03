#include "fat32.h"
#include "sdio.h"
#include "stdlib.h"
#include <string.h>
#include "string_utils.h"
#include "delay.h"

#define MOD(x, y) (x & (y - 1))  // y must be power of 2!
#define has_boot_sector(content) (content[0] == 0 && content[1] == 0 && content[2] == 0 && SDCard.Type == SDCT_SDHC)

SDResult ReadBiosPartBlock(uint32_t boot_address, FAT32_BiosPartBlock* BPB){
    sd_last_result = SD_ReadBlock(boot_address * SDCard.BlockSize, (uint32_t *)sd_read_buf, 512);
    BPB->BytesPerSec = *(uint16_t*)(sd_read_buf + BPB_BytsPerSec_offset);
    BPB->SecPerClus = sd_read_buf[BPB_SecPerClus_offset];
    BPB->RsvdSecCnt = *(uint16_t*)(sd_read_buf + BPB_RsvdSecCnt_offset);
    BPB->NumFATs = sd_read_buf[BPB_NumFATs_offset];
    BPB->FATSz32 = *(uint32_t*)(sd_read_buf + BPB_FATSz32_offset);
    BPB->RootClus = *(uint32_t*)(sd_read_buf + BPB_RootClus_offset);
    BPB->FSInfo = *(uint16_t*)(sd_read_buf + BPB_FSInfo_offset);
    BPB->BkBootSec = *(uint16_t*)(sd_read_buf + BPB_BkBootSec_offset);
    return sd_last_result;
}


int8_t ReadFSInfoSector(FAT32t *fat32){
    sd_last_result = SD_ReadBlock(fat32->BPB.FSInfo * fat32->BPB.BytesPerSec, (uint32_t *)sd_read_buf, 512);
    if(*(uint32_t*)(sd_read_buf) != FSI_Signature) return -1;
    fat32->FSI.FreeCount = *(uint32_t*)(sd_read_buf + FSI_FreeCount_offset);
    fat32->FSI.NextFree = *(uint32_t*)(sd_read_buf + FSI_NextFree_offset);
    return 0;
}


FAT32_Status FAT32_init(FAT32t *fat32){
	sd_last_result = SD_ReadBlock(0, (uint32_t *)sd_read_buf, 512);
    fat32->boot_address = 0;
	if(has_boot_sector(sd_read_buf)){
		if(sd_read_buf[0x1C7] != 0){
			fat32->boot_address = (sd_read_buf[0x1C7] << 8 | sd_read_buf[0x1C6]);
            ReadBiosPartBlock(fat32->boot_address, &fat32->BPB);
		}
		else{
            return BOOT_SECTOR_NOT_FOUND;
		}
	} else {
        ReadBiosPartBlock(fat32->boot_address, &fat32->BPB);
    }
    fat32->FAT1_sector = fat32->boot_address + fat32->BPB.RsvdSecCnt;
    fat32->FAT2_sector = fat32->BPB.FATSz32 + fat32->FAT1_sector;
    fat32->root_sector = fat32->BPB.FATSz32 * fat32->BPB.NumFATs + fat32->FAT1_sector;
    fat32->table.sector_num = -1;
    ReadFSInfoSector(fat32);
    return OK;
}


SDResult read_data_cluster(FAT32t *this, uint32_t cluster_num, uint8_t *buffer){
    uint32_t first_data_sector = (cluster_num - FIRST_ROOT_CLUSTER) * this->BPB.SecPerClus + this->root_sector;
    return SD_ReadBlock(first_data_sector * this->BPB.BytesPerSec, (uint32_t *)buffer, this->BPB.BytesPerSec);
}


SDResult write_data_cluster(FAT32t *this, uint32_t cluster_num, uint8_t *buffer){
    uint32_t first_data_sector = (cluster_num - FIRST_ROOT_CLUSTER) * this->BPB.SecPerClus + this->root_sector;
    return SD_WriteBlock(first_data_sector * this->BPB.BytesPerSec, (uint32_t *)buffer, this->BPB.BytesPerSec);
}


SDResult read_FAT_sector(FAT32t *this, uint32_t sector_num){
    if(sector_num != this->table.sector_num){  // читаем только те сектора, что не находятся сейчас в буфере
        this->table.sector_num = sector_num;
        return SD_ReadBlock((this->FAT1_sector + sector_num) * this->BPB.BytesPerSec, this->table.content, this->BPB.BytesPerSec);
    }
    return SDR_Success;
}

SDResult write_FAT_sector(FAT32t *this, uint16_t sector_num, uint32_t *content){
    uint32_t addr = (this->FAT1_sector + sector_num) * this->BPB.BytesPerSec;
    SDResult result = SD_WriteBlock(addr, content, this->BPB.BytesPerSec);
    if(result != SDR_Success) return result;
    addr = (this->FAT2_sector + sector_num) * this->BPB.BytesPerSec;
    result = SD_WriteBlock(addr, content, this->BPB.BytesPerSec);
    return result;
}

uint8_t check_FSInfo_next_free(FAT32t *this){
    uint32_t next_free = this->FSI.NextFree;
    uint32_t approx_table_num = next_free >> 7;
    uint8_t in_FAT_sector_offset = (uint8_t)MOD(next_free, (this->BPB.BytesPerSec >> 2));
    read_FAT_sector(this, approx_table_num);
    if(this->table.content[in_FAT_sector_offset] == EMPTY_CLUSTER)
        return 1;
    return 0;
}

uint32_t find_next_empty_cluster_from(FAT32t *this, uint32_t cluster_num){
    uint32_t approx_table_num = 0;
    uint8_t in_FAT_sector_offset = 0;
    for(uint32_t cluster_count = cluster_num; cluster_count < this->BPB.FATSz32; cluster_count++){
        approx_table_num = cluster_count >> 7;
        read_FAT_sector(this, approx_table_num);
        in_FAT_sector_offset = (uint8_t)MOD(cluster_count, (this->BPB.BytesPerSec >> 2));
        for(uint8_t i = in_FAT_sector_offset; i < (this->BPB.BytesPerSec >> 2); i++){  // ищем с начала таблицы, т.к. предыдущий файл мог быть удален
            if(this->table.content[i] == EMPTY_CLUSTER)
                return i + approx_table_num * (this->BPB.BytesPerSec >> 2);
        }
    }
    return 0;
}

FAT32_Status UpdateFSInfo(FAT32t *this, uint8_t decrement, uint32_t next_free_cluster){
    this->FSI.FreeCount -= decrement;
    uint32_t next_free = find_next_empty_cluster_from(this, next_free_cluster);
    if(next_free == 0)
        return NO_FREE_CLUSTER;
    this->FSI.NextFree = next_free_cluster == next_free ? next_free_cluster : next_free;
    sd_last_result = SD_ReadBlock(this->BPB.FSInfo * this->BPB.BytesPerSec, (uint32_t *)sd_read_buf, this->BPB.BytesPerSec);
    if(sd_last_result != SDR_Success)
        return SD_READ_ERROR;
    if(*(uint32_t*)(sd_read_buf) != FSI_Signature)
        return FSI_SIGNATURE_ERROR;
    *(uint32_t*)(sd_read_buf + FSI_FreeCount_offset) = this->FSI.FreeCount;
    *(uint32_t*)(sd_read_buf + FSI_NextFree_offset) = next_free_cluster;
    sd_last_result = SD_WriteBlock(this->BPB.FSInfo * this->BPB.BytesPerSec, (uint32_t *)sd_read_buf, this->BPB.BytesPerSec);
    if(sd_last_result != SDR_Success)
        return SD_WRITE_ERROR;
    return OK;
}

FAT32_File open_file(struct FAT32s* fat32, char *filename){
    FAT32_File file = FILE_INIT(fat32);
    char converted_name[11];
    if(convert_file_name(filename, converted_name) != 0){
        file.status = INCORRECT_FILE_NAME;
        return file;
    }

    FAT32_Status result = find_first_cluster(&file, converted_name);
    if(result != OK){
        file.status = result;
        return file;
    }
    result = find_last_cluster(&file);
    file.status = result;
    return file;
}

/*
Для создания файла необходимо сначала найти свободную область в корневой директории и свободный сектор для файла в
таблице FAT. В свободный сектор корневой директории записать структуру файла, в которой первым кластером указывается
ранее найденный свободный кластер в пространстве данных.
*/
FAT32_Status create_file(FAT32t *this, char* filename){
    uint32_t next_free = 0;
    if(check_FSInfo_next_free(this))
        next_free = this->FSI.NextFree;
    else
        next_free = find_next_empty_cluster_from(this, 3);
    if(next_free == 0)
        return NO_FREE_CLUSTER;
    FAT32_ByteDirEntryStruct root_dir;
    memset(&root_dir, 0, sizeof(FAT32_ByteDirEntryStruct));
    strcpy(root_dir.DIR_Name, filename);
    root_dir.DIR_Attr = ATTR_ARCHIVE;
    root_dir.DIR_FstClusLO = next_free & 0xFF;
    root_dir.DIR_FstClusHI = next_free >> 8;

    return OK;
}


FAT32t FAT32(){
    FAT32t this;
    FAT32_Status result = FAT32_init(&this);

    this.open = open_file;
    this.create_file = create_file;
    return this;
}

