all: np_simple 

np_simple: np_simple.o 
	g++ -o np_simple np_simple.o 

np_simple.o: np_simple.cpp
	g++ -c np_simple.cpp

clean:
	rm np_simple np_simple.o 
