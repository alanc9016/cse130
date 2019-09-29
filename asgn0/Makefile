#------------------------------------------------------------------------------
# Makefile for CSE130g Assignment 0
#
#       make                   makes dog
#------------------------------------------------------------------------------

all : dog
	gcc -o dog dog.o

dog : dog.o
	gcc -o dog dog.o

dog.o : dog.cpp
	gcc -c -std=gnu++11 -O2 -Wall -Wextra -Wpedantic -Wshadow dog.cpp

clean :
	rm -f dog.o

spotless :
	rm -f dog dog.o
