#include "fat32.h"
#include "fat32_file.h"
#include "string_utils.h"
#include "sdio.h"
#include "string.h"
#include "stdlib.h"

#define ALLOCATED_AMOUNT        4
#define CEIL(x, y) ((uint16_t)((x + y - 1) / x))
#define MOD(x, y) (x & (y - 1))  // y must be power of 2!

SDResult sd_last_result;
/*
Если при записи данные не помещаются в один кластер, то необходимо найти следующий пустой кластер и обновить таблицу FAT.
В таблице необходимо текущий "последний" кластер заменить на следующий свободный, а его уже указать как "последний"
кластер файла.
Так как в файловой системе есть копия таблицы FAT, то ее также необходимо обновить.
Когда следующий свободный кластер данных находится в другом кластере таблицы FAT, то необходимо сначала в текущем
кластере таблице записать адрес следующего свободного кластера данных, а потом в новом кластере таблицы записать метку
конца файла.
*/
SDResult update_file_fat(FAT32_File *file, uint32_t new_last_cluster){
    uint16_t approx_table_num = file->last_cluster >> 7;
    uint16_t new_approx_table_num = new_last_cluster >> 7;
    uint8_t in_FAT_sector_offset = (uint8_t)MOD(file->last_cluster, (file->__parent->BPB.BytesPerSec >> 2));
    uint8_t new_in_FAT_sector_offset = (uint8_t)MOD(new_last_cluster, (file->__parent->BPB.BytesPerSec >> 2));
    SDResult result;

    if(approx_table_num == new_approx_table_num){
        file->__parent->table.content[in_FAT_sector_offset] = new_last_cluster;
        file->__parent->table.content[new_in_FAT_sector_offset] = LAST_CLUSTER;
        result = write_FAT_sector(file->__parent, file->__parent->table.sector_num, file->__parent->table.content);
    } else {
        read_FAT_sector(file->__parent, approx_table_num);
        file->__parent->table.content[in_FAT_sector_offset] = new_last_cluster;
        result = write_FAT_sector(file->__parent, file->__parent->table.sector_num, file->__parent->table.content);
        read_FAT_sector(file->__parent, new_approx_table_num);
        file->__parent->table.content[new_in_FAT_sector_offset] = LAST_CLUSTER;
        result = write_FAT_sector(file->__parent, file->__parent->table.sector_num, file->__parent->table.content);
    }
    file->last_cluster = new_last_cluster;
    if((uint8_t)(MOD((file->cluster_amount + 1), ALLOCATED_AMOUNT)) == 0){
        file->cluster_nums = (uint32_t*)realloc(file->cluster_nums, (file->cluster_amount + 5) * sizeof(uint32_t));
    }
    file->cluster_amount++;
    file->cluster_nums[file->cluster_amount] = new_last_cluster;

    return result;
}

/*
Данная функция вызывается только в процессе записи большого объема данных на карту. Также она отвечает за обновление
данных FSInfo.

Данная функция не заметит, если в предшествующей таблице FAT будет удален файл. Остается надеяться, что драйвер, который
удалял файл позаботился об обновлении данных FSInfo. В противном случае, нужно сканировать всю область FAT с начала.

Сначала определяем в каком кластере таблицы FAT находится укзатель на последний кластер файла. Далее начиная с этого
кластера таблицы ищем в ней пустой кластер. Если в текущей таблице не нашли пустых кластеров, то ищем в следующей.
*/
uint32_t find_next_empty_cluster(FAT32_File *file){
    if(check_FSInfo_next_free(file->__parent))
        return file->__parent->FSI.NextFree;
    // обрабатываем случай, когда в FSInfo лежат неправильные данные
    return find_next_empty_cluster_from(file->__parent, file->last_cluster);
}


