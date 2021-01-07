#include <iostream>

#include "HT.h"
#include <cstring>
#include "BF.h"

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

	char my_db[15] = "my_db";
	HT_CreateIndex(my_db, 'i', "id", 14, 126+8);

	HT_info* index = HT_OpenIndex(my_db);

	//Insert the entries inside the database.
	for (int i = 0; i < NUM_OF_ENTRIES; i++)
	{
		if (HT_InsertEntry(*index, items[i]) < 0)
		{
			cout << "There was an error in the insertion of entry " << items[i].id << endl;
			return 1;
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

	// Delete the entries.
	for (auto entry : entries_to_delete)
	{
		if (HT_DeleteEntry(*index, &entry) == -1)
		{
			cout << "There was an error in the deletion of the entry: " << entry;
			return 1;
		}
	}

	// Check if any of the deleted entries exist.
	for (auto entry : entries_to_delete)
	{
		if (HT_GetAllEntries(*index, &entry) != -1)
		{
			cout << "The entry: " << entry << " was found." << endl;
			return 1;
		}
	}

	if(HashStatistics(my_db)<0){
		cout << "Hash returned error" << endl;
		return -1;
	}
	return 0;
}
