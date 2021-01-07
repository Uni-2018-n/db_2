#include "SHT.h"

int SHT_CreateSecondaryIndex(char* sfileName, char* attrName, int attrLength, int buckets, char* fileName)
{
    return 0;
}

SHT_info* SHT_OpenSecondaryIndex(char* sfileName)
{
    return nullptr;
}

int SHT_CloseSecondaryIndex(SHT_info* header_info)
{
    return 0;
}

int SHT_SecondaryInsertEntry(SHT_info header_info, SecondaryRecord record)
{
    return 0;
}