#include "HP.h"
#include "BF.h"
#include <cstring>
#include <iostream>
using namespace std;

#define MAX_RECORDS_IN_BLOCK ((BLOCK_SIZE - 2 * (int) sizeof(int)) / (int) sizeof(Record))

int HP_CreateFile(const char *fileName, char attrType, const char* attrName, int attrLength)
{
	// Don't do anything if name greater than the limit.
	if (strlen(attrName) > MAX_ATTR_NAME_SIZE - 1)
		return 1;

	BF_Init();

	int error = BF_CreateFile(fileName);
	if (error != 0)
		return error;

	int fd = BF_OpenFile(fileName);
	if (fd < 0)
		return fd;

	error = BF_AllocateBlock(fd);
	if (error != 0)
			return error;

	void *block;
	error = BF_ReadBlock(fd, 0, &block);
	if (error != 0)
		return error;

	// Copy data to block.
	memcpy((char *)block, "HP", 3);
	memcpy((char *)block + 3, &attrType, sizeof(char));
	memcpy((char *)block + 3 + sizeof(char), attrName, strlen(attrName) + 1);
	memcpy((char *)block + 3 + sizeof(char) + MAX_ATTR_NAME_SIZE , &attrLength, sizeof(int));

	error = BF_WriteBlock(fd, 0);
		if (error != 0)
			return error;

	error = BF_CloseFile(fd);
	if (error != 0)
		return error;

	return 0;
}

HP_info* HP_OpenFile(const char *fileName)
{
	BF_Init();

	int fd = BF_OpenFile(fileName);
	if (fd < 0)
		return nullptr;

	HP_info* header_info = new HP_info;

	header_info->fileDesc = fd;

	void *block;
	int error = BF_ReadBlock(fd, 0, &block);
	if (error != 0)
		return nullptr;

	char type[3];
	memcpy(type, (char *)block, 3);
	// Check if the file type is compatible.
	if (strcmp(type, "HP") != 0)
		return nullptr;

	// Copy the data to the struct.
	memcpy(&(header_info->attrType), (char *)block + 3, sizeof(char));
	memcpy(header_info->attrName, ((char *)block) + 3 + sizeof(char), MAX_ATTR_NAME_SIZE);
	memcpy(&(header_info->attrLength) , (char *)block + 3 + sizeof(char) + MAX_ATTR_NAME_SIZE, sizeof(int));

	return header_info;
}

int HP_CloseFile(HP_info* header_info)
{
	int error = BF_CloseFile(header_info->fileDesc);
	if (error != 0)
		return error;

	delete header_info;

	return 0;
}

int ReadNumOfRecords(void* block)
{
	int num_of_records;

	memcpy(&num_of_records, (char *)block + BLOCK_SIZE - sizeof(int) * 2, sizeof(int));

	return num_of_records;
}

void WriteNumOfRecords(void* block, int recordNumber)
{
	memcpy((char *)block + BLOCK_SIZE - sizeof(int) * 2, &recordNumber, sizeof(int));
}

int ReadNextBlockAddr(void* block)
{
	int next_block_addr;

	memcpy(&next_block_addr, (char *)block + BLOCK_SIZE - sizeof(int), sizeof(int));

	return next_block_addr;
}

void WriteNextBlockAddr(void* block, int blockAddrNumber)
{
	memcpy((char *)block + BLOCK_SIZE - sizeof(int), &blockAddrNumber, sizeof(int));
}

void ReadRecord(void* block, int recordNumber, Record* record)
{
	memcpy(record, (char *)block + recordNumber * sizeof(Record), sizeof(Record));
}

void WriteRecord(void* block, int recordNumber, const Record* record)
{
	memcpy((char *)block + recordNumber * sizeof(Record), record, sizeof(Record));
}

int InitBlock(int fileDesc, void** block)
{
	int error = BF_AllocateBlock(fileDesc);
	if (error != 0)
		return error;

	int new_block_addr = BF_GetBlockCounter(fileDesc) - 1;
	if (new_block_addr < 0)
		return new_block_addr;

	BF_ReadBlock(fileDesc, new_block_addr, block);
	if (error != 0)
		return error;

	WriteNumOfRecords(*block, 0);
	WriteNextBlockAddr(*block, -1);

	return 0;
}

