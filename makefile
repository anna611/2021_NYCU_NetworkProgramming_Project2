all: np_simple np_single_proc np_multi_proc

np_simple: np_simple.o 
	g++ -o np_simple np_simple.o 

np_simple.o: np_simple.cpp
	g++ -c np_simple.cpp

np_single_proc: np_single_proc.cpp
	g++ np_single_proc.cpp -o np_single_proc

np_multi_proc: np_multi_proc.cpp
	g++ np_multi_proc.cpp -o np_multi_proc -lrt

clean:
	rm np_simple np_simple.o 
	rm np_single_proc 
	rm np_multi_proc
