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

#include <err.h>

#include "request.h"
#include "util.h"
#include "chttp.h"
#include "hpack/hpack.h"

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

int Server_handle(Server *server, char *url, handler handler, handler2 handler2)
{
	TrieNode_insert(server->routes, url, handler, handler2);
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
	*out = (const unsigned char *)"h2-14";
	*outlen = 2;
	return 0;
}

int Server_next_id(Server *server)
{
	server->id += 2;
	return server->id - 2;
}

static unsigned char next_proto_list[256];
static size_t next_proto_list_len;

static int next_proto_cb(SSL *__attribute__((unused))s, const unsigned char **data, unsigned int *len, void *__attribute__((unused))arg) {
	*data = next_proto_list;
	*len = (unsigned int)next_proto_list_len;
	return SSL_TLSEXT_ERR_OK;
}

static SSL_CTX *create_ssl_ctx(const char *key_file, const char *cert_file) {
	SSL_CTX *ssl_ctx;
	EC_KEY *ecdh;

	ssl_ctx = SSL_CTX_new(SSLv23_server_method());
	if (!ssl_ctx) {
		errx(1, "Could not create SSL/TLS context: %s", ERR_error_string(ERR_get_error(), NULL));
	}
	SSL_CTX_set_options(ssl_ctx,
	            		SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 |
	                	SSL_OP_NO_COMPRESSION |
	                	SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION);

	ecdh = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
	if (!ecdh) {
		errx(1, "EC_KEY_new_by_curv_name failed: %s",
		ERR_error_string(ERR_get_error(), NULL));
	}
	SSL_CTX_set_tmp_ecdh(ssl_ctx, ecdh);
	EC_KEY_free(ecdh);

	if (SSL_CTX_use_PrivateKey_file(ssl_ctx, key_file, SSL_FILETYPE_PEM) != 1) {
		errx(1, "Could not read private key file %s", key_file);
	}
	if (SSL_CTX_use_certificate_chain_file(ssl_ctx, cert_file) != 1) {
		errx(1, "Could not read certificate file %s", cert_file);
	}

	next_proto_list[0] = 2;
	memcpy(&next_proto_list[1], "h2", 2);
	next_proto_list_len = 1 + 2;

	SSL_CTX_set_next_protos_advertised_cb(ssl_ctx, next_proto_cb, NULL);
	return ssl_ctx;
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

	// SSL_CTX *sslctx = SSL_CTX_new(SSLv23_server_method());
	// sslctx->alpn_select_cb = alpn_select_cb;
	// // SSL_CTX_set_options(sslctx, SSL_OP_SINGLE_DH_USE);
	// int use_cert = SSL_CTX_use_certificate_file(sslctx, "conf/server.crt" , SSL_FILETYPE_PEM);
	// int use_prv = SSL_CTX_use_PrivateKey_file(sslctx, "conf/server.key", SSL_FILETYPE_PEM);

	// auto int ssl_opts = (SSL_OP_ALL & ~SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS) |
	// 					SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION |
	// 					SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION |
	// 					SSL_OP_SINGLE_ECDH_USE | SSL_OP_NO_TICKET |
	// 					SSL_OP_CIPHER_SERVER_PREFERENCE;
	// SSL_CTX_set_options(sslctx, ssl_opts);
	// SSL_CTX_set_mode(sslctx, SSL_MODE_AUTO_RETRY);
	// SSL_CTX_set_mode(sslctx, SSL_MODE_RELEASE_BUFFERS);
	// // SSL_CTX_set_cipher_list(sslctx, SSL_DEFAULT_CIPHER_LIST);

	SSL_CTX *sslctx = create_ssl_ctx("conf/server.key", "conf/server.crt");

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
		close(childfd);
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
	int size = readSize(cSSL, buf, 9);
	if (size <= 0) {
		printf("readSize error: %d\n", size);
		return NULL;
	}

	Frame_decode_header(frame, buf);

	if (frame->len == 0) {
		return frame;
	}

	frame->payload = calloc(frame->len, sizeof(char));
	readSize(cSSL, frame->payload, frame->len);

	return frame;
}

struct http2Conn {
	SSL *cSSL;
	struct hpack *decoder;
	struct hpack *encoder;
	struct SettingFrame settings;
	int connWindowSize;
};

