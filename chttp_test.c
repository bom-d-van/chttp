#include <unistd.h>
#include <stdio.h>
#include "chttp.h"

typedef struct Info {
	char action[1024];
	char priority[1024];
} Info;

Info info = { .action = {0}, .priority = {0} };

int handleHome(Request *__attribute__((unused))request, Response *response)
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

int main(void)
{
	int port = 443;
	printf("Listening :%d\n", port);
	Server *server = Server_new(port);
	Server_handle(server, "/", handleHome);
	Server_handle(server, "/post", handlePost);
	Server_run(server);
	return 0;
}
