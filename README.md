Η εργασία έγινε από τους:
	* Αντώνης Καλαμάκης (sdi1800056)
	* Χρήστος Καφρίτσας  (sdi1800072)

We have changed the file BF.h in the following ways:
	* Added extern in the variable BF_Errno.
	* Added extern "C" in a #ifdef __cplusplus, in order for
	the code to compile under g++.

Απο εργασια 1:
In the main.cpp file we chose to use the HT database, because, it uses some of the HP functions.

When we create the databases (in both hp and ht),
we write "HP" or "HT", in the start, so we can check if the file
opened is valid for the database.

HP.cpp
* The functions behave in the expected way.
* There is a limit (15 characters) in the key's name,
when we use HP_CreateFIle because we need to store it in a struct.
* The function HP_GetAllEntries, stops after finding the value, specified because we only search for keys and not other attributes in this project.
* The program can support char type keys, with little modification.
* In the HP.[cpp/h] files we have included the HT_HP_* functions,
which are very similar to the HP ones but also different because
they are used for the hash based database.
Because of that we need both HP and HT files for a successful compilation of a HT program and only HP files for a HP one.
* Blocks are stored one after another in the file.
In the end of the block,
I write the number of Records that are inside the file and
the <address> of the the next block. (The address is mostly used to know if this is the last block in the heap.)
* When I delete a Record in a block I swap it with the last one inside that block. (Except for the case that it is the only block)
* If I delete all Records from a block, the block stays empty.

HT.cpp:

For HT_CreateIndex:
To create a file we need to use BF_Init and then use the BF_Createfile function.
After that opening the file is needed so we can allocate and read all the blocks.
In our implementation we have a header block, the 0 block of our file here we
save the data provided from the user.
To initialize the block(s) used to save the heap addresses we first need to find
how many buckets we have. This is happening by dividing the number of buckets
with the maximum number of buckets that a block can store.
And to allocate them. Because of the fact that in this simulator the block
addresses are not "random"  we take advantage that the bucket-blocks are created
at the initialization so they always have a number of 1-x (where x is the number
of blocks) so it's easier for us to use. If this wasn't legal to do in the places
of this project we would have to repurpose the last part of the block (with
size of int) as the address number for our next block.
To be possible for me to know how many buckets I need to store into a block I
see if the block we are now is the last block, if not then the max buckets per
block will be added, if we are in the last block we abstract the number of
buckets that we already added from the full number of buckets so we will add
the remaining buckets.
For every bucket, as initialization we set the value of 0 so the program will
know that there are no entries inside the bucket and for every block we write
it to save the changes.
Finally because of the fact that the function only initializes the file we
close it.

For HT_OpenIndex:
first of all the HT_info struct's variables, for this implementation, are
enough to store the data we need so no more variables needed.
With that being said this function is pretty straight forward, first we need to
open the file and then store into a temp variable the data stored into the
header block so its more easily available to the user.

For HT_CloseIndex:
Pretty straight forward, close the file and delete the HT_info allocated into
HT_openIndex.

For HT_InsertEntry:
First of all a bit for the hash function used in this implementation, it's the
djb2 hash function provided by K08 lesson. We used it there and it's easy to
recreate. This function originally was for strings but we turned into an int
function because the id is an integer. Finally, there is a second hash function
in case the user decides to give as a key a string instead of int. And returning
a number that goes from 0 to the number of buckets we have.

For insert, we used the hash function first to see the bucket we are in.
After that initialize some variables and do the same thing as HT_CreateIndex to
go through all the buckets in all the blocks (if more than one). The only
difference is for every bucket we see if the number of bucket is the same as
the hash function returned, if yes we read the heap's address for  that bucket
and send it into HT_HP_InsertEntry to add the entry in to that specific heap.
That function returns the address of that heap or -1, if the heap is different
than our original heap number, that we found from the bucket search, we have to
change the heaps address inside the block to update it(this means that the
original heap number was 0 so we gave it a real address number).

For HT_DeleteEntry:
it's the same exact logic with HT_InsertEntry but with a different HT_HP_
function at the end, for the correct purpose.

For HT_GetAllEntries:
exactly as before but now we have an extra counter that counts the number of
blocks we needed to go through to find our bucket and also at the end there is
a different HT_HP_ function. The extra counter is needed because this function
needs to return the number of blocks that we needed to go through to find that
entry.

For HashStatistics:
Because of the fact that we will not make any changes into the buckets or the
hash table it was easier for us to create an array and do one last time the
same thing that we have done to go through the bucket-blocks and store it into the
array.
After that we used some basic variables and initialized some of them to be able
to find the minimum, maximum and average values that we need for our statistics.
Then we have gone through every bucket and called some HT_HP_ functions to return
the block counter and the record counter of every bucket, if the heap value was
zero there is no need for us to go through it so we skipped it(zero means
bucket is empty).
After that we calculated and printed the statistics.
To find the overflow blocks its pretty straight forward, we get the block
counter for every heap, if it's more than one then we have an overflow, so we
add it to the counter and print how many overflowed blocks we have for the
specific bucket and finally print the overflow sum.

Because of the fact that this function is not an implementation of the hash
table we thought that the file must be done with any processing and to be
closed. So in this function we open and close the file.

Απο Εργασια 2:
For this practice we used the already implemented functions used for the Hash table
and changed some minor stuff to make it work with the new record type. The only
things needed to be changed was the variables that before was Record now changed
into SecondaryRecord and every compare was made with the int id as key now needed
to be change to the char surname.
One major change was that the value that the HT_InsertEntry provided now is getting
used by SHT_SecondaryInsertEntry so we can store the block id of the block that we
just stored our record.
Finally the last major change was inside SHT_HP_GetAllEntries now instead of
just printing the information we pass it to SHT_HP_GetAllEntries_T so it will find
the correct record inside the primary's block.

Inside SHT_HashStatistics nothing really changed except that now we calculate the
 statistics based on a SHT_info file. Opening and closing the HT_info file was
 just for formality.

All the helping functions created when implemented HT was recreated for SHT
but now with diffrent record members and compared variables(if needed).

Because of the fact that we've already implemented the HashFunction for chars
we just used it from the HT file.
