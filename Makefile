FLAGS = -std=c++11 -Wall -O0 -g
SOURCE = main.cpp HT.cpp HP.cpp SHT.cpp SHT_HP.cpp
FULL_SOURCE = main.cpp HT.h HT.cpp HP.h HP.cpp SHT.h SHT.cpp SHT_HP.cpp SHT_HP.h
ITEM = main

main : $(FULL_SOURCE)
	g++ $(FLAGS) $(SOURCE) -L. -static -l\:BF_64.a -o $(ITEM)
