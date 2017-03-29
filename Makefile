all: 
	g++ -o test.out test.cpp bmp.cpp
clean:
	rm -f *.out
	rm -f images/*.bmp
