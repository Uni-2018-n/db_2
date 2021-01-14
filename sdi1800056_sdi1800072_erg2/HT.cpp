#include "HT.h"
#include "BF.h"
#include "HP.h"
#include <cstring>
#include <iostream>
#define MAX_BUCKETS_IN_BLOCK ((BLOCK_SIZE - 2 * (int) sizeof(int)) / (int) sizeof(int))

using namespace std;
int HT_CreateIndex(const char *fileName, const char attrType, const char* attrName, const int attrLength, const int buckets){
  if(strlen(attrName) > MAX_ATTR_NAME_SIZE-1){ //failsafe if name is too big
    return -1;
  }
  BF_Init();

  if(BF_CreateFile(fileName) < 0){
    return -1;
  }

  int file = BF_OpenFile(fileName); //int file has the address of our file
  if(file < 0){
    return -1;
  }

  if(BF_AllocateBlock(file) <0){
    return -1;
  }

  void *block;
  if(BF_ReadBlock(file, 0, &block) <0){ //read the first block
    return -1;
  }
  //alocate space and save every data needed into the block
  memcpy((char *)block, "HT", 3); //safety to know that we have the corect file
	memcpy((char *)block + 3, &attrType, sizeof(char));
	memcpy((char *)block + 3 + sizeof(char), attrName, strlen(attrName) + 1);
	memcpy((char *)block + 3 + sizeof(char) + MAX_ATTR_NAME_SIZE , &attrLength, sizeof(int));
  memcpy((char *)block + 3 + sizeof(char) + MAX_ATTR_NAME_SIZE + sizeof(int), &buckets, sizeof(int));

  BF_WriteBlock(file, 0);


  int pl_blocks = (buckets / MAX_BUCKETS_IN_BLOCK)+ 1; //calculate how many blocks are needed for buckets
  for(int i=0;i<pl_blocks;i++){//for every block
    if(BF_AllocateBlock(file) < 0){//allocate it
      return -1;
    }
    if(BF_ReadBlock(file, BF_GetBlockCounter(file)-1, &block) < 0){//read it
      return -1;
    }
    int max;//and find how many blocks we need to add in it
    if(i == pl_blocks-1){//if its the last block
      max = buckets - MAX_BUCKETS_IN_BLOCK*i;//set as max only the number that the buckets are left to add
    }else{
      max = MAX_BUCKETS_IN_BLOCK; // else set the full number of buckets
    }//for example if we have 134 buckets we need 2 blocks(lets say i is 0 and 1 for each block) so for first block we have 126 buckets and for second we have 134-(126*1) = 8 buckets

    for(int j=0;j<max;j++){//and then with each time the correct max initialize the bucket as 0
      int temp =0;
      memcpy((char*)block+ sizeof(int)*j, &temp, sizeof(int));
    }
    BF_WriteBlock(file, BF_GetBlockCounter(file)-1);
  }

  if(BF_CloseFile(file) < 0){
    return -1;
  }

  return 0;
}

HT_info* HT_OpenIndex(char *fileName){
  int file = BF_OpenFile(fileName); //open the file and read the data saved into the block and add it into the temp HT_info variable.
  if(file <0){
    return NULL;
  }
  HT_info* temp = new HT_info;
  temp->fileDesc = file;

  void* block;
  if(BF_ReadBlock(file, 0, &block)<0){
    return NULL;
  }
  char* ht = new char[3];
  memcpy(ht, (char *)block, 3);
  if(strcmp(ht, "HT") != 0){
    return NULL;
  }
  memcpy(&(temp->attrType), (char *)block+3, sizeof(char));
  memcpy(&(temp->attrName), (char *)block + 3+1, MAX_ATTR_NAME_SIZE);
  memcpy(&(temp->attrLength), (char *)block + 3+1 + MAX_ATTR_NAME_SIZE, sizeof(int));
  memcpy(&(temp->numBuckets), (char *)block + 3 + 1 + MAX_ATTR_NAME_SIZE + sizeof(int), sizeof(int));

  return temp;
}

