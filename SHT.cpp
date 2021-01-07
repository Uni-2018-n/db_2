#include "SHT.h"
#include "HP.h"
#include "BF.h"

#include <cstring>
#include <iostream>

#define MAX_BUCKETS_IN_BLOCK ((BLOCK_SIZE - 2 * (int) sizeof(int)) / (int) sizeof(int))
#define MAX_RECORDS_IN_BLOCK ((BLOCK_SIZE - 2 * (int) sizeof(int)) / (int) sizeof(SecondaryRecord))

int SHT_CreateSecondaryIndex(char* sfileName, char* attrName, int attrLength, int buckets, char* fileName){
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

  BF_Init();

  if (BF_CreateFile(sfileName) < 0){
    return -1;
  }

  int new_file = BF_OpenFile(sfileName);
  if (new_file < 0){
    return -1;
  }

  if (BF_ReadBlock(new_file, 0, &block) < 0){
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

  if (BF_CloseFile(new_file) < 0)
      return -1;

  if (BF_CloseFile(old_file) < 0)
      return -1;

  return 0;
}

SHT_info* SHT_OpenSecondaryIndex(char* sfileName){
  int fd = BF_OpenFile(sfileName);
  if (fd < 0)
      return nullptr;

  SHT_info* info = new SHT_info;

  info->fileDesc = fd;

  void* block;
  if (BF_ReadBlock(fd, 0, &block) < 0)
      return nullptr;

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

int SHT_GetAllEntries(SHT_info header_info_sht, HT_info header_info_ht, void *value){
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

int SHT_HP_InsertEntry(SHT_info* header_info, SecondaryRecord* record, int heap_address)
{
	int curr_block_addr = heap_address;
	int should_init_block = 0;

	// First block in heap.
	if (heap_address == 0)
	{
		should_init_block = 1;
		// Set the address that the heap is going to get.
		heap_address = BF_GetBlockCounter(header_info->fileDesc);
	}

	void* block = nullptr;

	int available_block_addr = -1; // a block with space to add a record.
	int next_block_addr = -1;

	// Loop until you have read all the blocks.
	while(1)
	{
		if (should_init_block == 0)
		{
			if (BF_ReadBlock(header_info->fileDesc, curr_block_addr, &block) != 0)
				return -1;
		}

		else
		{
			// Create a block and initialize some values.
			if (InitBlock(header_info->fileDesc, &block) == -1)
				return -1;

			should_init_block = 0;
			curr_block_addr = BF_GetBlockCounter(header_info->fileDesc) - 1;
		}

		if (IsKeyInBlock(record, block) > -1)
			return -1;

		next_block_addr = ReadNextBlockAddr(block);

		int num_of_records = ReadNumOfRecords(block);
		// If we haven't enough space for another record.
		if (num_of_records + 1 > MAX_RECORDS_IN_BLOCK)
		{
			// If there isn't a next block and we haven't found an available address for a record.
			if (next_block_addr == -1 && available_block_addr == -1)
			{
        		int num_of_blocks = BF_GetBlockCounter(header_info->fileDesc);
				WriteNextBlockAddr(block, num_of_blocks);
				BF_WriteBlock(header_info->fileDesc, curr_block_addr);
        		next_block_addr = num_of_blocks;

				should_init_block = 1;
			}
		}

		// If we have space to store the block and haven't assigned one yet.
		else if (available_block_addr == -1)
			available_block_addr = curr_block_addr;

		// If we have reached the last block.
		if (next_block_addr == -1)
		{
			// Only read when necessary.
			if (curr_block_addr != available_block_addr)
			{
				if (BF_ReadBlock(header_info->fileDesc, available_block_addr, &block) != 0)
					return -1;
			}

			WriteRecord(block, num_of_records, record);
			WriteNumOfRecords(block, num_of_records + 1);

			if (BF_WriteBlock(header_info->fileDesc, available_block_addr) != 0)
				return -1;

			break;
		}

    curr_block_addr = next_block_addr;
	}

	return heap_address;
}

int SHT_HP_GetAllEntries(SHT_info* header_info_sht, HT_info* header_info_ht, void* value, int heap_addr)
{
  if (heap_addr == 0)
    return -1;

	void* block;
	int curr_block_addr = heap_addr;
	SecondaryRecord record;
  int counter = 0; // Counts how many blocks were read until we found the key.

  while (curr_block_addr != -1)
	{
    counter++;

		if (BF_ReadBlock(header_info_sht->fileDesc, curr_block_addr, &block) != 0)
			return -1;

    if (AssignKeyToRecord(&record, value) != 0)
			return 1; //check this if we need this or not to find the secondaryrecord(so we can get the primary record's blockID)


		int record_pos = IsKeyInBlock(&record, block);
		if (record_pos > -1)
		{
			ReadRecord(block, record_pos, &record);

      HT_HP_GetAllEntries(header_info_ht, value, record.blockId);//check this, if blockId is the same that we pass as heap_addr

			return counter;
		}

    curr_block_addr = ReadNextBlockAddr(block);
	}

	return -1;
}

int IsKeyInBlock(SecondaryRecord* record, void* block)
{
	int num_of_records = ReadNumOfRecords(block);

	Record tmp_record;

	for (int i = 0; i < num_of_records; i++)
	{
		ReadRecord(block, i, &tmp_record);

    if(strcmp(record->surname, tmp_record.surname) == 0){
      return i;
    }
	}
	return -1;
}

void WriteRecord(void* block, int recordNumber, const SecondaryRecord* record)
{
	memcpy((char *)block + recordNumber * sizeof(Record), record, sizeof(Record));
}

void ReadRecord(void* block, int recordNumber, SecondaryRecord* record)
{
	memcpy(record, (char *)block + recordNumber * sizeof(Record), sizeof(Record));
}

int AssignKeyToRecord(SecondaryRecord* record, void* value)
{
  strcpy(record->surname, (char *)value);
	return 0;
}

int HT_function(char* value, int buckets){//same as int but for characters
  unsigned int hash = 5381;
  for(char* s= value; *s != '\0'; s++){
    hash = (hash << 5) + hash + *s;
  }
  return hash % buckets;
}
