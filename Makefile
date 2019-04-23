CXXOPT =-std=c++14 -O3 

all: compile

compile: main.o smt.o
	$(CXX) $(CXXOPT) main.o smt.o -o tree
main.o: main.cpp
	$(CXX) -c $(CXXOPT) main.cpp
smt.o: smt.cpp
	$(CXX) -c $(CXXOPT) smt.cpp

clean:
	rm *.o
