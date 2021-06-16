#ifndef BNK_H
#define BNK_H

#include <stdbool.h>
#include <stdio.h>
#include "bin.h"
#include "api.h"

uint32_t skip_to_section(FILE* bnk_file, char name[4], bool from_beginning);

WemInformation* extract_bnk_file(char* bnk_path, StringHashes* string_hashes, char* output_path, bool wems_only, bool oggs_only);

#endif
