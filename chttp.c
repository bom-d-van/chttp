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
#include <darray.h>

#include "request.h"
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
	return server;
}

int Server_handle(Server *server, char *url, handler *handler)
{
	TrieNode_insert(server->routes, url, handler);
	return 0;
}

int Server_handle_conn(Server *server, int childfd)
{
	Request request = {.conn = childfd};
	Request_parse_request_line(&request);
	Request_parse_headers(&request);
	printf("[%s] %s\n", request.method, request.url);
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

int Server_run(Server *server) {
	int parentfd; /* parent socket */
	int childfd; /* child socket */
	// int portno; /* port to listen on */
	int clientlen; /* byte size of client's address */
	struct sockaddr_in serveraddr; /* server's addr */
	struct sockaddr_in clientaddr; /* client addr */
	struct hostent *hostp; /* client host info */
	// char buf[BUFSIZE]; /* message buffer */
	char *hostaddrp; /* dotted decimal host addr string */
	int optval; /* flag value for setsockopt */
	int n; /* message byte size */

	// /*
	// * check command line arguments
	// */
	// if (argc != 2) {
	// 	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	// 	exit(1);
	// }
	// portno = atoi(argv[1]);

	/*
	* socket: create the parent socket
	*/
	parentfd = socket(AF_INET, SOCK_STREAM, 0);
	if (parentfd < 0) {
		error("ERROR opening socket");
	}

	/* setsockopt: Handy debugging trick that lets
	* us rerun the server immediately after we kill it;
	* otherwise we have to wait about 20 secs.
	* Eliminates "ERROR on binding: Address already in use" error.
	*/
	optval = 1;
	setsockopt(parentfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

	/*
	* build the server's Internet address
	*/
	bzero((char *) &serveraddr, sizeof(serveraddr));

	/* this is an Internet address */
	serveraddr.sin_family = AF_INET;

	/* let the system figure out our IP address */
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

	/* this is the port we will listen on */
	serveraddr.sin_port = htons((unsigned short)server->port);

	/*
	* bind: associate the parent socket with a port
	*/
	if (bind(parentfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) {
		error("ERROR on binding");
	}

	/*
	* listen: make this socket ready to accept connection requests
	*/
	/* allow 5 requests to queue up */
	if (listen(parentfd, 5) < 0) {
		error("ERROR on listen");
	}

	/*
	* main loop: wait for a connection request, echo input line,
	* then close connection.
	*/
	clientlen = sizeof(clientaddr);
	while (1) {
		/*
		 * accept: wait for a connection request
		 */
		childfd = accept(parentfd, (struct sockaddr *) &clientaddr, &clientlen);
		if (childfd < 0) {
			error("ERROR on accept");
		}

		/*
		 * gethostbyaddr: determine who sent the message
		 */
		hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, sizeof(clientaddr.sin_addr.s_addr), AF_INET);
		if (hostp == NULL) {
			error("ERROR on gethostbyaddr");
		}
		hostaddrp = inet_ntoa(clientaddr.sin_addr);
		if (hostaddrp == NULL) {
			error("ERROR on inet_ntoa\n");
		}

		Server_handle_conn(server, childfd);
	}
}
