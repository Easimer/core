CC=clang-7
CXX=clang++-7
LLVM_CFG=llvm-config-7
CXXFLAGS_LLVM := $(shell $(LLVM_CFG) --cxxflags)
CXXFLAGS=-g -std=c++17 -O0 -Wall $(CXXFLAGS_LLVM)
LDFLAGS_LLVM := $(shell $(LLVM_CFG) --ldflags --system-libs --libs core)
LDFLAGS=$(LDFLAGS_LLVM)
all: corec example.exe

OBJECTS=main.o lexer.o parser.o ast.o log.o

corec: $(OBJECTS)
	$(CXX) -o corec $(OBJECTS) $(LDFLAGS)

%.o: %.cpp types.h stdafx.h.pch
	$(CXX) $(CXXFLAGS) -include-pch stdafx.h.pch -o $@ -c $<

stdafx.h.pch: stdafx.h
	$(CXX) $(CXXFLAGS) -x c++-header stdafx.h -o stdafx.h.pch

corert.o: corert.c
	$(CC) -o corert.o -c corert.c

example.o: example.cor corec
	./corec -o example.o -c example.cor
example.exe: corert.o example.o
	$(CC) -o example.exe corert.o example.o -lc -lm


clean:
	rm *.o corec example.exe stdafx.h.pch

.PHONY: clean
