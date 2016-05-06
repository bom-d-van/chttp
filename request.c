#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "request.h"
#include "reader.h"

int last_index_of(char *line, int size, char byte)
{
	int i = size;
	line += size;
	for (; i > 0; i--) {
		if (*line == byte) {
			break;
		}
		line--;
	}

	return i;
}

// PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n
char *h2ConnPreface = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";

// TODO: fix fixed string length
int Request_parse_request_line(Request *request)
{
	char line[256];
	int count = conn_scan(request->conn, "\r\n", (char *)line);
	// printf("--------: %s\n", line);
	if (count == 16 && !strcmp(line, "PRI * HTTP/2.0\r\n")) {
		bzero(line, 256);
		conn_scan(request->conn, "\r\n", line);
		if (strcmp(line, "\r\n")) {
			return 1;
		}
		bzero(line, 256);
		conn_scan(request->conn, "\r\n", line);
		if (strcmp(line, "SM\r\n")) {
			return 1;
		}
		bzero(line, 256);
		conn_scan(request->conn, "\r\n", line);
		if (strcmp(line, "\r\n")) {
			return 1;
		}
		return 2;
	}

	request->method = calloc(8, sizeof(char));
	int findex = strcspn(line, " ");
	memcpy(request->method, line, findex * sizeof(char));

	request->version = calloc(16, sizeof(char));
	int lindex = last_index_of(line, count, ' ');
	memcpy(request->version, line+lindex+1, (count - lindex - 3) * sizeof(char));

	// printf("==============\n");
	// free(request->version);
	// printf("==============\n");

	request->url = calloc(256, sizeof(char));
	memcpy(request->url, line+findex+1, (lindex - findex -1) * sizeof(char));

	return 0;
}

int *Request_parse_headers(Request *request)
{
	char line[256];
	// TODO: fix fixed headers size
	int cap = 10;
	Header *headers = calloc(cap, sizeof(Header));
	request->headers = headers;
	request->headerNum = 0;

	int count = conn_scan(request->conn, "\r\n", line);
	while (count > 2) {
		int index = strcspn(line, ":");
		headers->name = calloc(256, sizeof(char));
		headers->value = calloc(256, sizeof(char));
		memcpy(headers->name, line, (index) * sizeof(char));
		memcpy(headers->value, line + index + 2, (count - index - 4) * sizeof(char));
		request->headerNum++;
		headers++;
		if (request->headerNum >= cap) {
			cap += 10;
			request->headers = realloc(request->headers, cap*sizeof(Header));
			headers = request->headers + request->headerNum;
		}

		count = conn_scan(request->conn, "\r\n", line);
	}

	// printf("==============\n");
	// free(request->headers);
	// printf("==============\n");

	return 0;
}

Header *Request_get_header(Request *request, char *name)
{
	Header *header = request->headers;
	for (int i = 0; i < request->headerNum; i++) {
		if (strcmp(header->name, name) == 0) {
			return header;
		}
		header++;
	}
	return NULL;
}

// TODO: add percentage decoding
int *Request_parse_form(Request *request)
{
	int count = atoi(Request_get_header(request, "Content-Length")->value);
	char *body = calloc(count, sizeof(char));
	read(request->conn, body, count);
	request->form = calloc(10, sizeof(FormItem));
	FormItem *item = request->form;
	int start = 0;
	for (int i = 0; i <= count; i++) {
		if (body[i] == '&' || i == count) {
			item->value = calloc(i-start, sizeof(char));
			memcpy(item->value, body+start, i-start);
			start = i + 1;
			item++;
			request->formNum++;
		} else if (body[i] == '=') {
			item->name = calloc(i-start, sizeof(char));
			memcpy(item->name, body+start, i-start);
			start = i + 1;
		}
	}
	return 0;
}

int Response_write(Response *response, char *data)
{
	if (response->statusCode == 0) {
		response->statusCode = STATUS_OK;
	}

	// Status-line: "HTTP/1.1 302 Found"
	write(response->conn, response->version, strlen(response->version));
	char code[3] = {0};
	sprintf(code, " %d ", response->statusCode);
	write(response->conn, code, 5);
	char *codeStr = status_to_string(response->statusCode);
	write(response->conn, codeStr, strlen(codeStr));
	write(response->conn, "\r\n", 2);

	Header *header = response->headers;
	for (int i = 0; i < response->headerNum; i++) {
		write(response->conn, header->name, strlen(header->name));
		write(response->conn, ": ", 2);
		write(response->conn, header->value, strlen(header->value));
		write(response->conn, "\r\n", 2);
		header++;
	}

	if (data == NULL) {
		return 0;
	}

	char length[256] = {0};
	// sprintf(length, "%d", strlen(data));
	write(response->conn, "Content-Length: ", 16);
	write(response->conn, length, strlen(length));
	write(response->conn, "\r\n", 2);

	write(response->conn, "\r\n", 2);
	write(response->conn, data, strlen(data));
	return 0;
}

