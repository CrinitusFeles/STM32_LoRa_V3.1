
#include "string_utils.h"

int convert_file_name(char *filename, char *converted_name){
    /*
    “foo.bar”            -> “FOO     BAR”
    “FOO.BAR”            -> “FOO     BAR”
    “Foo.Bar”            -> “FOO     BAR”
    “foo”                -> “FOO        “
    “foo.”               -> “FOO        “
    “PICKLE.A”           -> “PICKLE  A  “
    “prettybg.big”       -> “PRETTYBGBIG”
    “.big”               -> illegal, DIR_Name[0] cannot be 0x20
    */
    int8_t dot_position = -1;
    uint8_t initial_length = 0;
    for(uint8_t i = 0; filename[i] != '\0'; i++){
        initial_length++;
    }
    for(uint8_t i = 0; filename[i] != '\0'; i++){
        if(filename[i] == '.'){
            dot_position = i;
            break;
        }
    }

    if(dot_position == 0 || dot_position > 8)
        return -1;
    if(dot_position == -1)
        dot_position = initial_length;

    for(uint8_t i = 0; i < 8; i++){ // convert file name
        if(i < dot_position){
            converted_name[i] = filename[i];
            if(filename[i] >= 'a' && filename[i] <= 'z'){
                converted_name[i] = filename[i] - 32;
            }
        } else {
            converted_name[i] = ' ';
        }
    }

    for(uint8_t i = 0; i < 3; i++){  // convert extension
        char next_char = filename[dot_position + i + 1];
        if(dot_position + i + 1 < initial_length){
            converted_name[8 + i] = next_char;
            if(next_char >= 'a' && next_char <= 'z'){
                converted_name[8 + i] = next_char - 32;
            }
        } else {
            converted_name[8 + i] = ' ';
        }
    }
    return 0;
}


uint8_t compare_short_file_names(char *fname1, char *fname2){
    for(uint8_t i = 0; i < 11; i++){
        if(fname1[i] != fname2[i]) return 0;
    }
    return 1;
}