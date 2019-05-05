// Runtime library for the core language

#include <stdio.h>

// Core entry point
extern int Main(void);

double print(double f) {
	printf("%f\n", f);

	return f;
}

int printbool(int b) {
	printf("%s\n", b == 1 ? "true" : "false");
	return b;
}

int lnot(int b) {
	return b == 1 ? 0 : 1;
}

int main(int argc, char** argv) {
	return Main() ? 0 : -1;
}
