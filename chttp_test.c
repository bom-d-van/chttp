#include <unistd.h>
#include <stdio.h>
#include "chttp.h"
#include "hpack/hpack.h"

typedef struct Info {
	char action[1024];
	char priority[1024];
} Info;

Info info = { .action = {0}, .priority = {0} };

#define _U_ __attribute__((unused))

int handleHome(Request * _U_ request, Response *response)
{
	Header headers[5];
	response->headers = headers;
	response->headers[0].name = "Content-Type";
	response->headers[0].value = "text/html; charset=utf-8";
	response->headerNum++;
	char test[10] = {0};
	char *template = "<!DOCTYPE html>\n"
				 "<html>\n"
				 "<head>\n"
				 "	<title>CHTTP</title>\n"
				 "</head>\n"
				 "<body>\n"
				 "	<h1>Test</h1>\n"
				 "	<form action=\"/post\" method=\"POST\">\n"
				 "		<label>Action: <input name=\"action\" value=\"%s\"></label><br>\n"
				 "		<label>Priority: <input name=\"priority\" value=\"%s\"></label><br>\n"
				 "		<button>Submit</button>"
				 "	</form>\n"
				 "</body>\n"
				 "</html>\n";
	char page[2046] = {0};
	// printf("info.action = %s; info.priority = %s\n", info.action, info.priority);
	sprintf(page, template, info.action, info.priority);
	Response_write(response, page);
	return 0;
}

int handleHome2(struct Request2 *__attribute__((unused))request, struct Response2 *__attribute__((unused))response)
{
	response->header = (struct hpack_header_field *)calloc(5, sizeof(struct hpack_header_field));
	response->headerLen = 0;

	response->header[response->headerLen].name = ":status";
	response->header[response->headerLen].value = "200";
	response->headerLen++;

	response->header[response->headerLen].name = "content-type";
	response->header[response->headerLen].value = "text/html; charset=utf-8";
	response->headerLen++;
	char test[10] = {0};
	char *template = "<!DOCTYPE html>\n"
				 "<html>\n"
				 "<head>\n"
				 "	<title>CHTTP</title>\n"
				 "</head>\n"
				 "<body>\n"
				 "	<h1>Test</h1>\n"
				 "	<form action=\"/post\" method=\"POST\">\n"
				 "		<label>Action: <input name=\"action\" value=\"%s\"></label><br>\n"
				 "		<label>Priority: <input name=\"priority\" value=\"%s\"></label><br>\n"
				 "		<button>Submit</button>"
				 "	</form>\n"
				 "</body>\n"
				 "</html>\n";
	char *page = calloc(2046, sizeof(char));
	// printf("info.action = %s; info.priority = %s\n", info.action, info.priority);
	sprintf(page, template, info.action, info.priority);
	// Response_write(response, page);

	char *length = calloc(256, sizeof(char));
	sprintf(length, "%lu", strlen(page));
	response->header[response->headerLen].name = "content-length";
	response->header[response->headerLen].value = length;
	response->headerLen++;

	response->body = page;
	response->bodyLen = strlen(page);
	return 0;
}

int handlePost(Request *request, Response *response)
{
	if (!strcmp(request->method, "POST")) {
		Request_parse_form(request);
		for (int i = 0; i < request->formNum; i++) {
			// printf("%s=%s\n", request->form[i].name, request->form[i].value);
			if (!strcmp(request->form[i].name, "action")) {
				memcpy(info.action, request->form[i].value, strlen(request->form[i].value));
				// printf("action = %s\n", info.action);
			} else if (!strcmp(request->form[i].name, "priority")) {
				memcpy(info.priority, request->form[i].value, strlen(request->form[i].value));
				// printf("priority = %s\n", info.priority);
			}
		}
	}
	// printf("info.action = %s; info.priority = %s\n", info.action, info.priority);

	Header headers[5];
	response->headers = headers;
	response->headers[0].name = "Location";
	response->headers[0].value = "/";
	response->headerNum++;
	response->statusCode = STATUS_SEE_OTHER;
	Response_write(response, NULL);
	return 0;
}

int handlePost2(struct Request2 *request, struct Response2 *response)
{
	struct hpack_header_field *method = hpack_get_header(request->header, request->headerLen, ":method");
	if (method && !strcmp(method->value, "POST")) {
		int formNum = 0;
		FormItem *form = parseURLEncodedForm(request->body, request->bodyLen, &formNum);

		for (int i = 0; i < formNum; i++) {
			// printf("%s=%s\n", form[i].name, form[i].value);
			if (!strcmp(form[i].name, "action")) {
				memcpy(info.action, form[i].value, strlen(form[i].value));
				// printf("action = %s\n", info.action);
			} else if (!strcmp(form[i].name, "priority")) {
				memcpy(info.priority, form[i].value, strlen(form[i].value));
				// printf("priority = %s\n", info.priority);
			}
		}
	}
	// printf("info.action = %s; info.priority = %s\n", info.action, info.priority);

	response->header = (struct hpack_header_field *)calloc(5, sizeof(struct hpack_header_field));
	response->headerLen = 0;

	response->header[response->headerLen].name = ":status";
	response->header[response->headerLen].value = "303";
	response->headerLen++;

	response->header[response->headerLen].name = "location";
	response->header[response->headerLen].value = "/";
	response->headerLen++;

	return 0;
}

int main(void)
{
	signal(SIGPIPE, SIG_IGN);

	int port = 8443;
	printf("Listening :%d\n", port);
	Server *server = Server_new(port);
	init_huffman_tree();

	Server_handle(server, "/", handleHome, handleHome2);
	Server_handle(server, "/post", handlePost, handlePost2);

	Server_run(server);
	return 0;
}
