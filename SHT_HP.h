#ifndef __SHT_HP_H__
#define __SHT_HP_H__

#include "SHT.h"

int SHT_HP_InsertEntry(SHT_info* header_info, SecondaryRecord* record, int heap_address);
int SHT_HP_GetAllEntries(SHT_info* header_info_sht, HT_info* header_info_ht, void* value, int heap_addr);

int SHT_IsKeyInBlock(SecondaryRecord* record, void* block);
void SHT_WriteRecord(void* block, int recordNumber, const SecondaryRecord* record);
void SHT_ReadRecord(void* block, int recordNumber, SecondaryRecord* record);
int SHT_AssignKeyToRecord(SecondaryRecord* record, void* value);
int SHT_HP_GetAllEntries(HT_info* header_info, void* value, int heap_addr);
int SHT_AssignKeyToRecord_T(Record* record, void* value);
int SHT_IsKeyInBlock_T(Record* record, void* block);
#endif
