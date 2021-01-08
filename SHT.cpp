#include "SHT.h"
#include "HP.h"
#include "BF.h"
#include "SHT_HP.h"

#include <cstring>
#include <iostream>
using namespace std;

#define MAX_BUCKETS_IN_BLOCK ((BLOCK_SIZE - 2 * (int) sizeof(int)) / (int) sizeof(int))

int SHT_CreateSecondaryIndex(const char* sfileName, const char* attrName, int attrLength, int buckets, const char* fileName){
  if (strlen(sfileName) > MAX_NAME_SIZE - 1 || strlen(fileName) > MAX_NAME_SIZE - 1){
    return -1;
  }

  int old_file = BF_OpenFile(fileName);
  if (old_file < 0){
    return -1;
  }
  void *block;
  if (BF_ReadBlock(old_file, 0, &block) < 0){
    return -1;
  }
  // Check if the old file is HT.
  char type[3];
  memcpy(type, (char *)block, 3);
  if (strcmp(type, "HT") != 0){
    return -1;
  }

  if (BF_CreateFile(sfileName) < 0){
    return -1;
  }
  int new_file = BF_OpenFile(sfileName);
  if (new_file < 0){
    return -1;
  }
  if (BF_AllocateBlock(new_file) < 0){
    return -1;
  }
  if (BF_ReadBlock(new_file, 0, &block) < 0){
    return -1;
  }

  // Save data to block.
  memcpy((char *)block, "SHT", 4); //safety to know that we have the corect file
  memcpy((char *)block + 4, &attrLength, sizeof(int));
  memcpy((char *)block + 4 + sizeof(int), attrName, MAX_NAME_SIZE);
  memcpy((char *)block + 4 + sizeof(int) + MAX_NAME_SIZE, &buckets, sizeof(int));
  memcpy((char *)block + 4 + sizeof(int) + MAX_NAME_SIZE + sizeof(int), fileName, MAX_NAME_SIZE);

  BF_WriteBlock(new_file, 0);

  int pl_blocks = (buckets / MAX_BUCKETS_IN_BLOCK)+ 1;
  for(int i=0;i<pl_blocks;i++){
    if(BF_AllocateBlock(new_file) < 0){
      return -1;
    }
    if(BF_ReadBlock(new_file, BF_GetBlockCounter(new_file)-1, &block) < 0){
      return -1;
    }

    int max;
    if(i == pl_blocks-1){
      max = buckets - MAX_BUCKETS_IN_BLOCK*i;
    }else{
      max = MAX_BUCKETS_IN_BLOCK;
    }

    for(int j=0;j<max;j++){
      int temp =0;
      memcpy((char*)block+ sizeof(int)*j, &temp, sizeof(int));
    }

    BF_WriteBlock(new_file, BF_GetBlockCounter(new_file)-1);
  }

  if (BF_CloseFile(new_file) < 0){
    return -1;
  }
  if (BF_CloseFile(old_file) < 0){
    return -1;
  }

  return 0;
}

SHT_info* SHT_OpenSecondaryIndex(char* sfileName){
  int fd = BF_OpenFile(sfileName);
  if (fd < 0){
    return nullptr;
  }

  SHT_info* info = new SHT_info;
  info->fileDesc = fd;

  void* block;
  if (BF_ReadBlock(fd, 0, &block) < 0){
    return nullptr;
  }

  // Check if we have opened a valid file.
  char type[4];
  memcpy(type, (char *)block, 4);
  if (strcmp(type, "SHT") != 0){
    return nullptr;
  }

  memcpy(&(info->attrLength), (char *)block + 4, sizeof(int));
  memcpy(&(info->attrName), (char *)block + 4 + sizeof(int), MAX_NAME_SIZE);
  memcpy(&(info->numBuckets), (char *)block + 4 + sizeof(int) + MAX_NAME_SIZE, sizeof(int));
  memcpy(&(info->fileName), (char *)block + 4 + sizeof(int) + MAX_NAME_SIZE + sizeof(int), MAX_NAME_SIZE);
  return info;
}

int SHT_CloseSecondaryIndex(SHT_info* header_info){
  if (BF_CloseFile(header_info->fileDesc) != 0){
    return -1;
  }

  delete header_info;

  return 0;
}

int SHT_SecondaryInsertEntry(SHT_info header_info, SecondaryRecord record){
  int h = HT_function(record.surname, header_info.numBuckets);
  void* block;
  int heap;
  int j;
  int i;
  int counter =0;
  int pl_blocks = (header_info.numBuckets / MAX_BUCKETS_IN_BLOCK)+ 1;

  for(i =0;i<pl_blocks;i++){
    if(BF_ReadBlock(header_info.fileDesc, i+1, &block) < 0){
      return -1;
    }
    int max;
    heap =0;
    if(i == pl_blocks-1){
      max = header_info.numBuckets - MAX_BUCKETS_IN_BLOCK*i;
    }else{
      max = MAX_BUCKETS_IN_BLOCK;
    }
    int found =0;
    for(j=0;j<max;j++){
      memcpy(&heap, (char *)block + sizeof(int)*j, sizeof(int));
      if(counter == h){
        found =1;
        break;
      }

      counter++;
    }
    if(found){
      break;
    }
  }

  int new_heap_addr = SHT_HP_InsertEntry(&header_info, &record, heap);
  if (new_heap_addr == -1){
    return -1;
  }

  if (new_heap_addr != heap){
    memcpy((char*)block +sizeof(int)*j, &new_heap_addr, sizeof(int));
    BF_WriteBlock(header_info.fileDesc, i+1);
  }

  return 0;
}

