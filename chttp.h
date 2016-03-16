#ifndef _chttp_h
#define _chttp_h

#include "request.h"
#include "trie.h"

typedef struct Server {
	int port;
	TrieNode *routes;
} Server;

Server *Server_new(int port);
int Server_handle(Server *server, char *url, handler *handler);
int Server_run(Server *server);

#endif
