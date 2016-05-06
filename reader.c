#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
// #include <darray.h>

#include "debug.h"
// #include "reader.h"

struct creader {
	int conn;
	char peek;
};

int creader_read(struct creader *r, char *byte)
{
	if (r->peek != '\0') {
		*byte = r->peek;
		r->peek = '\0';
		return 1;
	}

	int n = read(r->conn, byte, 1);

	return n;
}

int creader_peek(struct creader *r)
{
	int n = read(r->conn, &(r->peek), 1);
	return n;
}

// delimiter is included in line.
int conn_scan(int conn, char *delimiter, char *line)
{
	// dprintf("sizeof(void *) = %d\n", sizeof(void *));
	// TODO: need to expand if necessary
	// *line = (char *)calloc(256, sizeof(char));
	char byte;
	struct creader r = {.conn = conn};
	char *dcurrent = delimiter;
	int matched = 0;
	int count = 0;
	for (int n = creader_read(&r, &byte); n > 0; n = creader_read(&r, &byte)) {
		// dprintf("%d ", *dcurrent);
		line[count] = byte;
		count++;
		if (byte == *dcurrent) {
			dcurrent++;
			matched++;
			if (*dcurrent == '\0') {
				// dprintf("break\n");
				// DArray_push(array, '\0');
				break;
			}
		} else if (matched > 1) {
			// int count = strlen(*line);
			// int omatched = matched;
			matched--;
			while (matched > 0) {
				dcurrent = delimiter;
				for (int rematched = 0; rematched < matched; rematched++) {
					if (*dcurrent != line[count-matched+rematched-1]) {
						matched--;
						break;
					} else {
						dcurrent++;
					}
				}
			}
		}
	}

	return count;
}