/*
Поиск файла в определенной корневой директории производится путем сравнения имени файла с записями корневой директории.
Файлы, помеченные как сразу пропускаются. Как только встречаем пустую область, то считаем, что в данной корневой
директории файл отсутствует и нужно вызывать эту функцию с адресом следующей корневой директории.
*/
FAT32_Status search_file_in_rootdir(FAT32_File *file, uint16_t root_cluster, char *short_filename){
    FAT32_ByteDirEntryStruct root_dir;
    sd_last_result = read_data_cluster(file->__parent, root_cluster, file->meta.content);
    file->meta.dir_cluster = root_cluster;
    uint8_t dirs_per_cluster = file->__parent->BPB.BytesPerSec >> 5;
    for(uint8_t dir_counter = 0; dir_counter < dirs_per_cluster; dir_counter++){
        // root_dir = *(FAT32_ByteDirEntryStruct*)(file->meta.content + (sizeof(FAT32_ByteDirEntryStruct) * dir_counter));
        memcpy(&root_dir, (FAT32_ByteDirEntryStruct*)(file->meta.content + (sizeof(FAT32_ByteDirEntryStruct) * dir_counter)), sizeof(FAT32_ByteDirEntryStruct));
        if(root_dir.DIR_Name[0] == EMPTY_SPACE)
            return FILE_NOT_FOUND;
        if(root_dir.DIR_Name[0] == DELETED_FILE)
            continue;
        if(compare_short_file_names(root_dir.DIR_Name, short_filename)){
            memcpy(file->filename, root_dir.DIR_Name, 11);
            file->last_cluster = (root_dir.DIR_FstClusHI << 16) + root_dir.DIR_FstClusLO;
            file->file_size = root_dir.DIR_FileSize;
            file->meta.dir_num = dir_counter;
            file->cluster_amount = 0;
            return OK;
        }
    }
    return FILE_NOT_FOUND;
}

/*
В каждом секторе таблицы FAT хранятся 4 байтные значения от N до N + (128 * SecPerCluster), где N - номер кластера таблицы.
Из данных корневой директории мы получаем значение первого кластера, в котором лежит файл, но для этого нужно найти
нужный кластер корневую директорию, которая может быть разбросана по всей карте памяти.

В отличии от корневой директории, таблица FAT находится в определенном месте и адрес следующего кластера таблицы
увеличивается последовательно. Поэтому, зная адрес кластера, можно сразу вычислить в каком кластере таблицы он находится
и не читать каждый сектор таблицы. FAT_cluster = file_cluster / (128 * SecPerCluster)

Начинаем искать начало файла в первом кластере корневой директории, который лежит во втором кластере области данных.
Если в таблице FAT написано, что корневая директория достаточно маленькая, что занимает всего один сектор и
при этом файл в корневой директории не найден, то файла не существует. Аналогично, если мы добрались до последнего
кластера корневой директории.
Если в корневая директория занимает больше одного кластера, то в таблице FAT по отступу 2 будет лежать адрес
следующего кластера корневой директории и поиск файла стоит продолжать уже там.
Если номер кластера выходит за пределы текущей таблицы FAT, то нужно прочитать нужный кластер таблицы и искать
продолжение уже там.
*/
FAT32_Status find_first_cluster(FAT32_File *file, char *short_filename){
    uint32_t next_root_cluster = FIRST_ROOT_CLUSTER;
    uint32_t last_root_cluster = FIRST_ROOT_CLUSTER;
    uint16_t FAT_sector = 0;

    sd_last_result = read_FAT_sector(file->__parent, FAT_sector);
    for(uint32_t i = 0; i < file->__parent->BPB.FATSz32; i++){
        last_root_cluster = next_root_cluster;
        if(search_file_in_rootdir(file, last_root_cluster, short_filename) == OK)
            return OK;

        if(last_root_cluster > (uint32_t)(128 * (FAT_sector + 1))){
            FAT_sector = last_root_cluster >> 7;  // FAT table consists of 4 byte values
            sd_last_result = read_FAT_sector(file->__parent, FAT_sector);
        }
        next_root_cluster = *(file->__parent->table.content + last_root_cluster);
        if(next_root_cluster == EMPTY_CLUSTER)
            return NO_FIRST_SECTOR;
    }
    return NO_FIRST_SECTOR;
}

