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

	char my_db[15] = "my_db_primary";
	HT_CreateIndex(my_db, 'i', "id", 14, 126+8);

	char my_db_secondary[16] = "my_db_secondary";
	SHT_CreateSecondaryIndex(my_db_secondary, "surname", 25, 126+8, my_db);

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
			if(SHT_SecondaryInsertEntry(*secondary_index, t) < 0){
				cout << "There was an error in the insertion of Secondary entry " << items[i].id << endl;
				return 1;
			}
		}
	}

	int entries_to_delete[] = {1, 18, 25, 62, 32, 116, 99, 442, 482};

	// Check if the entries exist.
	for (auto entry : entries_to_delete)
	{
		if (HT_GetAllEntries(*index, &entry) == -1)
		{
			cout << "The entry: " << entry << " wasn't found." << endl;
			return 1;
		}
	}

	if(HashStatistics(my_db)<0){
		cout << "Hash returned error" << endl;
		return -1;
	}
	return 0;
}
