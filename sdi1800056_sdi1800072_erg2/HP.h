#ifndef __HP_H__
#define __HP_H__

#include "Record.h"
#include "HT.h"


#define MAX_ATTR_NAME_SIZE 15

struct HP_info
{
    int fileDesc;
    char attrType;
    char attrName[MAX_ATTR_NAME_SIZE];
    int attrLength;
};

// Core functions.
int HP_CreateFile(const char *fileName, char attrType, const char* attrName, int attrLength);
HP_info* HP_OpenFile(const char *fileName);
int HP_CloseFile(HP_info* header_info);
int HP_InsertEntry(HP_info header_info, Record record);
int HP_DeleteEntry(HP_info header_info, void *value);
int HP_GetAllEntries(HP_info header_info, void *value);

// Helper functions.
int ReadNumOfRecords(void *block);
void WriteNumOfRecords(void* block, int recordNumber);
int ReadNextBlockAddr(void* block);
void WriteNextBlockAddr(void* block, int blockNumber);
void ReadRecord(void* block, int recordNumber, Record* record);
void WriteRecord(void* block, int recordNumber, const Record* record);
void* InitBlock(HP_info* header_info, int blockNumber);
int IsBlockEmpty(int file_desc);
void ReplaceWithLastRecord(int pos, void* block);
int InitBlock(int fileDesc, void** block);
int IsKeyInBlock(Record* record, void* block);
int AssignKeyToRecord(Record* record, void* value, char key_type);

// Functions used by HT.
int HT_HP_InsertEntry(HT_info* header_info, Record* record, int heap);
int HT_HP_DeleteEntry(HT_info* header_info, void* value, int heap_address);
int HT_HP_GetAllEntries(HT_info* header_info, void* value, int heap_addr);
int HT_HP_GetRecordCounter(HT_info* header_info, int heap_addr);
int HT_HP_GetBlockCounter(HT_info* header_info, int heap_addr);

#endif // __HP_H__
