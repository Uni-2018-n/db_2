#include "SHT.h"
#include "BF.h"
#include "HP.h"
#include "SHT_HP.h"
#include <cstring>
#include <iostream>
using namespace std;

#define MAX_RECORDS_IN_BLOCK ((BLOCK_SIZE - 2 * (int) sizeof(int)) / (int) sizeof(SecondaryRecord))

int SHT_HP_InsertEntry(SHT_info* header_info, SecondaryRecord* record, int heap_address){
	int curr_block_addr = heap_address;
	int should_init_block = 0;

	// First block in heap.
	if (heap_address == 0){
		should_init_block = 1;
		// Set the address that the heap is going to get.
		heap_address = BF_GetBlockCounter(header_info->fileDesc);
	}

	void* block = nullptr;

	int available_block_addr = -1; // a block with space to add a record.
	int next_block_addr = -1;

	// Loop until you have read all the blocks.
	while(1){
		if (should_init_block == 0){
			if (BF_ReadBlock(header_info->fileDesc, curr_block_addr, &block) != 0){
				return -1;
			}
		}else{
			// Create a block and initialize some values.
			if (InitBlock(header_info->fileDesc, &block) == -1){
				return -1;
			}

			should_init_block = 0;
			curr_block_addr = BF_GetBlockCounter(header_info->fileDesc) - 1;
		}

		int record_pos;

		if ((record_pos = SHT_IsKeyInBlock(record, block, 0)) > -1)
		{	
			SecondaryRecord temp_rec;
			SHT_ReadRecord(block, record_pos, &temp_rec);

			if (record->blockId == temp_rec.blockId)
				return curr_block_addr;

			cout << "Exit";
			exit(0);
		}

		next_block_addr = ReadNextBlockAddr(block);

		int num_of_records = ReadNumOfRecords(block);
		// If we haven't enough space for another record.
		if (num_of_records + 1 > MAX_RECORDS_IN_BLOCK){
			// If there isn't a next block and we haven't found an available address for a record.
			if (next_block_addr == -1 && available_block_addr == -1){
        int num_of_blocks = BF_GetBlockCounter(header_info->fileDesc);
				WriteNextBlockAddr(block, num_of_blocks);
				BF_WriteBlock(header_info->fileDesc, curr_block_addr);
        next_block_addr = num_of_blocks;

				should_init_block = 1;
			}
		}else if (available_block_addr == -1){// If we have space to store the block and haven't assigned one yet.
			available_block_addr = curr_block_addr;
		}

		if (next_block_addr == -1){// If we have reached the last block.
			if (curr_block_addr != available_block_addr){// Only read when necessary.
				if (BF_ReadBlock(header_info->fileDesc, available_block_addr, &block) != 0){
					return -1;
				}
			}

			SHT_WriteRecord(block, num_of_records, record);
			WriteNumOfRecords(block, num_of_records + 1);

			if (BF_WriteBlock(header_info->fileDesc, available_block_addr) != 0){
				return -1;
			}
			break;
		}

    curr_block_addr = next_block_addr;
	}
	return heap_address;
}

int SHT_HP_GetAllEntries(SHT_info* header_info_sht, HT_info* header_info_ht, void* value, int heap_addr){
  if (heap_addr == 0){
		return -1;
	}
	void* block;
	int curr_block_addr = heap_addr;
	SecondaryRecord record;
  int counter = 0; // Counts how many blocks were read until we found the key.

  bool found = false;

  while (curr_block_addr != -1){
    counter++;

		if (BF_ReadBlock(header_info_sht->fileDesc, curr_block_addr, &block) != 0){
			return -1;
		}
    if (SHT_AssignKeyToRecord(&record, value) != 0){
			return 1;
		}

		int record_pos;
		int last_pos = 0;

		while ((record_pos = SHT_IsKeyInBlock(&record, block, last_pos)) != -1)
		{
			SHT_ReadRecord(block, record_pos, &record);
      		HT_HP_GetAllEntries_T(header_info_ht, value, record.blockId);//pass it to an HT_HP function so it will know what to do with it

			found = true;
			last_pos = record_pos + 1;
		}

		curr_block_addr = ReadNextBlockAddr(block);
	}

	if (found == true)
		return counter;

	return -1;
}