/*
Зная номер первого кластера файла можно посчитать, в каком кластере таблицы FAT он лежит и даже его offset. Если файл
достаточно большой и занимает настолько много кластеров, что не влезает в один кластер таблицы FAT, то нужно продолжать
искать в следующем кластере таблицы.

Также может быть ситуация когда рядом с файлом, который занимает несколько кластеров лежит огромный файл на несколько
кластеров таблицы FAT. В таком случае продолжение файла придется искать в других кластерах таблицы FAT.

Для поиска последнего кластера файла придется последовательно пройтись по всей цепочке номеров кластеров файла,
записанных в таблицах FAT. Если в таблице FAT цепочка адресов файла заканчивается маркером EOC, то обнаружение в ней
пустого кластера говорит о неправильном алгоритме поиска или о повреждении файловой системы.
*/
FAT32_Status find_last_cluster(FAT32_File *file){
    uint16_t allocated_clusters = 1;
    uint32_t prev_cluster = 0;
    uint16_t FAT_sector = 0;
    uint8_t in_FAT_sector_offset = 0;
    if(file->last_cluster == 0 && file->file_size == 0){
        file->last_cluster = find_next_empty_cluster(file);
    }
    uint32_t next_cluster = file->last_cluster;  // at the beginning it is first cluster
    file->cluster_nums = (uint32_t*)malloc(sizeof(uint32_t));

    for(uint32_t sector_count = 1; sector_count <= file->__parent->BPB.FATSz32; sector_count++){  // sector_count unused
        FAT_sector = next_cluster >> 7;
        sd_last_result = read_FAT_sector(file->__parent, FAT_sector);
        for(uint16_t i = 0; i < (file->__parent->BPB.BytesPerSec >> 2); i++){
            prev_cluster = next_cluster;
            uint32_t new_FAT_sector = next_cluster >> 7;
            if(new_FAT_sector != FAT_sector){  // when the next part of file is far away from previous in FAT table
                sd_last_result = read_FAT_sector(file->__parent, new_FAT_sector);
            }
            in_FAT_sector_offset = (uint8_t)MOD(next_cluster, (file->__parent->BPB.BytesPerSec >> 2));
            next_cluster = *(file->__parent->table.content + in_FAT_sector_offset);
            file->cluster_amount++;
            if(next_cluster == LAST_CLUSTER){
                file->last_cluster = prev_cluster;
                return OK;
            }
            if(next_cluster == EMPTY_CLUSTER){
                if(file->file_size == 0){
                    file->cluster_nums[i] = file->last_cluster;
                    return OK;
                }
                return MISSED_LAST_CLUSTER;
            }
            if(next_cluster == BAD_CLUSTER){
                return FAT_BAD_CLUSTER;  // а вот тут не понятно, как искать следующий кластер
            }
            if(i * sector_count >= allocated_clusters){
                file->cluster_nums = (uint32_t*)realloc(file->cluster_nums, (allocated_clusters + ALLOCATED_AMOUNT) * sizeof(uint32_t));
                allocated_clusters += ALLOCATED_AMOUNT;
            }
            file->cluster_nums[i * sector_count] = next_cluster;
            if (in_FAT_sector_offset == (file->__parent->BPB.BytesPerSec >> 2) - 1)  // проверка на последний кластер была выше
                break;  // номер кластера выходит за пределы допустимых номеров текущей таблицы FAT и нужно прочитать следующую
        }
    }
    return NO_LAST_SECTOR;
}


