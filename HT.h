#ifndef __HT_H__
#define __HT_H__

#include "Record.h"
#define MAX_ATTR_NAME_SIZE 15
typedef struct {
  int fileDesc;
  char attrType;
  char attrName[MAX_ATTR_NAME_SIZE];
  int attrLength;
  long int numBuckets;
} HT_info;

int HT_CreateIndex(const char *fileName, const char attrType, const char* attrName, const int attrLength, const int buckets);

HT_info* HT_OpenIndex(char *fileName);

int HT_CloseIndex(HT_info* header_info);

int HT_InsertEntry(HT_info header_info, Record record);

int HT_DeleteEntry(HT_info header_info, void *value);

int HT_GetAllEntries( HT_info header_info, void *value);

int HT_function(int* value, int buckets);

int HT_function(char* value, int buckets);

int HashStatistics(char* filename);
#endif
