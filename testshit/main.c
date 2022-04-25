#include "defs.h"

// int char2int(char input)
// {
//     if (input >= '0' && input <= '9')
//         return input - '0';
//     if (input >= 'A' && input <= 'F')
//         return input - 'A' + 10;
//     if (input >= 'a' && input <= 'f')
//         return input - 'a' + 10;
//     eprintf("Error: Malformed input to \"%s\" (%X).\n", __func__, input);
//     return -1;
// }

// void hex2bytes(const char* input, void* output, int input_length)
// {
//     for (int i = 0; i < input_length; i += 2) {
//         ((uint8_t*) output)[i / 2] = char2int(input[i]) * 16 + char2int(input[i + 1]);
//     }
// }

// call like a main()
// WemInformation* bnk_extract(int argc, char* argv[]);
BinaryData* WemToOgg(AudioData* wemData);
// BinaryData* ww2ogg(int argc, char **argv);
int VERBOSE = 0;

int main()
{
    // volatile codebook_library cbl((unsigned char*) main_codebook, 74387);
    // char** bnk_extract_args = (char*[]) {"", "-a", "hellu", NULL};
    // char data_pointer[17] = "0123456789ABCDEF";
    // char* ww2ogg_args[] = {"", "--audiodata", data_pointer, NULL};
    // BinaryData* raw_ogg = ww2ogg(sizeof(ww2ogg_args) / sizeof(ww2ogg_args[0]) - 1, ww2ogg_args);;
    // printf("raw_ogg: %p\n", raw_ogg);
    WemInformation* wemInformation = NULL;//bnk_extract(3, bnk_extract_args);
    printf("wemInformation: %p\n", wemInformation);
    BinaryData* oggData = WemToOgg(&wemInformation->sortedWemDataList->objects[0]);
    printf("oggData: %p\n", oggData);
}
