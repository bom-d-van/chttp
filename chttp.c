/*
 * tcpserver.c - A simple TCP echo server
 * usage: tcpserver <port>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// #include <openssl/applink.c>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "request.h"
#include <util.h>
#include "chttp.h"

#define BUFSIZE 1024

/*
 * error - wrapper for perror
 */
void error(char *msg) {
	perror(msg);
	exit(1);
}

Server *Server_new(int port)
{
	Server *server = calloc(1, sizeof(Server));
	server->port = port;
	server->routes = TrieNode_new();
	server->id = 0;
	return server;
}

int Server_handle(Server *server, char *url, handler handler)
{
	TrieNode_insert(server->routes, url, handler);
	return 0;
}

int Server_handle_http1_conn(Server *server, int childfd)
{
	Request request = {.conn = childfd};
	int result = Request_parse_request_line(&request);
	if (result == 2) {
		printf("http2 is coming\n");
	} else if (result == 1) {
		return 1;
	}

	Request_parse_headers(&request);
	// printf("[%s] %s\n", request.method, request.url);
	// printf("%d\n", request.headerNum);
	// for (int i = 0; i < request.headerNum; i++) {
	// 	printf("%s=%s\n", request.headers[i].name, request.headers[i].value);
	// }

	TrieNode *node = TrieNode_find(server->routes, request.url);
	Response response = {.conn = childfd, .version = request.version};
	if (node == NULL) {
		// Status-line: "HTTP/1.1 302 Found"
		response.statusCode = STATUS_NOT_FOUND;
		Response_write(&response, NULL);
	} else {
		node->handler(&request, &response);
	}

	Request_free(&request);
	Response_free(&response);
	close(childfd);

	return 0;
}

int alpn_select_cb(SSL *__attribute__((unused))s, const unsigned char **out, unsigned char *outlen, const unsigned char *__attribute__((unused))in, unsigned int __attribute__((unused))inlen, void *__attribute__((unused))arg)
{
	*out = (const unsigned char *)"h2";
	*outlen = 2;
	return 0;
}

int Server_next_id(Server *server)
{
	server->id += 2;
	return server->id - 2;
}

int Server_run(Server *server)
{
	// socket: create the parent socket
	int parentfd = socket(AF_INET, SOCK_STREAM, 0);
	if (parentfd < 0) {
		error("ERROR opening socket");
	}

	/* setsockopt: Handy debugging trick that lets
	* us rerun the server immediately after we kill it;
	* otherwise we have to wait about 20 secs.
	* Eliminates "ERROR on binding: Address already in use" error.
	*/
	int optval = 1;
	setsockopt(parentfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

	struct sockaddr_in serveraddr; /* server's addr */
	bzero((char *) &serveraddr, sizeof(serveraddr)); // build the server's Internet address

	serveraddr.sin_family = AF_INET; /* this is an Internet address */
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); /* let the system figure out our IP address */
	serveraddr.sin_port = htons((unsigned short)server->port); /* this is the port we will listen on */

	// bind: associate the parent socket with a port
	if (bind(parentfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) {
		error("ERROR on binding");
	}

	SSL_library_init();

	SSL_CTX *sslctx = SSL_CTX_new(SSLv23_server_method());
	sslctx->alpn_select_cb = alpn_select_cb;
	SSL_CTX_set_options(sslctx, SSL_OP_SINGLE_DH_USE);
	int use_cert = SSL_CTX_use_certificate_file(sslctx, "conf/server.pem" , SSL_FILETYPE_PEM);
	int use_prv = SSL_CTX_use_PrivateKey_file(sslctx, "conf/server.pem", SSL_FILETYPE_PEM);

	// listen: make this socket ready to accept connection requests
	// allow 5 requests to queue up
	if (listen(parentfd, 5) < 0) {
		error("ERROR on listen");
	}

	struct sockaddr_in clientaddr; /* client addr */
	unsigned int clientlen = sizeof(clientaddr);
	while (1) {
		int childfd = accept(parentfd, (struct sockaddr *) &clientaddr, &clientlen);
		if (childfd < 0) {
			error("ERROR on accept");
		}

		SSL *cSSL = SSL_new(sslctx);
		SSL_set_fd(cSSL, childfd);
		// Here is the SSL Accept portion.  Now all reads and writes must use SSL
		int ssl_err = SSL_accept(cSSL);
		if (ssl_err != 1) {
			printf("SSL_accept: %d\n", ssl_err);
		}

		Server_handle_http2_conn(server, cSSL);
		// Server_handle_http1_conn(server, childfd);
	}
}

int readSize(SSL *cSSL, char *buf, int len)
{
	int read = 0;
	while (read < len) {
		// printf("reading %d...\n", read);
		int v = SSL_read(cSSL, &buf[read], len-read);
		if (v <= 0) {
			return v;
		}
		read += v;
	}

	return read;
}

Frame *get_next_frame(SSL *cSSL)
{
	Frame *frame = Frame_new(0);

	char buf[10] = {0};
	readSize(cSSL, buf, 9);
	Frame_decode_header(frame, buf);

	if (frame->len == 0) {
		return frame;
	}

	frame->payload = calloc(frame->len, sizeof(char));
	readSize(cSSL, frame->payload, frame->len);

	return frame;
}

int Server_handle_http2_conn(Server *__attribute__((unused))server, SSL *cSSL)
{
	Frame *frame = Frame_new(FT_SETTINGS);
	frame->id = Server_next_id(server);
	char header[9] = {0};
	Frame_encode_header(frame, header);

	SSL_write(cSSL, header, 9);

	char preface[25] = {0};
	SSL_read(cSSL, preface, 24);

	// read preface frame
	// char buf[9] = {0};
	// int len = readSize(cSSL, buf, 9);
	// if (len <= 0) {
	// 	printf("readSize: %d\n", len);
	// }

	while (1) {
		Frame *frame = get_next_frame(cSSL);
		Frame_dump(frame);
	}

	// printf("1:"); print_hex(buf, 9);

	return 0;
}
