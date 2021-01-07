FLAGS = -std=c++11 -Wall -O0 -g
SOURCE = main.cpp HT.cpp HP.cpp
ITEM = main

clean : run
	rm -rf $(ITEM)
	rm -rf *.o

run : compile
	./$(ITEM)

debug : $(ITEM)
	gdb $(ITEM)

compile : $(SOURCE)
	# gcc $(FLAGS) $(ITEM) $(SOURCE)
	g++ $(FLAGS) $(SOURCE) -L. -static -l\:BF_64.a -o $(ITEM)
