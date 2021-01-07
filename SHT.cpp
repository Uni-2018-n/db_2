#include "SHT.h"
#include "HP.h"
#include "BF.h"

#include <cstring>

int SHT_CreateSecondaryIndex(char* sfileName, char* attrName, int attrLength, int buckets, char* fileName)
{
    if (strlen(sfileName) > MAX_FILE_NAME_SIZE - 1 ||
        strlen(fileName) > MAX_FILE_NAME_SIZE - 1)
            return -1;

    BF_Init();

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
    
    // TODO: Read stuff from the old file here.

    if (BF_CreateFile(sfileName) < 0)
        return -1;

    int new_file = BF_OpenFile(sfileName);
    if (new_file < 0)
        return -1;
    
    if (BF_ReadBlock(new_file, 0, &block) < 0)

    if (BF_AllocateBlock(new_file) < 0)
        return -1;

    if (BF_ReadBlock(new_file, 0, &block) < 0)    
        return -1;
    
    // Save data to block.
    memcpy((char *)block, "SHT", 4); //safety to know that we have the corect file
	memcpy((char *)block + 4, &attrLength, sizeof(int));
	memcpy((char *)block + 4 + sizeof(int), attrName, attrLength);
	memcpy((char *)block + 4 + sizeof(int) + attrLength, &buckets, sizeof(int));
	memcpy((char *)block + 4 + sizeof(int) * 2 + attrLength, fileName, MAX_FILE_NAME_SIZE);

    BF_WriteBlock(new_file, 0);
    
    // TODO: Write stuff to new_file here.

    if (BF_CloseFile(new_file) < 0)
        return -1;

    if (BF_CloseFile(old_file) < 0)
        return -1;

    return 0;
}

SHT_info* SHT_OpenSecondaryIndex(char* sfileName)
{
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
    if (strcmp(type, "SHT") != 0)
        return nullptr;

	memcpy(&(info->attrLength), (char *)block + 4, sizeof(int));

    info->attrName = new char[info->attrLength];
	memcpy(&(info->attrName), (char *)block + 4 + sizeof(int), info->attrLength);

	memcpy(&(info->numBuckets), (char *)block + 4 + sizeof(int) + info->attrLength, sizeof(int));
	memcpy(&(info->fileName), (char *)block + 4 + sizeof(int) * 2 + info->attrLength, MAX_FILE_NAME_SIZE);

    // TODO: Do stuff here.

    return info;
}

int SHT_CloseSecondaryIndex(SHT_info* header_info)
{
    return 0;
}

int SHT_SecondaryInsertEntry(SHT_info header_info, SecondaryRecord record)
{
    return 0;
}