int HT_HP_GetAllEntries_T(HT_info* header_info, void* value, int heap_addr){
  if (heap_addr == 0){
		return -1;
	}

	void* block;
	int curr_block_addr = heap_addr;
	Record record;
  int counter = 0; // Counts how many blocks were read until we found the key.

  bool found = false;

  while (curr_block_addr != -1){
    counter++;

		if (BF_ReadBlock(header_info->fileDesc, curr_block_addr, &block) != 0){
			return -1;
		}
		if (SHT_AssignKeyToRecord_T(&record, value) != 0){
			return 1;
		}

		int record_pos;
		int last_pos = 0;

		while ((record_pos = SHT_IsKeyInBlock_T(&record, block, last_pos)) != -1)
		{
			ReadRecord(block, record_pos, &record);
			std::cout << "id: " << record.id
					  << "\nname: " << record.name
					  << "\nsurname: " << record.surname
					  << "\naddress: " << record.address
					  << std::endl;

			found = true;
			last_pos = record_pos + 1;
		}

    curr_block_addr = ReadNextBlockAddr(block);
	}

	if (found)
		return counter;

	return -1;
}

int SHT_HP_GetRecordCounter(SHT_info* header_info, int heap_addr){
	if (heap_addr == 0){
		return -1;
	}

	void* block;
	int curr_block_addr = heap_addr;
	int counter = 0; // Counts how many records there are in a heap.
	while (curr_block_addr != -1){
		if (BF_ReadBlock(header_info->fileDesc, curr_block_addr, &block) < 0){
			return -1;
		}

		counter += ReadNumOfRecords(block);
  	curr_block_addr = ReadNextBlockAddr(block);
	}

	return counter;
}

int SHT_HP_GetBlockCounter(SHT_info* header_info, int heap_addr){
  if (heap_addr == 0){
		return -1;
	}

	void* block;
	int curr_block_addr = heap_addr;
	int counter = 0; // Counts how many blocks there are in a heap.
  while (curr_block_addr != -1){
		if (BF_ReadBlock(header_info->fileDesc, curr_block_addr, &block) != 0){
			return -1;
		}

		counter++;
  	curr_block_addr = ReadNextBlockAddr(block);
	}

	return counter;
}


int SHT_IsKeyInBlock(SecondaryRecord* record, void* block, int last_pos){
	int num_of_records = ReadNumOfRecords(block);
	SecondaryRecord tmp_record;
	for (int i = last_pos; i < num_of_records; i++){
		SHT_ReadRecord(block, i, &tmp_record);
    if(strcmp(record->surname, tmp_record.surname) == 0){
      return i;
    }
	}
	return -1;
}

int SHT_IsKeyInBlock_T(Record* record, void* block, int last_pos){//used incase we need to find a record inside an HT block but we need to search it with the surname
	int num_of_records = ReadNumOfRecords(block);
	Record tmp_record;
	for (int i = last_pos; i < num_of_records; i++){
		ReadRecord(block, i, &tmp_record);
    if(strcmp(record->surname, tmp_record.surname) == 0){
      return i;
    }
	}
	return -1;
}

void SHT_WriteRecord(void* block, int recordNumber, const SecondaryRecord* record){
	memcpy((char *)block + recordNumber * sizeof(SecondaryRecord), record, sizeof(SecondaryRecord));
}

void SHT_ReadRecord(void* block, int recordNumber, SecondaryRecord* record){
	memcpy(record, (char *)block + recordNumber * sizeof(SecondaryRecord), sizeof(SecondaryRecord));
}

int SHT_AssignKeyToRecord(SecondaryRecord* record, void* value){
  strcpy(record->surname, (char *)value);
	return 0;
}

int SHT_AssignKeyToRecord_T(Record* record, void* value){
  strcpy(record->surname, (char *)value);
	return 0;
}