/*
После записи данных в файл необходимо также обновить его метаданные в корневом каталоге о размере и времени
редактирования. При чтении файла драйвер будет смотреть на данные о размере файла и если там будет указано,
меньший размер, то остальные данные просто не будут отображаться.
*/
void update_file_meta(FAT32_File *file){
    // *(uint32_t *)(file->meta.content + file->meta.dir_num * sizeof(FAT32_ByteDirEntryStruct) - sizeof(uint32_t)) = file->file_size;
    uint32_t *size_ptr = (uint32_t *)(file->meta.content + (file->meta.dir_num + 1) * sizeof(FAT32_ByteDirEntryStruct) - sizeof(uint32_t));
    uint32_t *ptr = &file->file_size;
    memcpy(size_ptr, ptr, sizeof(uint32_t));
    write_data_cluster(file->__parent, file->meta.dir_cluster, file->meta.content);
}

uint32_t append(FAT32_File *file, char *data, uint16_t length){
    uint16_t next_free = file->last_cluster;
    uint16_t wr_count = 0;  // количество записанных байт
    SDResult result = read_data_cluster(file->__parent, file->last_cluster, file->content);
    uint16_t empty_ptr = (uint16_t)(MOD(file->file_size, file->__parent->BPB.BytesPerSec));  // отступ в кластере данных, откуда начинается свободная область
    if(result != SDR_Success) return 0;
    if(file->file_size == 0){
        *(uint16_t*)(file->meta.content + (sizeof(FAT32_ByteDirEntryStruct) * file->meta.dir_num) + 26) = file->last_cluster & 0xFF;
        *(uint16_t*)(file->meta.content + (sizeof(FAT32_ByteDirEntryStruct) * file->meta.dir_num) + 20) = file->last_cluster >> 8;
        write_data_cluster(file->__parent, file->meta.dir_cluster, file->meta.content);

        file->__parent->table.content[file->last_cluster] = LAST_CLUSTER;
        result = write_FAT_sector(file->__parent, file->__parent->table.sector_num, file->__parent->table.content);
        UpdateFSInfo(file->__parent, 1, next_free + 1);
    }

    for(uint16_t i = empty_ptr; wr_count < length; i++){
        // если дошли до конца кластера или изначально этот кластер был заполнен
        if(((i > (file->__parent->BPB.BytesPerSec - 1)) || ((i == 0) && (file->content[0] != 0))) && (file->file_size > 0)){
            if(i != 0){  // если дошли до конца кластера и успели записать что-то в текущий, то нужно обновить текущий и искать следующий пустой кластер
                result = write_data_cluster(file->__parent, next_free, file->content);
                if(result != SDR_Success) return 0;  // не смогли записать данные на карту
            }
            i = 0;  // в новом кластере будем записывать данные с начала
            next_free = find_next_empty_cluster(file);
            if(next_free == 0) return wr_count;  // не нашли пустой кластер
            if(update_file_fat(file, next_free) != SDR_Success) return 0; // не смогли записать данные на карту
            if(UpdateFSInfo(file->__parent, 1, next_free + 1) != OK) return 0; // не смогли записать данные на карту
            // нам не важно, что может лежать в кластере, помеченном как "свободный", поэтому затираем наш буфер нулями
            // и впоследствии, те данные, что были на карте в этом кластере будут перезаписаны нашим буфером
            memset(file->content, 0, file->__parent->BPB.BytesPerSec);
        }
        file->content[i] = data[wr_count]; // пишем данные в пустую свободную область кластера
        wr_count++;
    }
    result = write_data_cluster(file->__parent, next_free, file->content);
    // file->file_size += wr_count;
    uint16_t sum = empty_ptr + wr_count;
    if(sum > file->__parent->BPB.BytesPerSec)
        sum -= file->__parent->BPB.BytesPerSec;
    file->file_size = (file->cluster_amount - 1) * file->__parent->BPB.BytesPerSec * file->__parent->BPB.SecPerClus + sum;
    update_file_meta(file);
    return wr_count;
}

// void read(FAT32_File *file, uint32_t start_cluster, char *result_buffer){

// }

FAT32_File FILE_INIT(struct FAT32s *fat32){
    FAT32_File file;
    file.__parent = fat32;
    file.append = append;
    // file.read = read;
    return file;
}