int IsKeyInBlock(Record* record, void* block)
{
	int num_of_records = ReadNumOfRecords(block);

	Record tmp_record;

	for (int i = 0; i < num_of_records; i++)
	{
		ReadRecord(block, i, &tmp_record);

		if (record->id == tmp_record.id)
			return i;
	}

	return -1;
}

int HP_InsertEntry(HP_info header_info, Record record)
{
	int num_of_blocks = BF_GetBlockCounter(header_info.fileDesc);
	if (num_of_blocks < 0)
		return -1;

	int should_init_block = 0;
	// Only the header.
	if (num_of_blocks == 1)
	{
		num_of_blocks = 2; // We will create the second block later.
		should_init_block = 1;
	}

	void* block = nullptr;

	int available_block_addr = -1; // a block with space to add a record.
	int curr_block_addr = 1; // 1 cause we don't need to read the header.

	// Loop until you have read all the blocks.
	for (;;curr_block_addr++)
	{
		if (should_init_block == 0)
		{
			if (BF_ReadBlock(header_info.fileDesc, curr_block_addr, &block) != 0)
				return 1;
		}

		else
		{
			// Create a block and initialize some values.
			if (InitBlock(header_info.fileDesc, &block) == -1)
				return -1;

			should_init_block = 0;
		}

		if (IsKeyInBlock(&record, block) > -1)
			return -1;

		int num_of_records = ReadNumOfRecords(block);

		// If we haven't enough space for another record.
		if (num_of_records + 1 > MAX_RECORDS_IN_BLOCK)
		{
			int next_block_addr = ReadNextBlockAddr(block);

			// If there isn't a next block and we haven't found an available address for a record.
			if (next_block_addr == -1 && available_block_addr == -1)
			{
				WriteNextBlockAddr(block, curr_block_addr + 1);
				BF_WriteBlock(header_info.fileDesc, curr_block_addr);

				should_init_block = 1;

				num_of_blocks++;
			}
		}

		// If we have space to store the block and haven't assigned one yet.
		else if (available_block_addr == -1)
			available_block_addr = curr_block_addr;

		// If we have reached the last block.
		if (curr_block_addr == num_of_blocks - 1)
		{
			// Only read when necessary.
			if (curr_block_addr != available_block_addr)
			{
				if (BF_ReadBlock(header_info.fileDesc, available_block_addr, &block) != 0)
					return -1;
			}

			WriteRecord(block, num_of_records, &record);
			WriteNumOfRecords(block, num_of_records + 1);

			if (BF_WriteBlock(header_info.fileDesc, available_block_addr) != 0)
				return -1;

			break;
		}
	}

	return available_block_addr;
}

int AssignKeyToRecord(Record* record, void* value, char key_type)
{
	switch (key_type)
	{
	case 'i':
		record->id = *(int *)value;
		return 0;
	case 'c':
		record->id = *(char *)value;
		return 0;
	default:
		return 1;
	}
}

int IsBlockEmpty(int file_desc)
{
	return BF_GetBlockCounter(file_desc) < 1;
}

void ReplaceWithLastRecord(int pos, void* block)
{
	int num_of_records = ReadNumOfRecords(block);

	// Last record, no need to do anything.
	if (pos == num_of_records - 1)
		return;

	Record record;
	ReadRecord(block, num_of_records - 1, &record);
	WriteRecord(block, pos, &record);
}

int HP_DeleteEntry(HP_info header_info, void *value)
{
	if (IsBlockEmpty(header_info.fileDesc))
		return -1;

	void* block;
	Record record;
	if (AssignKeyToRecord(&record, value, header_info.attrType) != 0)
		return 1;

	int num_of_blocks = BF_GetBlockCounter(header_info.fileDesc);

	for (int curr_block_addr = 1; curr_block_addr < num_of_blocks; curr_block_addr++)
	{
		if (BF_ReadBlock(header_info.fileDesc, curr_block_addr, &block) != 0)
			return -1;

		int pos = IsKeyInBlock(&record, block);
		if (pos > -1)
		{
			ReplaceWithLastRecord(pos, block);

			// Lower num_of_records.
			WriteNumOfRecords(block, ReadNumOfRecords(block) - 1);

			BF_WriteBlock(header_info.fileDesc, curr_block_addr);

			return 0;
		}
	}

	// Key wasn't found.
	return -1;
}

