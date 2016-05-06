#include "chttp.h"

Frame *Frame_new(int type)
{
	Frame *frame = calloc(1, sizeof(Frame));
	frame->type = type;
	frame->len = 0;
	frame->flags = 0;
	frame->id = 0;
	frame->payload = NULL;
	// frame->payloadSize = 0;
	return frame;
}

void Frame_free(Frame *frame)
{
	free(frame);
	if (frame->payload) {
		free(frame->payload);
	}
}

void Frame_encode_header(Frame *frame, char *result)
{
	// char *result = calloc(9, sizeof(char));

	result[0] = frame->len >> 16;
	result[1] = frame->len >> 8;
	result[2] = frame->len;

	result[3] = frame->type;
	result[4] = frame->flags;

	result[5] = frame->id >> 24;
	result[6] = frame->id >> 16;
	result[7] = frame->id >> 8;
	result[8] = frame->id;

	// return result;
}

void Frame_decode_header(Frame *frame, char *data)
{
	// char *result = calloc(9, sizeof(char));
	// frame->len = (2<<24) - 1;

	// 0000 0000 0000 0000 0000 0000
	// 0000 0000 0000 0000 0000 0000
	frame->len = 0;
	frame->len = data[0] << 16 | data[1] << 8 | data[2];
	// frame->len |= ;
	// frame->len |= ;
	frame->len &= ((2<<24) - 1);
	// printf("--- %x\n", (2<<23)-1);

	frame->type = data[3];
	frame->flags = data[4];

	frame->id = 0;
	frame->id |= data[5] << 24;
	frame->id |= data[6] << 16;
	frame->id |= data[7] << 8;
	frame->id |= data[8];

	// return 0;
}

void Frame_dump(Frame *frame)
{
	char *header = "";
	switch (frame->type) {
	case FT_HEADERS:
		header = "HEADERS"; break;
	case FT_PRIORITY:
		header = "PRIORITY"; break;
	case FT_RST_STREAM:
		header = "RST_STREAM"; break;
	case FT_SETTINGS:
		header = "SETTINGS"; break;
	case FT_PUSH_PROMISE:
		header = "PUSH_PROMISE"; break;
	case FT_PING:
		header = "PING"; break;
	case FT_GOAWAY:
		header = "GOAWAY"; break;
	case FT_WINDOW_UPDATE:
		header = "WINDOW_UPDATE"; break;
	case FT_CONTINUATION:
		header = "CONTINUATION"; break;
	}

	printf("[FrameHeader %s len=%d flag=%x]\n", header, frame->len, frame->flags);

	if (frame->payload) {
		print_hex(frame->payload, frame->len);
	}
}
