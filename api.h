#ifndef BNK_EXTRACT_API_H
#define BNK_EXTRACT_API_H

#include "list.h"

typedef LIST(struct stringWithChildren) StringWithChildrenList;

typedef struct stringWithChildren {
    char* string;
    BinaryData* wemData;
    StringWithChildrenList children;
} StringWithChildren;

typedef struct {
    StringWithChildren* grouped_wems;
    IndexedDataList* sortedWemDataList;
} WemInformation;

// call like a main()
WemInformation* bnk_extract(int argc, char* argv[]);

BinaryData* convert_audio(BinaryData* wemData);

#endif
