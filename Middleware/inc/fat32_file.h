
#ifndef FAT32_FILE_
#define FAT32_FILE_

#include "stm32l4xx.h"
#include "fat32.h"
#include "fat32_status.h"
#include "sdio.h"

#define DELETED_FILE                0xE5
#define EMPTY_SPACE                 0

/*
Корневой каталог раздел на структуры FAT32_ByteDirEntryStruct по 32 байта, где можно имя файла/каталога в формате 8.3,
его размер, дату создания, атрибуты, а так-же номер первого кластера, с которого начинается данный файл. Если-же хвост
файла торчит снаружи выделенного кластера, номер следующего указывается уже в таблице FAT. Иначе, в таблице будет
прописан только маркер окончания EOC. Таким образом, таблица и корневой-каталог логически связаны между собой, и потеря
любого из них приведёт к краху всей FS.

Данная структура хранит в себе данные кластера корневой директории, в которой находится текущий файл.
*/
typedef struct sFILE_Meta{
    uint16_t dir_cluster;  // номер кластера корневой директории в таблице FAT.
    uint8_t dir_num;  // данные кластера корневой директории content разбиваются на структуры FAT32_ByteDirEntryStruct и данное поля указывает offset до структуры текущего файла.
    uint8_t content[BytesPerSector];  // the content of dir_num root sector
} FILE_Meta;


typedef struct sFAT32_File{
    char filename[11];
    uint32_t* cluster_nums;  // поиск последнего кластера файла происходит через прохождение всей цепочки адресов кластеров в таблице FAT. Данное поле хранит все номера кластеров текущего файла.
    uint16_t cluster_amount;  // len(cluster_nums)
    uint32_t last_cluster;
    uint32_t file_size;
    FILE_Meta meta;
    uint8_t content[512U];
    struct FAT32s* __parent;
    FAT32_Status status;

    uint32_t (*append)(struct sFAT32_File *file, char *data, uint16_t length);
    void (*read)(struct sFAT32_File *file, uint32_t start_cluster, char *data);

} FAT32_File;

FAT32_File *FILE_INIT(struct FAT32s *fat32);
FAT32_Status find_first_cluster(FAT32_File *file, char *short_filename);
FAT32_Status find_last_cluster(FAT32_File *file);
#endif