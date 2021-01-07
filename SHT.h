#ifndef __SHT_H__
#define __SHT_H__

#include "Record.h"
#include "HT.h"

#define MAX_NAME_SIZE 15


struct SHT_info
{
    int fileDesc;
    int attrLength;
    char attrName[MAX_NAME_SIZE];
    int numBuckets;
    char fileName[MAX_NAME_SIZE];
};

struct SecondaryRecord
{
    char surname[25];
    int blockId;
};

int SHT_CreateSecondaryIndex(const char* sfileName, const char* attrName, int attrLength, int buckets, const char* fileName);

SHT_info* SHT_OpenSecondaryIndex(char* sfileName);

int SHT_CloseSecondaryIndex(SHT_info* header_info);

int SHT_SecondaryInsertEntry(SHT_info header_info, SecondaryRecord record);
int SHT_SecondaryGetAllEntries(SHT_info header_info_sht, HT_info header_info_ht, void *value);
// int HT_function(char* value, int buckets);
#endif // __SHT_H__
