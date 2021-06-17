#ifndef WPK_H
#define WPK_H

#include "bin.h"
#include "defs.h"

WemInformation* parse_wpk_file(char* wpk_path, StringHashes* string_hashes);

#endif
