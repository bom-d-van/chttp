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
	int flags;
	char type;
	int id;

	int len;
	char* payload;
} Frame;

Frame *Frame_new(int type);
void Frame_free(Frame *frame);
void Frame_encode_header(Frame *frame, char *result);
void Frame_decode_header(Frame *frame, char *data);
void Frame_dump(Frame *frame);

// +---------------+
// |Pad Length? (8)|
// +-+-------------+-----------------------------------------------+
// |E|                 Stream Dependency? (31)                     |
// +-+-------------+-----------------------------------------------+
// |  Weight? (8)  |
// +-+-------------+-----------------------------------------------+
// |                   Header Block Fragment (*)                 ...
// +---------------------------------------------------------------+
// |                           Padding (*)                       ...
// +---------------------------------------------------------------+
struct HeadersFrame {
	int e;
	int streamDependency;
	int weight;

	char *headerBlock;
	int headerBlockLen;

	int padLength;
	char *padding;
	// struct hpack_header_field *headers;
};

struct HeadersFrame *HeadesFrame_decode(Frame *frame);
void HeadesFrame_encode(Frame *frame, struct HeadersFrame *hf);
int HeadesFrame_free(struct HeadersFrame *hf);

// +---------------+
// |Pad Length? (8)|
// +---------------+-----------------------------------------------+
// |                            Data (*)                         ...
// +---------------------------------------------------------------+
// |                           Padding (*)                       ...
// +---------------------------------------------------------------+
struct DataFrame {
	int padLength;
	char *padding;

	char *data;
	int dataLen;
};

void DataFrame_encode(Frame *frame, struct DataFrame *hf);
struct DataFrame *DataFrame_decode(Frame *frame);
int DataFrame_free(struct DataFrame *hf);

// +-------------------------------+
// |       Identifier (16)         |
// +-------------------------------+-------------------------------+
// |                        Value (32)                             |
// +---------------------------------------------------------------+
struct SettingFrame {
	long SETTINGS_HEADER_TABLE_SIZE; // 0x1
	long SETTINGS_ENABLE_PUSH; // 0x2
	long SETTINGS_MAX_CONCURRENT_STREAMS; // 0x3
	long SETTINGS_INITIAL_WINDOW_SIZE; // 0x4
	long SETTINGS_MAX_FRAME_SIZE; // 0x5
	long SETTINGS_MAX_HEADER_LIST_SIZE; // 0x6
};

struct SettingFrame SettingFrame_decode(Frame *frame);

// +-+-------------------------------------------------------------+
// |R|                  Last-Stream-ID (31)                        |
// +-+-------------------------------------------------------------+
// |                      Error Code (32)                          |
// +---------------------------------------------------------------+
// |                  Additional Debug Data (*)                    |
// +---------------------------------------------------------------+
struct GoAwayFrame {
	int r;
	int lastStreamID;
	int errorCode;
	char* debugData;
};

struct GoAwayFrame *GoAwayFrame_decode(Frame *frame);
void GoAwayFrame_free(struct GoAwayFrame *gaframe);
void GoAwayFrame_dump(struct GoAwayFrame *gaframe);

typedef struct Server {
	int port;
	TrieNode *routes;
	int id;
} Server;

Server *Server_new(int port);
int Server_handle(Server *server, char *url, handler handler, handler2 handler2);
int Server_run(Server *server);

int Server_handle_http1_conn(Server *server, int childfd);
int Server_handle_http2_conn(Server *server, SSL *cSSL);

int Server_next_id(Server *server);

#define FRAME_HEADER_SIZE 9

#define FT_DATA          0x0
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

#define FLAGS_HEADERS_END_STREAM  0x1  //     1
#define FLAGS_HEADERS_END_HEADERS 0x4  //   100
#define FLAGS_HEADERS_PADDED      0x8  //  1000
#define FLAGS_HEADERS_PRIORITY    0x20 // 10100

#define FLAGS_DATA_END_STREAM  0x1
#define FLAGS_DATA_PADDED      0x8

#define FLAGS_SETTING_ACK  0x1

#endif