int Server_handle_http2_conn(Server *server, SSL *cSSL)
{
	// The server connection preface
	Frame *frame = Frame_new(FT_SETTINGS);
	frame->id = Server_next_id(server);
	char header[9] = {0};
	Frame_encode_header(frame, header);
	SSL_write(cSSL, header, 9);

	// The client connection preface
	char preface[25] = {0};
	SSL_read(cSSL, preface, 24);
	// TODO: check preface

	// read preface frame
	// char buf[9] = {0};
	// int len = readSize(cSSL, buf, 9);
	// if (len <= 0) {
	// 	printf("readSize: %d\n", len);
	// }

	// struct hpack *decoder = hpack_new();
	// struct hpack *encoder = hpack_new();
	// struct SettingFrame settings;
	// int connWindowSize = (1 << 16) - 1; // TODO

	struct http2Conn conn = {
		.cSSL           = cSSL,
		.decoder        = hpack_new(),
		.encoder        = hpack_new(),
		.connWindowSize = (1 << 16) - 1, // TODO
	};

	struct http2Request {
		struct Request2 *req;
		int requestReady;
	};

	struct http2Request newRequest = {
		.requestReady = 0,
	};

	// int requestComplete = 0;
	while (1) {
		Frame *frame = get_next_frame(conn.cSSL);
		if (!frame) {
			printf("PROTOCOL_ERROR\n");
			return 1;
		}

		Frame_dump(frame); // DEBUG


		switch (frame->type) {
		case FT_HEADERS:
			{};
			struct HeadersFrame *hf = (struct HeadersFrame *)HeadesFrame_decode(frame);
			int len = 0;
			// print_hex(hf->headerBlock, strlen(hf->headerBlock));
			struct hpack_header_field *header = hpack_decode(conn.decoder, hf->headerBlock, hf->headerBlockLen, &len);
			// printf("request headers\n");
			printf("----\n"); hpack_header_fields_dump(header, len); printf("----\n");

			struct Request2 *req = calloc(1, sizeof(struct Request2));
			req->header = header;
			req->headerLen = len;
			req->frameID = frame->id;

			newRequest.req = req;

			struct hpack_header_field *cntLen = hpack_get_header(header, len, "content-length");
			if (!cntLen) {
				newRequest.requestReady = 1;
			}

			// void Server_handle_http2_request(Server *server, struct http2Conn *conn, struct Request2 *req);
			// Server_handle_http2_request(server, &conn, &req);

			// TODO
			HeadesFrame_free(hf);
			break;
		case FT_CONTINUATION:
			// TODO
			break;
		case FT_DATA:
			{};
			struct DataFrame *df = DataFrame_decode(frame);
			printf("---------------\n");
			printf("%d\n", newRequest.req->headerLen);
			newRequest.req->body = df->data;
			newRequest.req->bodyLen = df->dataLen;

			if (frame->flags & FLAGS_DATA_END_STREAM) {
				newRequest.requestReady = 1;
			}

			// TODO: free df
			break;
		case FT_PRIORITY:
			{
				Frame settingsResp = {
					.flags = FLAGS_SETTING_ACK,
					.type = FT_SETTINGS,
					.id = frame->id,
					.len = 0,
				};
				char frameHeader[10] = {0};
				Frame_encode_header(&settingsResp, frameHeader);
				printf("wrote: "); Frame_dump(&settingsResp);
				SSL_write(conn.cSSL, frameHeader, 9);
			}
			break;
		case FT_RST_STREAM:
			break;
		case FT_SETTINGS:
			conn.settings = SettingFrame_decode(frame);
			conn.connWindowSize = conn.settings.SETTINGS_INITIAL_WINDOW_SIZE;
			{
				Frame settingsResp = {
					.flags = FLAGS_SETTING_ACK,
					.type = FT_SETTINGS,
					.id = frame->id,
					.len = 0,
				};
				char frameHeader[10] = {0};
				Frame_encode_header(&settingsResp, frameHeader);
				printf("wrote: "); Frame_dump(&settingsResp);
				SSL_write(conn.cSSL, frameHeader, 9);
			}

			break;
		case FT_PUSH_PROMISE:
			break;
		case FT_PING:
			break;
		case FT_GOAWAY:
			{};
			struct GoAwayFrame *gaframe = GoAwayFrame_decode(frame);
			GoAwayFrame_dump(gaframe);
			GoAwayFrame_free(gaframe);
			return 1;
			break;
		case FT_WINDOW_UPDATE:
			if (frame->len != 4) {
				// TODO
				printf("PROTOCOL_ERROR\n");
				return 1;
			}

			int size = 0;
			size |= frame->payload[0] << 24;
			size |= frame->payload[1] << 16;
			size |= frame->payload[2] << 8;
			size |= frame->payload[3];

			conn.connWindowSize += size;

			{
				Frame settingsResp = {
					.flags = FLAGS_SETTING_ACK,
					.type = FT_SETTINGS,
					.id = frame->id,
					.len = 0,
				};
				char frameHeader[10] = {0};
				Frame_encode_header(&settingsResp, frameHeader);
				printf("wrote: "); Frame_dump(&settingsResp);
				SSL_write(conn.cSSL, frameHeader, 9);
			}
			break;
		default:
			// TODO: send GOAWAY frame
			printf("PROTOCOL_ERROR\n");
			return 1;
			break;
		}

		if (!newRequest.requestReady) {
			continue;
		}

		void Server_handle_http2_request(Server *server, struct http2Conn *conn, struct Request2 *req);
		Server_handle_http2_request(server, &conn, newRequest.req);

		// TODO: free newRequest
		newRequest = (struct http2Request){};
	}

	// printf("1:"); print_hex(buf, 9);

	return 0;
}

