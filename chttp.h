#ifndef _chttp_h
#define _chttp_h

#include <openssl/ssl.h>

#include "request.h"
#include "trie.h"

// +-----------------------------------------------+
// |                 Length (24)                   |
// +---------------+---------------+---------------+
// |   Type (8)    |   Flags (8)   |
// +-+-------------+---------------+-------------------------------+
// |R|                 Stream Identifier (31)                      |
// +=+=============================================================+
// |                   Frame Payload (0...)                      ...
// +---------------------------------------------------------------+

typedef struct Frame {
	int len;
	int flags;
	char type;
	int id;
	char* payload;
	// int payloadSize;
} Frame;

Frame *Frame_new(int type);
void Frame_free(Frame *frame);
void Frame_encode_header(Frame *frame, char *result);
void Frame_decode_header(Frame *frame, char *data);
void Frame_dump(Frame *frame);

typedef struct Server {
	int port;
	TrieNode *routes;
	int id;
} Server;

Server *Server_new(int port);
int Server_handle(Server *server, char *url, handler handler);
int Server_run(Server *server);

int Server_handle_http1_conn(Server *server, int childfd);
int Server_handle_http2_conn(Server *server, SSL *cSSL);

int Server_next_id(Server *server);

#define FRAME_HEADER_SIZE 9

#define FT_HEADERS       0x1
#define FT_PRIORITY      0x2
#define FT_RST_STREAM    0x3
#define FT_SETTINGS      0x4
#define FT_PUSH_PROMISE  0x5
#define FT_PING          0x6
#define FT_GOAWAY        0x7
#define FT_WINDOW_UPDATE 0x8
#define FT_CONTINUATION  0x9

#define FLAG_SETTTINGS_ACK 0x1

#define FLAGS_HEADERS_END_STREAM  0x1 //      0
#define FLAGS_HEADERS_END_HEADERS 0x4 //    100
#define FLAGS_HEADERS_PADDED      0x8 //   1000
#define FLAGS_HEADERS_PRIORITY    0x20 // 10100

#endif