int SHT_SecondaryGetAllEntries(SHT_info header_info_sht, HT_info header_info_ht, void *value){
  int h = HT_function((char*)value, (int)header_info_sht.numBuckets);
  void* block;
  int heap;
  int j;
  int i;
  int counter=0;
  int pl_blocks = (header_info_sht.numBuckets / MAX_BUCKETS_IN_BLOCK)+ 1;
  int block_counter = 0;

  for(i=0;i<pl_blocks;i++){
    block_counter++;
    if(BF_ReadBlock(header_info_sht.fileDesc, i+1, &block) <0){
      return -1;
    }

    int max;
    if(i == pl_blocks-1){
      max = header_info_sht.numBuckets - MAX_BUCKETS_IN_BLOCK*i;
    }else{
      max = MAX_BUCKETS_IN_BLOCK;
    }

    heap=0;
    int found =0;
    for(j=0;j<max;j++){
      if(counter == h){
        memcpy(&heap, (char *)block + sizeof(int)*j, sizeof(int));
        found =1;
        break;
      }
      counter++;
    }
    if(found){
      break;
    }
  }

  int result = SHT_HP_GetAllEntries(&header_info_sht, &header_info_ht, value, heap);
  if (result == -1){
    return -1;
  }

  return block_counter + result;
}

int SHT_HashStatistics(char* filename){
  SHT_info* header_info_sht = SHT_OpenSecondaryIndex(filename);
  HT_info* header_info_ht = HT_OpenIndex(header_info_sht->fileName);

  int numBuckets =header_info_sht->numBuckets;
  int pl_blocks = (numBuckets / MAX_BUCKETS_IN_BLOCK)+ 1;
  int block_counter=1 + pl_blocks;

  int array[numBuckets];

  void* block;
  int heap;
  int j;
  int i;
  int counter=0;
  for(i=0;i<pl_blocks;i++){
    if(BF_ReadBlock(header_info_sht->fileDesc, i+1, &block) <0){
      return -1;
    }

    int max;
    if(i == pl_blocks-1){
      max = header_info_sht->numBuckets - MAX_BUCKETS_IN_BLOCK*i;
    }else{
      max = MAX_BUCKETS_IN_BLOCK;
    }

    heap=0;
    for(j=0;j<max;j++){
      memcpy(&heap, (char *)block + sizeof(int)*j, sizeof(int));
      array[counter] = heap;
      counter++;
    }
  }


  int k=0;
  while(array[k] == 0){
    k++;
  }
  int temp = SHT_HP_GetRecordCounter(header_info_sht, array[k]);
  int min = temp;
  int max = temp;
  int average_records = 0;
  int average_blocks = 0;
  int used_buckets = numBuckets;//used this so no empty buckets will be calculated in the averages
  for(int i=0;i< numBuckets; i++){
    heap = array[i];
    if(heap == 0){
      used_buckets--;
      continue;
    }

    int num = SHT_HP_GetBlockCounter(header_info_sht, heap);
    int pl_records = SHT_HP_GetRecordCounter(header_info_sht, heap);

    if(pl_records < min){
      min = pl_records;
    }
    if(pl_records > max){
      max = pl_records;
    }

    block_counter += num;
    average_blocks += num;
    average_records += pl_records;
  }
  average_blocks = average_blocks/used_buckets;
  average_records = average_records/used_buckets;

  cout << "Blocks used by file \"" << filename << "\": " << block_counter << endl;
  cout << "Minimum records per bucket: " << min << endl;
  cout << "Maximum records per bucket: " << max << endl;
  cout << "Average records per bucket: " << average_records << endl;

  cout << "Average number of blocks per bucket: " << average_blocks << endl;

  cout << "Overflow blocks: " << endl;
  int overflow=0;
  for(int i=0;i<numBuckets;i++){
    int heap;
    heap = array[i];
    int num = SHT_HP_GetBlockCounter(header_info_sht, heap);
    if(num > 1){
      overflow += num-1;
      cout << "For bucket " << i << ", " << num-1 << endl;
    }
  }
  cout << "Overflow sum: " << overflow << endl;

  if(HT_CloseIndex(header_info_ht) <0){
    return -1;
  }
  if(SHT_CloseSecondaryIndex(header_info_sht) < 0){
    return -1;
  }

  return 0;
}