void Server_handle_http2_request(Server *server, struct http2Conn *conn, struct Request2 *req)
{
	// struct hapck_header_field *field = hpack_get_header(req->header, ":path");
	// printf("%s\n", );
	TrieNode *node = TrieNode_find(server->routes, hpack_get_header(req->header, req->headerLen, ":path")->value);
	struct Response2 *resp = calloc(1, sizeof(struct Response2));
	resp->frameID = req->frameID;
	if (node == NULL) {
		// Status-line: "HTTP/1.1 302 Found"
		// resp.statusCode = STATUS_NOT_FOUND;
		// Response_write(resp, NULL);
		resp->header = (struct hpack_header_field *)calloc(5, sizeof(struct hpack_header_field));
		resp->headerLen = 0;

		resp->header[resp->headerLen].name = ":status";
		resp->header[resp->headerLen].value = "404";
		resp->headerLen++;

		resp->bodyLen = 0;
	} else {
		node->handler2(req, resp);
	}

	void writeResponse(struct http2Conn *conn, struct Response2 *resp);
	writeResponse(conn, resp);

	// TODO
	// Request_free(req);

	// TODO: free resp (headers, etc)

	// return resp;
}

struct HeadersFrame *HeadesFrame_decode(Frame *frame)
{
	struct HeadersFrame *hf = calloc(1, sizeof(struct HeadersFrame));
	hf->padLength = 0;
	hf->streamDependency = 0;
	hf->e = 0;

	int offset = 0;
	if (frame->flags & FLAGS_HEADERS_PADDED) {
		hf->padLength = frame->payload[0];
		offset++;
	}

	if (frame->flags & FLAGS_HEADERS_PRIORITY) {
		offset += 5;

		hf->e = frame->payload[1] >> 7;

		hf->streamDependency |= (frame->payload[1] & 127) << 24;
		hf->streamDependency |= frame->payload[2] << 16;
		hf->streamDependency |= frame->payload[3] << 8;
		hf->streamDependency |= frame->payload[4];

		hf->weight = frame->payload[5];
	}

	// printf("offset = %d\n", offset);
	int len = frame->len - offset - hf->padLength;
	hf->headerBlock = (char *)calloc(len + 1, sizeof(char));
	cpystr(hf->headerBlock, &(frame->payload[offset]), len);
	hf->headerBlockLen = len;

	printf("len = %d\n", len);
	print_hex(hf->headerBlock, len);

	if (hf->padLength > 0) {
		hf->padding = (char *)calloc(hf->padLength+1, sizeof(char));
		cpystr(hf->padding, &(frame->payload[len + offset]), hf->padLength);
	}
	return hf;
}

