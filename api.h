#ifndef BNK_EXTRACT_API_H
#define BNK_EXTRACT_API_H

#include "list.h"

typedef LIST(struct stringWithChildren) StringWithChildrenList;

typedef struct stringWithChildren {
    char* string;
    BinaryData* oggData;
    StringWithChildrenList children;
} StringWithChildren;

// call like a main()
StringWithChildren* bnk_extract(int argc, char* argv[]);

#endif