char *status_to_string(int status)
{
	switch (status) {
	case STATUS_CONTINUE:
		return "Continue";
	case STATUS_SWITCHING_PROTOCOLS:
		return "Switching Protocols";
	case STATUS_OK:
		return "OK";
	case STATUS_CREATED:
		return "Created";
	case STATUS_ACCEPTED:
		return "Accepted";
	case STATUS_NON_AUTHORITATIVE_INFORMATION:
		return "Non-Authoritative Information";
	case STATUS_NO_CONTENT:
		return "No Content";
	case STATUS_RESET_CONTENT:
		return "Reset Content";
	case STATUS_PARTIAL_CONTENT:
		return "Partial Content";
	case STATUS_MULTIPLE_CHOICES:
		return "Multiple Choices";
	case STATUS_MOVED_PERMANENTLY:
		return "Moved Permanently";
	case STATUS_FOUND:
		return "Found";
	case STATUS_SEE_OTHER:
		return "See Other";
	case STATUS_NOT_MODIFIED:
		return "Not Modified";
	case STATUS_USE_PROXY:
		return "Use Proxy";
	case STATUS_TEMPORARY_REDIRECT:
		return "Temporary Redirect";
	case STATUS_BAD_REQUEST:
		return "Bad Request";
	case STATUS_UNAUTHORIZED:
		return "Unauthorized";
	case STATUS_PAYMENT_REQUIRED:
		return "Payment Required";
	case STATUS_FORBIDDEN:
		return "Forbidden";
	case STATUS_NOT_FOUND:
		return "Not Found";
	case STATUS_METHOD_NOT_ALLOWED:
		return "Method Not Allowed";
	case STATUS_NOT_ACCEPTABLE:
		return "Not Acceptable";
	case STATUS_PROXY_AUTHENTICATION_REQUIRED:
		return "Proxy Authentication Required";
	case STATUS_REQUEST_TIME_OUT:
		return "Request Time-out";
	case STATUS_CONFLICT:
		return "Conflict";
	case STATUS_GONE:
		return "Gone";
	case STATUS_LENGTH_REQUIRED:
		return "Length Required";
	case STATUS_PRECONDITION_FAILED:
		return "Precondition Failed";
	case STATUS_REQUEST_ENTITY_TOO_LARGE:
		return "Request Entity Too Large";
	case STATUS_REQUEST_URI_TOO_LARGE:
		return "Request-URI Too Large";
	case STATUS_UNSUPPORTED_MEDIA_TYPE:
		return "Unsupported Media Type";
	case STATUS_REQUESTED_RANGE_NOT_SATISFIABLE:
		return "Requested range not satisfiable";
	case STATUS_EXPECTATION_FAILED:
		return "Expectation Failed";
	case STATUS_INTERNAL_SERVER_ERROR:
		return "Internal Server Error";
	case STATUS_NOT_IMPLEMENTED:
		return "Not Implemented";
	case STATUS_BAD_GATEWAY:
		return "Bad Gateway";
	case STATUS_SERVICE_UNAVAILABLE:
		return "Service Unavailable";
	case STATUS_GATEWAY_TIME_OUT:
		return "Gateway Time-out";
	case STATUS_HTTP_VERSION_NOT_SUPPORTED:
		return "HTTP Version not supported";
	}

	return "";
}

int Headers_free(Header *headers, int num)
{
	if (headers == NULL) {
		return 0;
	}

	for (int i = 0; i < num; ++i) {
		free(headers[i].name);
		free(headers[i].value);
	}

	free(headers);
	return 0;
}

int Request_free(Request *request)
{
	Headers_free(request->headers, request->headerNum);
	Form_free(request->form, request->formNum);

	free(request->version);
	free(request->url);
	free(request->method);
	return 0;
}

int Response_free(Response *__attribute__((unused))response)
{
	// Headers_free(response->headers, response->headerNum);
	return 0;
}

int Form_free(FormItem *form, int num)
{
	if (form == NULL) {
		return 0;
	}

	for (int i = 0; i < num; i++) {
		free(form[i].name);
		free(form[i].value);
	}
	free(form);
	return 0;
}
