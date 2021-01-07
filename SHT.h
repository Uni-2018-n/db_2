#ifndef __SHT_H__
#define __SHT_H__

#include "Record.h"

#define MAX_ATTR_NAME_SIZE 15
#define MAX_FILE_NAME_SIZE 30

struct SHT_info
{
    int fileDesc;
    char attrName[MAX_ATTR_NAME_SIZE];
    int attrLength;
    long int numBuckets;
    char fileName[MAX_FILE_NAME_SIZE];
};

struct SecondaryRecord
{
    Record record;
    int blockId;
};

int SHT_CreateSecondaryIndex(char* sfileName, char* attrName, int attrLength, int buckets, char* fileName);

SHT_info* SHT_OpenSecondaryIndex(char* sfileName);

int SHT_CloseSecondaryIndex(SHT_info* header_info);

int SHT_SecondaryInsertEntry(SHT_info header_info, SecondaryRecord record);

#endif // __SHT_H__