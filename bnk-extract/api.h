#ifndef BNK_EXTRACT_API_H
#define BNK_EXTRACT_API_H

#include "defs.h"

// call like a main()
WemInformation* bnk_extract(int argc, char* argv[]);

BinaryData* WemToOgg(AudioData* wemData);

#endif
