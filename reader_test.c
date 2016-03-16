#include <stdio.h>
#include <string.h>
// #include "reader.h"

int main(int argc, char const *argv[])
{
	FILE *file = tmpfile();
	write(file->_file, "first line\r\nanother line\r\n", 30);
	lseek(file->_file, 0, SEEK_SET);
	// char content[20];
	// read(file->_file, content, 10);
	char got[256];
	conn_scan(file->_file, "\r\n\0", got);
	char *expect = "first line\r\n";
	if (strcmp(got, expect) != 0) {
		printf("expect: %s\ngot: %s\n", expect, got);
	}
	return 0;
}
