#ifndef BNK_H
#define BNK_H

#include <stdbool.h>
#include <stdio.h>
#include "bin.h"
#include "defs.h"

uint32_t skip_to_section(FILE* bnk_file, char name[4], bool from_beginning);

WemInformation* parse_audio_bnk_file(char* bnk_path, StringHashes* string_hashes);

#endif