void HeadesFrame_encode(Frame *frame, struct HeadersFrame *hf)
{
	// printf("hf->headerBlockLen = %d\n", hf->headerBlockLen);
	frame->len = hf->headerBlockLen;
	frame->payload = calloc(frame->len + 7, sizeof(char));

	int index = 0;
	if (frame->flags & FLAGS_HEADERS_PADDED) {
		frame->payload[0] = hf->padLength;
		index++;
		frame->len++;
	}

	if (frame->flags & FLAGS_HEADERS_PRIORITY) {
		frame->payload[1] = hf->e << 7;

		frame->payload[1] |= hf->streamDependency >> 24;
		frame->payload[2] |= hf->streamDependency >> 16;
		frame->payload[3] |= hf->streamDependency >> 8;
		frame->payload[4] |= hf->streamDependency;

		frame->payload[5] = hf->weight;

		index += 5;
		frame->len += 5;
	}

	// printf("hf->headerBlock: ");
	// print_hex(hf->headerBlock, hf->headerBlockLen);

	// strlcpy(&(frame->payload[index]), hf->headerBlock, hf->headerBlockLen + 1);
	for (int i = 0; i < hf->headerBlockLen; i++) {
		frame->payload[index+i] = hf->headerBlock[i];
	}
	index += hf->headerBlockLen;

	// printf("hf->headerBlock: ");
	// print_hex(&(frame->payload[index - hf->headerBlockLen]), hf->headerBlockLen);

	// print_hex(hf->headerBlock, len);

	if (hf->padLength > 0) {
		cpystr(&(frame->payload[index]), hf->padding, hf->padLength);
	}
}

// TODO
int HeadesFrame_free(struct HeadersFrame *hf)
{
	free(hf);
	return 0;
}

void DataFrame_encode(Frame *frame, struct DataFrame *df)
{
	frame->payload = calloc(df->padLength + df->dataLen + 1, sizeof(char));
	frame->len = 0;
	int index = 0;

	if (frame->flags & FLAGS_DATA_PADDED) {
		frame->len++;
		index += 1;
		frame->payload[0] = df->padLength;
	}

	cpystr(&(frame->payload[index]), df->data, df->dataLen);
	index += df->dataLen;
	frame->len += df->dataLen;

	if (frame->flags & FLAGS_DATA_PADDED) {
		frame->len += df->padLength;
		cpystr(&(frame->payload[index]), df->padding, df->padLength);
	}
}

struct DataFrame *DataFrame_decode(Frame *frame)
{
	struct DataFrame *df = calloc(1, sizeof(struct DataFrame));
	df->dataLen = frame->len;
	df->padLength = 0;

	int index = 0;
	if (frame->flags & FLAGS_DATA_PADDED) {
		index += 1;
		df->padLength = frame->payload[0];
		df->dataLen -= df->padLength;
	}

	df->data = calloc(df->dataLen + 1, sizeof(char));
	cpystr(df->data, &frame->payload[index], df->dataLen);
	index += df->dataLen;

	if (frame->flags & FLAGS_DATA_PADDED) {
		df->padding = calloc(df->padLength + 1, sizeof(char));
		cpystr(df->padding, &frame->payload[index], df->padLength);
	}

	return df;
}

// TODO
int DataFrame_free(struct DataFrame *hf) {
	free(hf);
	return 0;
}

struct SettingFrame SettingFrame_decode(Frame *frame)
{
	struct SettingFrame settings = {
		.SETTINGS_HEADER_TABLE_SIZE = 4096,
		.SETTINGS_ENABLE_PUSH = 1,
		.SETTINGS_MAX_CONCURRENT_STREAMS = 1024,
		.SETTINGS_INITIAL_WINDOW_SIZE = (1 << 16) - 1,
		.SETTINGS_MAX_FRAME_SIZE = 1 << 14,
		.SETTINGS_MAX_HEADER_LIST_SIZE = -1,
	};
	for (int i = 0; i < frame->len/6; i++) {
		int parameter = 0;
		int index = i * 6;
		parameter |= frame->payload[index] << 8;
		parameter |= frame->payload[++index];
		int value = 0;
		value |= frame->payload[++index] << 24;
		value |= frame->payload[++index] << 16;
		value |= frame->payload[++index] << 8;
		value |= frame->payload[++index];
		switch (parameter) {
		case 0x1:
			settings.SETTINGS_HEADER_TABLE_SIZE = value;
			break;
		case 0x2:
			settings.SETTINGS_ENABLE_PUSH = value;
			break;
		case 0x3:
			settings.SETTINGS_MAX_CONCURRENT_STREAMS = value;
			break;
		case 0x4:
			settings.SETTINGS_INITIAL_WINDOW_SIZE = value;
			break;
		case 0x5:
			settings.SETTINGS_MAX_FRAME_SIZE = value;
			break;
		case 0x6:
			settings.SETTINGS_MAX_HEADER_LIST_SIZE = value;
			break;
		}
	}
	return settings;
}

