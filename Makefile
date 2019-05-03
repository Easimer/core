CXXFLAGS=-g -std=c++17
all: corec

corec: ast.o parser.o main.o lexer.o
	$(CXX) $(LDFLAGS) -o corec ast.o parser.o main.o lexer.o

ast.o: ast.cpp ast.h
	$(CXX) $(CXXFLAGS) -o ast.o -c ast.cpp

parser.o: parser.cpp parser.h
	$(CXX) $(CXXFLAGS) -o parser.o -c parser.cpp

main.o: main.cpp parser.h ast.h
	$(CXX) $(CXXFLAGS) -o main.o -c main.cpp

lexer.o: lexer.cpp lexer.h
	$(CXX) $(CXXFLAGS) -o lexer.o -c lexer.cpp