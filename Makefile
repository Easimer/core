CXXFLAGS=-g -std=c++17 -O0 -fsanitize=address
LDFLAGS=-lasan
all: corec

OBJECTS=main.o lexer.o parser.o ast.o

corec: $(OBJECTS)
	$(CXX) -o corec $(OBJECTS) $(LDFLAGS)

%.o: %.cpp types.h
	$(CXX) $(CXXFLAGS) -o $@ -c $<

clean:
	rm *.o corec

.PHONY: clean