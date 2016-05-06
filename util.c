#include <util.h>
#include <stdio.h>

void print_hex(const char *s, int size)
{
	for (int i = 0; i < size; i++) {
		printf("%02hhX", s[i]);
		printf(" ");
		// if ((i+1) % 2 == 0) {
		// }
	}
	printf("\n");
}

// char *to_hex(const char *s, int size)
// {
// 	char *result = calloc()
// 	for (int i = 0; i < size; i++) {
// 		sprintf("%08x", (unsigned int)s[i]);
// 		// if ((i+1) % 2 == 0) {
// 		// 	printf(" ");
// 		// }
// 	}
// }
