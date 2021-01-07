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
    long int numBuckets;
    char fileName[MAX_NAME_SIZE];
};

struct SecondaryRecord
{
    char surname[25];
    int blockId;
};

int SHT_CreateSecondaryIndex(char* sfileName, char* attrName, int attrLength, int buckets, char* fileName);

SHT_info* SHT_OpenSecondaryIndex(char* sfileName);

int SHT_CloseSecondaryIndex(SHT_info* header_info);

int SHT_SecondaryInsertEntry(SHT_info header_info, SecondaryRecord record);

int HT_function(char* value, long int buckets);
int SHT_HP_InsertEntry(SHT_info* header_info, SecondaryRecord* record, int heap_address);
int SHT_HP_GetAllEntries(SHT_info* header_info_sht, HT_info* header_info_ht, void* value, int heap_addr);

int IsKeyInBlock(SecondaryRecord* record, void* block);
void WriteRecord(void* block, int recordNumber, const SecondaryRecord* record);
void ReadRecord(void* block, int recordNumber, SecondaryRecord* record);
int AssignKeyToRecord(SecondaryRecord* record, void* value);
#endif // __SHT_H__
