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
  if (old_file < 0)
      return -1;

  void *block;
  if (BF_ReadBlock(old_file, 0, &block) < 0)
      return -1;


  // Check if the old file is HT.
  char type[3];
  memcpy(type, (char *)block, 3);
  if (strcmp(type, "HT") != 0)
      return -1;

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

  int new_heap_addr = SHT_HP_InsertEntry(&header_info, &record, heap); //then pass it to HT_HP_InsertEntry to add it into the heap's blocks
  if (new_heap_addr == -1){
    return -1;
  }
  if (new_heap_addr != heap){ //if the heap was empty the upper loop returned the int heap variable as 0 so we need to set the new address returned by HT_HP_InsertEntry
    memcpy((char*)block +sizeof(int)*j, &new_heap_addr, sizeof(int));
    BF_WriteBlock(header_info.fileDesc, i+1); //and save changed
  }
  return 0;



  return 0;
}

int SHT_SecondaryGetAllEntries(SHT_info header_info_sht, HT_info header_info_ht, void *value){
  int h;//again same implementation as above
  h = HT_function((char*)value, (int)header_info_sht.numBuckets);

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
    heap=0;
    if(i == pl_blocks-1){
      max = header_info_sht.numBuckets - MAX_BUCKETS_IN_BLOCK*i;
    }else{
      max = MAX_BUCKETS_IN_BLOCK;
    }
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

  //call the function to print the entry and return the blocks searched to find the entry.
  int result = SHT_HP_GetAllEntries(&header_info_sht, &header_info_ht, value, heap);

  if (result == -1)
    return -1;
  else
    return block_counter + result;
}
//
// int HashStatistics(char* filename){
//   HT_info* header_info = HT_OpenIndex(filename);
//
//   int numBuckets =header_info->numBuckets;
//   int pl_blocks = (numBuckets / MAX_BUCKETS_IN_BLOCK)+ 1;
//   int block_counter=1 + pl_blocks;//the block counter starts with one because we have the header block, and then we add the pl of blocks used by the hash table
//
//   int array[numBuckets];//initialize this array so we can easily store the heap's addresses
//
//   void* block;
//   int heap;
//   int j;
//   int i;
//   int counter=0;
//
//   for(i=0;i<pl_blocks;i++){//as above for each block read it and go through its buckets
//     if(BF_ReadBlock(header_info->fileDesc, i+1, &block) <0){
//       return -1;
//     }
//
//     int max;
//     heap=0;
//     if(i == pl_blocks-1){
//       max = header_info->numBuckets - MAX_BUCKETS_IN_BLOCK*i;
//     }else{
//       max = MAX_BUCKETS_IN_BLOCK;
//     }
//     for(j=0;j<max;j++){
//         memcpy(&heap, (char *)block + sizeof(int)*j, sizeof(int));
//         array[counter] = heap;//store the heap into the array
//       counter++;
//     }
//   }
//
//   //we used an array here because inside HT_HP_* functions we have BF_ReadBlock function that changes the void* block data so its not possible for us to have the correct one
//   int k=0;
//   while(array[k] == 0){
//     k++;
//   }
//   int temp = HT_HP_GetRecordCounter(header_info, array[k]);
//   int min = temp;//help variables to calculate min, max and averages
//   int max = temp;
//   int average_records = 0;
//   int average_blocks = 0;
//   for(int i=0;i< numBuckets; i++){//for every bucket do every calculation to find the output
//     heap = array[i];
//     if(heap == 0){//if its 0 then we have an unused heap so skip it
//       continue;
//     }
//     int num = HT_HP_GetBlockCounter(header_info, heap);//here it goes into infinite loop
//
//     int pl_records = HT_HP_GetRecordCounter(header_info, heap);
//
//     block_counter += num;
//
//     if(pl_records < min){
//       min = pl_records;
//     }
//     if(pl_records > max){
//       max = pl_records;
//     }
//     average_blocks += num;
//     average_records += pl_records;
//
//   }
//   average_blocks = average_blocks/numBuckets;
//   average_records = average_records/numBuckets;
//
//   //and print it
//   cout << "Blocks used by file \"" << filename << "\": " << block_counter << endl;
//   cout << "Minimum records per bucket: " << min << endl;
//   cout << "Maximum records per bucket: " << max << endl;
//   cout << "Average records per bucket: " << average_records << endl;
//
//   cout << "Average number of blocks per bucket: " << average_blocks << endl;
//
//
//   cout << "Overflow blocks: " << endl;//same thing with overflow
//   int overflow=0;
//   for(int i=0;i<numBuckets;i++){
//     int heap;
//     heap = array[i];
//     int num = HT_HP_GetBlockCounter(header_info, heap);
//     if(num > 1){//if the num is > 1 it means that there is an overflow happening
//       overflow += num-1;
//       cout << "For bucket " << i << ", " << num-1 << endl;
//     }
//   }
//   cout << "Overflow sum: " << overflow << endl;
//
//   if(HT_CloseIndex(header_info) <0){
//     return -1;
//   }
//   return 0;
// }



// int HT_function(char* value, int buckets){//same as int but for characters
//   unsigned int hash = 5381;
//   for(char* s= value; *s != '\0'; s++){
//     hash = (hash << 5) + hash + *s;
//   }
//   return hash % buckets;
// }
