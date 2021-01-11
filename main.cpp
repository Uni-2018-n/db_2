#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "HT.h"
#include "BF.h"
#include "SHT.h"

#define NUM_OF_ENTRIES 1000

using std::cout;
using std::endl;

int FindCharacter(char *str, char character)
{
	for (int i = 0; i < (int) strlen(str); i++)
	{
		if (str[i] == character)
			return i;
	}

	return -1;
}

void ReadRecord(char* buffer, Record& my_record)
{
		int pos = 1; // Line starts with {.

		sscanf(buffer + pos, "%d", &(my_record.id));

		pos += FindCharacter(buffer + pos, '"') + 1;

		int word_size = FindCharacter(buffer + pos, '"');	

		memcpy(my_record.name, buffer + pos, word_size);
		my_record.name[word_size] = '\0';

		pos += word_size + 3;

		word_size = FindCharacter(buffer + pos, '"');	

		memcpy(my_record.surname, buffer + pos, word_size);
		my_record.surname[word_size] = '\0';

		pos += word_size + 3;

		word_size = FindCharacter(buffer + pos, '"');	

		memcpy(my_record.address, buffer + pos, word_size);
		my_record.address[word_size] = '\0';
}

int main()
{
	char my_db[15] = "my_dbp";
	HT_CreateIndex(my_db, 'i', "id", 14, 126+8);

	char my_db_secondary[15] = "my_dbs";
	if(SHT_CreateSecondaryIndex(my_db_secondary, "surname", 25, 126+8, my_db) < 0){
		cout << "error at create secondary index" << endl;
	}

	HT_info* index = HT_OpenIndex(my_db);
	SHT_info* secondary_index = SHT_OpenSecondaryIndex(my_db_secondary);

	FILE* fp = fopen("records/records5K.txt", "r");
	if (fp == NULL)
		return 1;

	char *buffer = NULL;
	size_t max_len = 128;

	// Read file line, by line.
	while (getline(&buffer, &max_len, fp) != -1)
	{
		Record my_record;

		// Read line and copy contents to my_record.
		ReadRecord(buffer, my_record);

		int temp = HT_InsertEntry(*index, my_record);

		if (temp < 0)
		{
			cout << "There was an error in the insertion of entry " << my_record.id << endl;
			return 1;
		}

		else
		{
			SecondaryRecord t;
			strcpy(t.surname, my_record.surname);
			t.blockId = temp;
			int tt = SHT_SecondaryInsertEntry(*secondary_index, t);
			if(tt < 0){
				cout << "There was an error in the insertion of Secondary entry " << my_record.id << endl;
				return 1;
			}
		}
	}

	int ht_entries_to_check[] = {4999, 1, 18, 25, 62, 32, 116, 99, 442};
	const char* sht_entries_to_check[] = {"surname_4999", "surname_1", "surname_18", "surname_25", "surname_62", "surname_32", "surname_116", "surname_99", "surname_442"};
	// Check if the entries exist.
	for(int i=0; i<9; i++){
		if (SHT_SecondaryGetAllEntries(*secondary_index, *index, (char *)sht_entries_to_check[i]) == -1){
			cout << "SHT-> The entry: " << sht_entries_to_check[i] << " wasn't found." << endl;
			return 1;
		}else{
			cout << endl;
		}

		if (HT_GetAllEntries(*index, &(ht_entries_to_check[i])) == -1)
		{
			cout << "HT-> The entry: " << ht_entries_to_check[i] << " wasn't found." << endl;
			return 1;
		}
		else{
			cout << endl;
		}
	}

	HT_CloseIndex(index);
	SHT_CloseSecondaryIndex(secondary_index);

	if(SHT_HashStatistics(my_db_secondary)<0){
		cout << "Hash returned error" << endl;
		return -1;
	}

	return 0;
}
