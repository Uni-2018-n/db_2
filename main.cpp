#include <iostream>

#include "HT.h"
#include <cstring>
#include "BF.h"
#include "SHT.h"

#define NUM_OF_ENTRIES 1000

using namespace std;
int main(){
	// Create a lot of entries.
	// (Easier than reading them from the provided files.)
	Record items[NUM_OF_ENTRIES];
	for(int i=0;i<NUM_OF_ENTRIES;i++)
	{
		items[i].id = i;
		sprintf(items[i].name, "name_%d", i);
		sprintf(items[i].surname, "surname_%d", i);
		sprintf(items[i].address, "address_%d", i);
	}

	char my_db[15] = "my_dbp";
	HT_CreateIndex(my_db, 'i', "id", 14, 126+8);

	char my_db_secondary[15] = "my_dbs";
	if(SHT_CreateSecondaryIndex(my_db_secondary, "surname", 25, 126+8, my_db) < 0){
		cout << "error at create secondary index" << endl;
	}

	HT_info* index = HT_OpenIndex(my_db);
	SHT_info* secondary_index = SHT_OpenSecondaryIndex(my_db_secondary);


	//Insert the entries inside the database.
	for (int i = 0; i < NUM_OF_ENTRIES; i++)
	{
		int temp =HT_InsertEntry(*index, items[i]);
		if (temp < 0)
		{
			cout << "There was an error in the insertion of entry " << items[i].id << endl;
			return 1;
		}else{
			SecondaryRecord t;
			strcpy(t.surname, items[i].surname);
			t.blockId = temp;
			int tt = SHT_SecondaryInsertEntry(*secondary_index, t);
			if(tt < 0){
				cout << "There was an error in the insertion of Secondary entry " << items[i].id << endl;
				return 1;
			}
		}
	}

	const char* entries_to_delete[] = {"surname_1", "surname_18", "surname_25", "surname_62", "surname_32", "surname_116", "surname_99", "surname_442", "surname_482"};
	// Check if the entries exist.
	for(int i=0; i<9; i++)
	{
		if (SHT_SecondaryGetAllEntries(*secondary_index, *index, (char*)entries_to_delete[i]) == -1)
		{
			cout << "The entry: " << entries_to_delete[i] << " wasn't found." << endl;
			return 1;
		}else{
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