int HT_CloseIndex(HT_info* header_info){
  int temp =BF_CloseFile(header_info->fileDesc);
  delete header_info;
  return temp;
}

int HT_InsertEntry(HT_info header_info, Record record){
  int h = HT_function(&record.id, header_info.numBuckets);//get the hash function output for that id and return a number from 0 to numBuckets
  void* block;
  int heap=0;
  int j=0;
  int i;
  int counter=0;
  int pl_blocks = (header_info.numBuckets / MAX_BUCKETS_IN_BLOCK)+ 1;//how many blocks are used to store buckets

  for(i=0;i<pl_blocks;i++){//for each block
    if(BF_ReadBlock(header_info.fileDesc, i+1, &block) <0){//read it
      return -1;
    }

    int max;//calculate how many buckets are in the specific block as above
    heap=0;
    if(i == pl_blocks-1){
      max = header_info.numBuckets - MAX_BUCKETS_IN_BLOCK*i;
    }else{
      max = MAX_BUCKETS_IN_BLOCK;
    }
    int found =0;
    for(j=0;j<max;j++){
      memcpy(&heap, (char *)block + sizeof(int)*j, sizeof(int)); //if it is save the heap's address
      if(counter == h){//with the help of an external counter go through every block and find the correct bucket's address
        found =1;
        break;
      }
      counter++; //we use an external counter because everytime j is reset to 0 but our implementation continues until the number of buckets
    }
    if(found){//incase of found not need to go to the next block so break the loop
      break;
    }

  }
  //here we have heap with the correct block's address of the bucket with our entry inside
  int new_heap_addr = HT_HP_InsertEntry(&header_info, &record, heap); //then pass it to HT_HP_InsertEntry to add it into the heap's blocks
  if (new_heap_addr == -1){
    return -1;
  }
  if (new_heap_addr != heap){ //if the heap was empty the upper loop returned the int heap variable as 0 so we need to set the new address returned by HT_HP_InsertEntry
    memcpy((char*)block +sizeof(int)*j, &new_heap_addr, sizeof(int));
    BF_WriteBlock(header_info.fileDesc, i+1); //and save changed
  }
  return new_heap_addr;//TODO: add this to readme
}

