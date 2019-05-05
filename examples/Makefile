CORC=../corec
LD=$(CC)
all: fizzbuzz

fizzbuzz: ../corert.o fizzbuzz.o
	$(CC) -o fizzbuzz ../corert.o fizzbuzz.o -lc -lm

%.o: %.cor
	$(CORC) -o $@ -c $<



clean:
	rm *.o fizzbuzz

.PHONY: clean
