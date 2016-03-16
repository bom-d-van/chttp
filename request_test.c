#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "chttp.h"

int main()
{
	FILE *file = tmpfile();

	fputs("GET /test test test HTTP/1.1\r\n", file);
	fputs("Host: localhost:4000\r\n", file);
	fputs("User-Agent: curl/7.43.0\r\n", file);
	fputs("Accept: */*\r\n", file);
	fputs("Content-Length: 56\r\n", file);
	fputs("Content-Type: application/x-www-form-urlencoded\r\n", file);
	fputs("\r\n", file);
	fputs("image_id=ami-84db39ed&hwp_id=m1.small&keyname=marios_key\r\n", file);
	fflush(file);
	lseek(file->_file, 0, SEEK_SET);

	Request request = {.conn = file->_file};
	Request_parse_request_line(&request);
	Request_parse_headers(&request);

	char *want, *got;
	want = "GET";
	got = request.method;
	if (strcmp(want, got) != 0) {
		printf("request.method = %s; want %s\n", got, want);
		return 1;
	}
	want = "/test test test";
	got = request.url;
	if (strcmp(want, got) != 0) {
		printf("request.url = %s; want %s\n", got, want);
		return 1;
	}
	want = "HTTP/1.1";
	got = request.version;
	if (strcmp(want, got) != 0) {
		printf("request.version = %s; want %s\n", got, want);
		return 1;
	}

	int headerNum = 5;
	if (request.headerNum != headerNum) {
		printf("request.headerNum = %d; want %d\n", request.headerNum, headerNum);
		return 1;
	}
	want = "Host";
	got = request.headers[0].name;
	if (strcmp(want, got) != 0) {
		printf("request.headers[0].name = %s; want %s\n", got, want);
		return 1;
	}
	want = "localhost:4000";
	got = request.headers[0].value;
	if (strcmp(want, got) != 0) {
		printf("request.headers[0].value = %s; want %s\n", got, want);
		return 1;
	}
	want = "User-Agent";
	got = request.headers[1].name;
	if (strcmp(want, got) != 0) {
		printf("request.headers[1].name = %s; want %s\n", got, want);
		return 1;
	}
	want = "curl/7.43.0";
	got = request.headers[1].value;
	if (strcmp(want, got) != 0) {
		printf("request.headers[1].value = %s; want %s\n", got, want);
		return 1;
	}
	want = "Accept";
	got = request.headers[2].name;
	if (strcmp(want, got) != 0) {
		printf("request.headers[2].name = %s; want %s\n", got, want);
		return 1;
	}
	want = "*/*";
	got = request.headers[2].value;
	if (strcmp(want, got) != 0) {
		printf("request.headers[2].value = %s; want %s\n", got, want);
		return 1;
	}

	Request_parse_form(&request);
	int wantNum = 3;
	int gotNum = request.formNum;
	if (wantNum != gotNum) {
		printf("request.formNum = %d; want %d\n", gotNum, wantNum);
		return 1;
	}
	want = "image_id";
	got = request.form[0].name;
	if (strcmp(want, got) != 0) {
		printf("request.form[0].name = %s; want %s\n", got, want);
		return 1;
	}
	want = "ami-84db39ed";
	got = request.form[0].value;
	if (strcmp(want, got) != 0) {
		printf("request.form[0].value = %s; want %s\n", got, want);
		return 1;
	}
	want = "hwp_id";
	got = request.form[1].name;
	if (strcmp(want, got) != 0) {
		printf("request.form[1].name = %s; want %s\n", got, want);
		return 1;
	}
	want = "m1.small";
	got = request.form[1].value;
	if (strcmp(want, got) != 0) {
		printf("request.form[1].value = %s; want %s\n", got, want);
		return 1;
	}
	want = "keyname";
	got = request.form[2].name;
	if (strcmp(want, got) != 0) {
		printf("request.form[2].name = %s; want %s\n", got, want);
		return 1;
	}
	want = "marios_key";
	got = request.form[2].value;
	if (strcmp(want, got) != 0) {
		printf("request.form[2].value = %s; want %s\n", got, want);
		return 1;
	}

	return 0;
}