int HT_DeleteEntry(HT_info header_info, void *value){
  int h;//same as above calcualte the hash function
  if(header_info.attrType == 'i'){
    h = HT_function((int*)value, (int)header_info.numBuckets);
  }else{
    h = HT_function((char*)value, (int)header_info.numBuckets);
  }


  void* block;
  int heap=0;
  int j;
  int i;
  int counter=0;
  int pl_blocks = (header_info.numBuckets / MAX_BUCKETS_IN_BLOCK)+ 1;//get how many blocks are needed for hash table

  for(i=0;i<pl_blocks;i++){//read each block, get the heap's address and give it to the heap function
    if(BF_ReadBlock(header_info.fileDesc, i+1, &block) <0){
      return -1;
    }

    int max;//same implementation as above
    heap=0;
    if(i == pl_blocks-1){
      max = header_info.numBuckets - MAX_BUCKETS_IN_BLOCK*i;
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

  if (HT_HP_DeleteEntry(&header_info, value, heap) != 0){ // call the HT_HP_DeleteEntry to remove the entry from the heap
    return -1;
  }

  return 0;
}

int HT_GetAllEntries(HT_info header_info, void *value){
  int h;//again same implementation as above
  if(header_info.attrType == 'i'){
    h = HT_function((int*)value, (int)header_info.numBuckets);
  }else{
    h = HT_function((char*)value, (int)header_info.numBuckets);
  }

  void* block;
  int heap=0;
  int j;
  int i;
  int counter=0;
  int pl_blocks = (header_info.numBuckets / MAX_BUCKETS_IN_BLOCK)+ 1;
  int block_counter = 0;
  for(i=0;i<pl_blocks;i++){
    block_counter++;
    if(BF_ReadBlock(header_info.fileDesc, i+1, &block) <0){
      return -1;
    }

    int max;
    heap=0;
    if(i == pl_blocks-1){
      max = header_info.numBuckets - MAX_BUCKETS_IN_BLOCK*i;
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
  int result = HT_HP_GetAllEntries(&header_info, value, heap);

  if (result == -1)
    return -1;

  else
    return block_counter + result;
}

int HT_function(int* value, int buckets){
  char temp[32];
  sprintf(temp, "%d", *value);
  unsigned int hash = 5381; // djb2 hash function from data structures class
  for(int i=0;temp[i] != '\0';i++){
    hash = (hash << 5) + hash + temp[i];
  }
  return hash % buckets;
}

int HT_function(char* value, int buckets){//same as int but for characters
  unsigned int hash = 5381;
  for(char* s= value; *s != '\0'; s++){
    hash = (hash << 5) + hash + *s;
  }
  return hash % buckets;
}



int HashStatistics(char* filename){
  HT_info* header_info = HT_OpenIndex(filename);

  int numBuckets =header_info->numBuckets;
  int pl_blocks = (numBuckets / MAX_BUCKETS_IN_BLOCK)+ 1;
  int block_counter=1 + pl_blocks;//the block counter starts with one because we have the header block, and then we add the pl of blocks used by the hash table

  int array[numBuckets];//initialize this array so we can easily store the heap's addresses

  void* block;
  int heap;
  int j;
  int i;
  int counter=0;

  for(i=0;i<pl_blocks;i++){//as above for each block read it and go through its buckets
    if(BF_ReadBlock(header_info->fileDesc, i+1, &block) <0){
      return -1;
    }

    int max;
    heap=0;
    if(i == pl_blocks-1){
      max = header_info->numBuckets - MAX_BUCKETS_IN_BLOCK*i;
    }else{
      max = MAX_BUCKETS_IN_BLOCK;
    }
    for(j=0;j<max;j++){
        memcpy(&heap, (char *)block + sizeof(int)*j, sizeof(int));
        array[counter] = heap;//store the heap into the array
      counter++;
    }
  }

  //we used an array here because inside HT_HP_* functions we have BF_ReadBlock function that changes the void* block data so its not possible for us to have the correct one
  int k=0;
  while(array[k] == 0){
    k++;
  }
  int temp = HT_HP_GetRecordCounter(header_info, array[k]);
  int min = temp;//help variables to calculate min, max and averages
  int max = temp;
  int average_records = 0;
  int average_blocks = 0;
  for(int i=0;i< numBuckets; i++){//for every bucket do every calculation to find the output
    heap = array[i];
    if(heap == 0){//if its 0 then we have an unused heap so skip it
      continue;
    }
    int num = HT_HP_GetBlockCounter(header_info, heap);//here it goes into infinite loop

    int pl_records = HT_HP_GetRecordCounter(header_info, heap);

    block_counter += num;

    if(pl_records < min){
      min = pl_records;
    }
    if(pl_records > max){
      max = pl_records;
    }
    average_blocks += num;
    average_records += pl_records;

  }
  average_blocks = average_blocks/numBuckets;
  average_records = average_records/numBuckets;

  //and print it
  cout << "Blocks used by file \"" << filename << "\": " << block_counter << endl;
  cout << "Minimum records per bucket: " << min << endl;
  cout << "Maximum records per bucket: " << max << endl;
  cout << "Average records per bucket: " << average_records << endl;

  cout << "Average number of blocks per bucket: " << average_blocks << endl;


  cout << "Overflow blocks: " << endl;//same thing with overflow
  int overflow=0;
  for(int i=0;i<numBuckets;i++){
    int heap;
    heap = array[i];
    int num = HT_HP_GetBlockCounter(header_info, heap);
    if(num > 1){//if the num is > 1 it means that there is an overflow happening
      overflow += num-1;
      cout << "For bucket " << i << ", " << num-1 << endl;
    }
  }
  cout << "Overflow sum: " << overflow << endl;

  if(HT_CloseIndex(header_info) <0){
    return -1;
  }
  return 0;
}
