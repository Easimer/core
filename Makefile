CXX=clang++-7
LLVM_CFG=llvm-config-7
CXXFLAGS_LLVM := $(shell $(LLVM_CFG) --cxxflags)
CXXFLAGS=-g -std=c++17 -O0 -fsanitize=address -Wall $(CXXFLAGS_LLVM)
LDFLAGS_LLVM := $(shell $(LLVM_CFG) --ldflags --system-libs --libs core)
LDFLAGS=-lasan $(LDFLAGS_LLVM)
all: corec

OBJECTS=main.o lexer.o parser.o ast.o log.o

corec: $(OBJECTS)
	$(CXX) -o corec $(OBJECTS) $(LDFLAGS)

%.o: %.cpp types.h stdafx.h.pch
	$(CXX) $(CXXFLAGS) -include-pch stdafx.h.pch -o $@ -c $<

stdafx.h.pch: stdafx.h
	$(CXX) $(CXXFLAGS) -x c++-header stdafx.h -o stdafx.h.pch

clean:
	rm *.o corec

.PHONY: clean