int HP_GetAllEntries(HP_info header_info, void* value)
{
	if (IsBlockEmpty(header_info.fileDesc))
		return -1;

	void* block;
	int curr_block_addr = 1;
	Record record;

	for (;;curr_block_addr++)
	{
		if (BF_ReadBlock(header_info.fileDesc, curr_block_addr, &block) != 0)
			return -1;

		if (AssignKeyToRecord(&record, value, header_info.attrType) != 0)
			return -1;

		int record_pos = IsKeyInBlock(&record, block);
		if (record_pos > -1)
		{
			ReadRecord(block, record_pos, &record);

			std::cout << "id: " << record.id
					  << "\nname: " << record.name
					  << "\nsurname: " << record.surname
					  << "\naddress: " << record.address
					  << std::endl;

			return curr_block_addr;
		}
	}

	return -1;
}

int HT_HP_GetAllEntries(HT_info* header_info, void* value, int heap_addr)
{
  if (heap_addr == 0)
    return -1;

	void* block;
	int curr_block_addr = heap_addr;
	Record record;
  int counter = 0; // Counts how many blocks were read until we found the key.

  while (curr_block_addr != -1)
	{
    counter++;

		if (BF_ReadBlock(header_info->fileDesc, curr_block_addr, &block) != 0)
			return -1;

		if (AssignKeyToRecord(&record, value, header_info->attrType) != 0)
			return 1;

		int record_pos = IsKeyInBlock(&record, block);
		if (record_pos > -1)
		{
			ReadRecord(block, record_pos, &record);

			std::cout << "id: " << record.id
					  << "\nname: " << record.name
					  << "\nsurname: " << record.surname
					  << "\naddress: " << record.address
					  << std::endl;

			return counter;
		}

    curr_block_addr = ReadNextBlockAddr(block);
	}
	return -1;
}

int HT_HP_GetRecordCounter(HT_info* header_info, int heap_addr)
{
	if (heap_addr == 0){
		return -1;
	}

	void* block;
	int curr_block_addr = heap_addr;
	int counter = 0; // Counts how many records there are in a heap.

	while (curr_block_addr != -1)
	{
		if (BF_ReadBlock(header_info->fileDesc, curr_block_addr, &block) < 0)
			return -1;

		counter += ReadNumOfRecords(block);

    	curr_block_addr = ReadNextBlockAddr(block);
	}

	return counter;
}

int HT_HP_GetBlockCounter(HT_info* header_info, int heap_addr)
{
  if (heap_addr == 0)
    return -1;

	void* block;
	int curr_block_addr = heap_addr;
  	int counter = 0; // Counts how many blocks there are in a heap.

  while (curr_block_addr != -1)
	{
		if (BF_ReadBlock(header_info->fileDesc, curr_block_addr, &block) != 0)
			return -1;

		counter++;

    	curr_block_addr = ReadNextBlockAddr(block);
	}

	return counter;
}

int HT_HP_DeleteEntry(HT_info* header_info, void* value, int heap_address)
{
  if (heap_address == 0)
    return -1;

  void* block;
	Record record;
	if (AssignKeyToRecord(&record, value, header_info->attrType) != 0)
		return 1;

  int curr_block_addr = heap_address;

  while (curr_block_addr != -1)
	{
		if (BF_ReadBlock(header_info->fileDesc, curr_block_addr, &block) != 0)
			return -1;

		int pos = IsKeyInBlock(&record, block);
		if (pos > -1)
		{
			ReplaceWithLastRecord(pos, block);

			// Lower num_of_records.
			WriteNumOfRecords(block, ReadNumOfRecords(block) - 1);

			BF_WriteBlock(header_info->fileDesc, curr_block_addr);

			return 0;
		}

    curr_block_addr = ReadNextBlockAddr(block);
	}

	// Key wasn't found.
	return -1;
}

int HT_HP_InsertEntry(HT_info* header_info, Record* record, int heap_address)
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

	// return heap_address;
	return curr_block_addr;
}
