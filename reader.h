#ifndef _reader_h
#define _reader_h

struct creader {
	int conn;
	char peek;
};

int creader_read(struct creader *r, char *byte);
int creader_peek(struct creader *r, char *byte);
int conn_scan(int conn, char *delimiter, char **line);

#endif
