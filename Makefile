CC=clang++-5.0
CFLAGS=-std=c++1z -Wall -g -O3
IDIR=-I./include/ -I/usr/include/ -I/usr/local/include/
SRC=./main.cpp
LDIR=
LIBS=

all: $(SRC) $()
	$(CC) $(IDIR) $(CFLAGS) $(SRC) $(LDIR) $(LIBS) -o attribute-parser
