// Runtime library for the core language

#include <stdio.h>

extern int main(void);

float print(float f) {
	printf("%f\n", f);

	return f;
}

int printbool(int b) {
	printf("%s\n", b == 1 ? "true" : "false");
	return b;
}

int eq(float l, float r) {
	return l == r ? 1 : 0;
}

int neq(float l, float r) {
	return l != r ? 1 : 0;
}

int lnot(int b) {
	return b == 1 ? 0 : 1;
}