struct GoAwayFrame *GoAwayFrame_decode(Frame *frame)
{
	struct GoAwayFrame *gaframe = calloc(1, sizeof(struct GoAwayFrame));
	gaframe->r = frame->payload[0] >> 7;
	gaframe->lastStreamID = 0;
	gaframe->lastStreamID |= (frame->payload[0] & 127) << 24;
	gaframe->lastStreamID |= frame->payload[1] << 16;
	gaframe->lastStreamID |= frame->payload[2] << 8;
	gaframe->lastStreamID |= frame->payload[3];

	gaframe->errorCode = 0;
	gaframe->errorCode |= frame->payload[4] << 24;
	gaframe->errorCode |= frame->payload[5] << 16;
	gaframe->errorCode |= frame->payload[6] << 8;
	gaframe->errorCode |= frame->payload[7];

	gaframe->debugData = NULL;
	if (frame->len - 8 > 0) {
		gaframe->debugData = calloc(frame->len - 8 + 1, sizeof(char));
		cpystr(gaframe->debugData, &frame->payload[8], frame->len - 8);
	}

	return gaframe;
}

void GoAwayFrame_free(struct GoAwayFrame *gaframe)
{
	// TODO
	free(gaframe);
}

void GoAwayFrame_dump(struct GoAwayFrame *gaframe)
{
	char *data = "";
	switch (gaframe->errorCode) {
	case 0x0:
		data = "NO_ERROR";
		break;
	case 0x1:
		data = "PROTOCOL_ERROR";
		break;
	case 0x2:
		data = "INTERNAL_ERROR";
		break;
	case 0x3:
		data = "FLOW_CONTROL_ERROR";
		break;
	case 0x4:
		data = "SETTINGS_TIMEOUT";
		break;
	case 0x5:
		data = "STREAM_CLOSED";
		break;
	case 0x6:
		data = "FRAME_SIZE_ERROR";
		break;
	case 0x7:
		data = "REFUSED_STREAM";
		break;
	case 0x8:
		data = "CANCEL";
		break;
	case 0x9:
		data = "COMPRESSION_ERROR";
		break;
	case 0xa:
		data = "CONNECT_ERROR";
		break;
	case 0xb:
		data = "ENHANCE_YOUR_CALM";
		break;
	case 0xc:
		data = "INADEQUATE_SECURITY";
		break;
	case 0xd:
		data = "HTTP_1_1_REQUIRED";
		break;
	}
	printf("stream-id = %d; error-code = %s\n", gaframe->lastStreamID, data);
	if (gaframe->debugData) {
		printf("debugData = %s\n", gaframe->debugData);
	}
}

void writeResponse(struct http2Conn *conn, struct Response2 *resp)
{
	printf("\nresponse headers\n");
	hpack_header_fields_dump(resp->header, resp->headerLen);

	int dataLen = 0;
	char *data = hpack_encode(conn->encoder, resp->header, resp->headerLen, 1, &dataLen);
	struct HeadersFrame resphf = {
		.padLength = 0, .padding = NULL,
		.streamDependency = 0, .e = 0,
		.headerBlock = data, .headerBlockLen = dataLen,
	};

	int flag = FLAGS_HEADERS_END_HEADERS;
	if (!resp->bodyLen) {
		flag |= FLAGS_DATA_END_STREAM;
	}
	Frame respFrame = {
		.flags = flag,
		.type = FT_HEADERS,
		.id = resp->frameID,
	};
	HeadesFrame_encode(&respFrame, &resphf);

	char frameHeader[10] = {0};
	Frame_encode_header(&respFrame, frameHeader);

	// printf("frameHeader: ");
	// print_hex(frameHeader, 9);
	SSL_write(conn->cSSL, frameHeader, 9);

	printf("wrote: "); Frame_dump(&respFrame);
	// printf("payload (len=%d): ", respFrame.len);
	print_hex(respFrame.payload, respFrame.len);

	SSL_write(conn->cSSL, respFrame.payload, respFrame.len);

	// Data Frame

	if (resp->bodyLen > 0) {
		struct DataFrame df = {
			.padLength = 0, .padding = NULL,
			.dataLen = resp->bodyLen, .data = resp->body,
		};
		Frame respFrame = {
			.flags = FLAGS_DATA_END_STREAM,
			.type = FT_DATA,
			.id = resp->frameID,
			// .id = Server_next_id(server),
		};
		DataFrame_encode(&respFrame, &df);

		char frameHeader[10] = {0};
		Frame_encode_header(&respFrame, frameHeader);

		printf("wrote: "); Frame_dump(&respFrame);
		SSL_write(conn->cSSL, frameHeader, 9);
		SSL_write(conn->cSSL, respFrame.payload, respFrame.len);
	}
}
