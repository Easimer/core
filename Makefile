CC=gcc
CXX=g++
LLVM_CFG=llvm-config-7
CXXFLAGS_LLVM := $(shell $(LLVM_CFG) --cppflags)
CXXFLAGS=-g -std=c++17 -O0 -Wall $(CXXFLAGS_LLVM)
LDFLAGS_LLVM := $(shell $(LLVM_CFG) --link-static --ldflags --system-libs --libs all)
LDFLAGS=$(LDFLAGS_LLVM)
all: corec example.exe

OBJECTS=main.o lexer.o parser.o ast.o log.o

corec: $(OBJECTS)
	$(CXX) -o corec $(OBJECTS) $(LDFLAGS)

%.o: %.cpp types.h
	$(CXX) $(CXXFLAGS) -o $@ -c $<

stdafx.h.pch: stdafx.h
	$(CXX) $(CXXFLAGS) -x c++-header -o stdafx.h.pch -c stdafx.h

corert.o: corert.c
	$(CC) -o corert.o -c corert.c

example.o: example.cor corec
	./corec -o example.o -c example.cor
example.exe: corert.o example.o
	$(CC) -o example.exe corert.o example.o -lc -lm


clean:
	rm -f *.o corec example.exe stdafx.h.pch

.PHONY: clean
