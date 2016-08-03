#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "hpack.h"
#include "../util.h"

int exitCode = 0;

struct sample {
	char *text;
	int textLen;
	char *code;
	int codeLen;
};

char code0[] = {0xF1, 0xE3, 0xC2, 0xE5, 0xF2, 0x3A, 0x6B, 0xA0, 0xAB, 0x90, 0xF4, 0xFF, 0x00};
char code1[] = {0xA8, 0xEB, 0x10, 0x64, 0x9C, 0xBF, 0x00};
char code2[] = {0x25, 0xA8, 0x49, 0xE9, 0x5B, 0xA9, 0x7D, 0x7F, 0x00};
char code3[] = {0x25, 0xA8, 0x49, 0xE9, 0x5B, 0xB8, 0xE8, 0xB4, 0xBF, 0x00};
char code4[] = {0xAE, 0xC3, 0x77, 0x1A, 0x4B, 0x00};
char code5[] = {0xD0, 0x7A, 0xBE, 0x94, 0x10, 0x54, 0xD4, 0x44, 0xA8, 0x20, 0x05, 0x95, 0x04, 0x0B, 0x81, 0x66, 0xE0, 0x82, 0xA6, 0x2D, 0x1B, 0xFF, 0x00};
char code6[] = {0x9D, 0x29, 0xAD, 0x17, 0x18, 0x63, 0xC7, 0x8F, 0x0B, 0x97, 0xC8, 0xE9, 0xAE, 0x82, 0xAE, 0x43, 0xD3, 0x00};
char code7[] = {0x64, 0x0E, 0xFF, 0x00};
char code8[] = {0x94, 0xE7, 0x82, 0x1D, 0xD7, 0xF2, 0xE6, 0xC7, 0xB3, 0x35, 0xDF, 0xDF, 0xCD, 0x5B, 0x39, 0x60, 0xD5, 0xAF, 0x27, 0x08, 0x7F, 0x36, 0x72, 0xC1, 0xAB, 0x27, 0x0F, 0xB5, 0x29, 0x1F, 0x95, 0x87, 0x31, 0x60, 0x65, 0xC0, 0x03, 0xED, 0x4E, 0xE5, 0xB1, 0x06, 0x3D, 0x50, 0x07, 0x00};

struct sample samples[] = {
	/*0*/ {.text = "www.example.com", .textLen = 15, .code = code0, .codeLen = 12},
	/*1*/ {.text = "no-cache", .textLen = 8, .code = code1, .codeLen = 6},
	/*2*/ {.text = "custom-key", .textLen = 10, .code = code2, .codeLen = 8},
	/*3*/ {.text = "custom-value", .textLen = 12, .code = code3, .codeLen = 9},
	/*4*/ {.text = "private", .textLen = 7, .code = code4, .codeLen = 5},
	/*5*/ {.text = "Mon, 21 Oct 2013 20:13:21 GMT", .textLen = 29, .code = code5, .codeLen = 22},
	/*6*/ {.text = "https://www.example.com", .textLen = 23, .code = code6, .codeLen = 17},
	/*7*/ {.text = "307", .textLen = 3, .code = code7, .codeLen = 3},
	/*8*/ {.text = "foo=ASDJKHQKBZXOQWEOPIUAXQWEOIU; max-age=3600; version=1", .textLen = 56, .code = code8, .codeLen = 45},
	/*9*/ {
		.text = "text/html; charset=utf-8", .textLen = 24,
		.code = (char[]){0x49, 0x7C, 0xA5, 0x89, 0xD3, 0x4D, 0x1F, 0x6A, 0x12, 0x71, 0xD8, 0x82, 0xA6, 0x0B, 0x53, 0x2A, 0xCF, 0x7F, 0x00}, .codeLen = 18,
	},
};

void test_encode()
{
	printf("== test_encode\n");
	for (int i = 0; i < (int)(sizeof(samples)/sizeof(struct sample)); i++) {
		dprintf("case %d\n", i);
		struct sample s = samples[i];
		int gotSize = 0;
		char *gotResult = huffman_encode(s.text, s.textLen, &gotSize);
		if (gotSize != s.codeLen) {
			printf("gotSize = %d; want %d\n", gotSize, s.codeLen);
			exitCode = 1;
		}
		if (strcmp(gotResult, s.code)) {
			printf("got =  ");
			print_hex(gotResult, gotSize);
			printf("code = ");
			print_hex(s.code, s.codeLen);
			exitCode = 1;
		}
	}
}

void test_decode()
{
	printf("== test_decode\n");
	for (int i = 0; i < (int)(sizeof(samples)/sizeof(struct sample)); i++) {
		struct sample s = samples[i];
		int gotSize = 0;
		char *gotResult = huffman_decode(s.code, s.codeLen, &gotSize);
		if (gotSize != s.textLen) {
			printf("%s\n", s.text);
			printf("gotSize = %d; want %d\n", gotSize, s.textLen);
			exitCode = 1;
		}
		if (strcmp(gotResult, s.text)) {
			printf("got =  ");
			print_hex(gotResult, gotSize);
			printf("text = ");
			print_hex(s.text, s.textLen);
			exitCode = 1;
		}
	}
}

struct integer_sample {
	int n;
	int val;
	char *code;
	int codeLen;
	int prefix;
	int readLen;
};

char integer_code1[] = {0xEA, 0x00};
char integer_code2[] = {0x9F, 0x9A, 0xA, 0x00};
char integer_code3[] = {0x2A, 0x00};
char integer_code4[] = {0x0A, 0x63, 0x75, 0x73, 0x74, 0x6F, 0x6D, 0x2D, 0x6B, 0x65, 0x79, 0x0D, 0x63, 0x75, 0x73, 0x74, 0x6F, 0x6D, 0x2D, 0x68, 0x65, 0x61, 0x64, 0x65, 0x72, 0x00};
char integer_code5[] = {0x82, 0x86, 0x84, 0x41, 0x0F, 0x77, 0x77, 0x77, 0x2E, 0x65, 0x78, 0x61, 0x6D, 0x70, 0x6C, 0x65, 0x2E, 0x63, 0x6F, 0x6D, 0x00};

struct integer_sample integer_samples[] = {
	{.n = 5, .val = 10,   .codeLen = 1,  .code = integer_code1, .readLen = 1, .prefix = 0xE0}, // 0xE0 -> 11100000
	{.n = 5, .val = 1337, .codeLen = 3,  .code = integer_code2, .readLen = 3, .prefix = 0x80}, // 0xE0 -> 11100000
	{.n = 8, .val = 42,   .codeLen = 1,  .code = integer_code3, .readLen = 1, .prefix = 0x0},
	{.n = 7, .val = 10,   .codeLen = 25, .code = integer_code4, .readLen = 1, .prefix = 0x0},
	{.n = 7, .val = 2,    .codeLen = 20, .code = integer_code5, .readLen = 1, .prefix = 0x80},
};

void test_hpack_encode_integer()
{
	printf("== test_hpack_encode_integer\n");
	for (size_t i = 0; i < sizeof(integer_samples)/sizeof(struct integer_sample); i++) {
		struct integer_sample s = integer_samples[i];
		int size = 0;
		char *result = hpack_encode_integer(s.n, s.val, &size);
		char *code = calloc(s.readLen+1, sizeof(char));
		strlcpy(code, s.code, s.readLen+1);
		if (size != s.readLen) {
			printf("case[%zu]: size = %d; want %d\n", i, size, s.readLen);
			exitCode = 1;
		}
		result[0] |= s.prefix;
		if (strcmp(result, code)) {
			printf("case[%zu]: \n", i);
			printf("result = ");
			print_hex(result, size);
			printf("want   = ");
			print_hex(code, s.readLen);
			exitCode = 1;
		}
		free(code);
	}
}

void test_hpack_decode_integer()
{
	printf("== test_hpack_decode_integer\n");
	for (size_t i = 0; i < sizeof(integer_samples)/sizeof(struct integer_sample); i++) {
		struct integer_sample s = integer_samples[i];
		s.code[0] |= s.prefix; // make sure prefix doesn't get in the way of decoding
		int readLen = 0;
		int val = hpack_decode_integer(s.n, s.code, s.codeLen, &readLen);
		if (val != s.val) {
			printf("val = %d; want %d\n", val, s.val);
			exitCode = 1;
		}
		if (readLen != s.readLen) {
			printf("readLen = %d; want %d\n", readLen, s.readLen);
			exitCode = 1;
		}
	}
}

struct hpack_decode_sample_unit {
	char *data;
	int dataLen;

	struct hpack_header_field *fields;
	int fieldsLen;

	struct hpack_header_field *dynamicTable;
	int dynamicTableLen;
	int dynamicTableSize;
};

struct hpack_decode_sample {
	size_t unitLen;
	int maxDynamicTableSize;
	struct hpack_decode_sample_unit *unit;
	int huffmanEnc;

	int hasEncodeTest;
	int hasDecodeTest;
};

struct hpack_decode_sample hpack_decode_samples[] = {
	{
		// case 0
		.unitLen = 1,
		.maxDynamicTableSize = 0,
		.huffmanEnc = 0,
		.hasEncodeTest = 1,
		.hasDecodeTest = 1,
		.unit = (struct hpack_decode_sample_unit[]){
			{
				.data = (char[]){0x82, 0x00}, .dataLen = 1,
				.fieldsLen = 1,
				.fields = (struct hpack_header_field[]){
					{.name = ":method", .value = "GET",},
				},
				.dynamicTableLen = 0, .dynamicTableSize = 0,
			},
		},
	},
	{
		// case 1
		.unitLen = 1,
		.maxDynamicTableSize = 0,
		.huffmanEnc = 0,
		.hasEncodeTest = 1,
		.hasDecodeTest = 1,
		.unit = (struct hpack_decode_sample_unit[]){
			{
				.data = (char[]){0x40, 0x0A, 0x63, 0x75, 0x73, 0x74, 0x6F, 0x6D, 0x2D, 0x6B, 0x65, 0x79, 0x0D, 0x63, 0x75, 0x73, 0x74, 0x6F, 0x6D, 0x2D, 0x68, 0x65, 0x61, 0x64, 0x65, 0x72, 0x00}, .dataLen = 26,
				.fieldsLen = 1,
				.fields = (struct hpack_header_field[]){
					{.name = "custom-key", .value = "custom-header", .type = HEADER_FIELD_WITH_INCREMENTAL},
				},
				.dynamicTableLen = 1, .dynamicTableSize = 55,
				.dynamicTable = (struct hpack_header_field[]){
					{.name = "custom-key", .value = "custom-header", .size = 55},
				},
			},
		},
	},
	{
		// case 2
		.unitLen = 1,
		.maxDynamicTableSize = 0,
		.huffmanEnc = 0,
		.hasEncodeTest = 1,
		.hasDecodeTest = 1,
		.unit = (struct hpack_decode_sample_unit[]){
			{
				.data = (char[]){0x04, 0x0C, 0x2F, 0x73, 0x61, 0x6D, 0x70, 0x6C, 0x65, 0x2F, 0x70, 0x61, 0x74, 0x68, 0x00}, .dataLen = 14,
				.fieldsLen = 1,
				.fields = (struct hpack_header_field[]){
					{.name = ":path", .value = "/sample/path", .type = HEADER_FIELD_WITHOUT_INDEXING},
				},
				.dynamicTableLen = 0, .dynamicTableSize = 0,
			},
		},
	},
	{
		// case 3
		.unitLen = 1,
		.maxDynamicTableSize = 0,
		.huffmanEnc = 0,
		.hasEncodeTest = 1,
		.hasDecodeTest = 1,
		.unit = (struct hpack_decode_sample_unit[]){
			{
				.data = (char[]){0x10, 0x08, 0x70, 0x61, 0x73, 0x73, 0x77, 0x6F, 0x72, 0x64, 0x06, 0x73, 0x65, 0x63, 0x72, 0x65, 0x74, 0x00}, .dataLen = 17,
				.fieldsLen = 1,
				.fields = (struct hpack_header_field[]){
					{.name = "password", .value = "secret", .type = HEADER_FIELD_NEVER_INDEXED},
				},
				.dynamicTableLen = 0, .dynamicTableSize = 0,
			},
		},
	},
	{
		// case 4: RFC7541 C.3 Request Examples without Huffman Coding
		.unitLen = 3,
		.maxDynamicTableSize = 0,
		.huffmanEnc = 0,
		.hasEncodeTest = 1,
		.hasDecodeTest = 1,
		.unit = (struct hpack_decode_sample_unit[]){
			{
				.data = (char[]){0x82, 0x86, 0x84, 0x41, 0x0F, 0x77, 0x77, 0x77, 0x2E, 0x65, 0x78, 0x61, 0x6D, 0x70, 0x6C, 0x65, 0x2E, 0x63, 0x6F, 0x6D, 0x00}, .dataLen = 20,
				.fieldsLen = 4,
				.fields = (struct hpack_header_field[]){
					{.name = ":method",    .value = "GET", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = ":scheme",    .value = "http", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = ":path",      .value = "/", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = ":authority", .value = "www.example.com", .type = HEADER_FIELD_WITH_INCREMENTAL},
				},
				.dynamicTableLen = 1, .dynamicTableSize = 57,
				.dynamicTable = (struct hpack_header_field[]){
					{.name = ":authority", .value = "www.example.com", .size = 57},
				},
			},
			{
				.data = (char[]){0x82, 0x86, 0x84, 0xBE, 0x58, 0x08, 0x6E, 0x6F, 0x2D, 0x63, 0x61, 0x63, 0x68, 0x65, 0x00}, .dataLen = 14,
				.fieldsLen = 5,
				.fields = (struct hpack_header_field[]){
					{.name = ":method",    .value = "GET", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = ":scheme",    .value = "http", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = ":path",      .value = "/", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = ":authority", .value = "www.example.com", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = "cache-control", .value = "no-cache", .type = HEADER_FIELD_WITH_INCREMENTAL},
				},
				.dynamicTableLen = 2, .dynamicTableSize = 110,
				.dynamicTable = (struct hpack_header_field[]){
					{.name = "cache-control", .value = "no-cache", .size = 53},
					{.name = ":authority", .value = "www.example.com", .size = 57},
				},
			},
			{
				.data = (char[]){0x82, 0x87, 0x85, 0xBF, 0x40, 0x0A, 0x63, 0x75, 0x73, 0x74, 0x6F, 0x6D, 0x2D, 0x6B, 0x65, 0x79, 0x0C, 0x63, 0x75, 0x73, 0x74, 0x6F, 0x6D, 0x2D, 0x76, 0x61, 0x6C, 0x75, 0x65, 0x00}, .dataLen = 29,
				.fieldsLen = 5,
				.fields = (struct hpack_header_field[]){
					{.name = ":method",    .value = "GET", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = ":scheme",    .value = "https", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = ":path",      .value = "/index.html", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = ":authority", .value = "www.example.com", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = "custom-key", .value = "custom-value", .type = HEADER_FIELD_WITH_INCREMENTAL},
				},
				.dynamicTableLen = 3, .dynamicTableSize = 164,
				.dynamicTable = (struct hpack_header_field[]){
					{.name = "custom-key", .value = "custom-value", .size = 54},
					{.name = "cache-control", .value = "no-cache", .size = 53},
					{.name = ":authority", .value = "www.example.com", .size = 57},
				},
			},
		},
	},
	{
		// case 5: RFC7541 C.4 Request Examples with Huffman Coding
		.unitLen = 3,
		.maxDynamicTableSize = 0,
		.huffmanEnc = 1,
		.hasEncodeTest = 1,
		.hasDecodeTest = 1,
		.unit = (struct hpack_decode_sample_unit[]){
			{
				.data = (char[]){0x82, 0x86, 0x84, 0x41, 0x8C, 0xF1, 0xE3, 0xC2, 0xE5, 0xF2, 0x3A, 0x6B, 0xA0, 0xAB, 0x90, 0xF4, 0xFF, 0x00}, .dataLen = 17,
				.fieldsLen = 4,
				.fields = (struct hpack_header_field[]){
					{.name = ":method",    .value = "GET", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = ":scheme",    .value = "http", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = ":path",      .value = "/", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = ":authority", .value = "www.example.com", .type = HEADER_FIELD_WITH_INCREMENTAL},
				},
				.dynamicTableLen = 1, .dynamicTableSize = 57,
				.dynamicTable = (struct hpack_header_field[]){
					{.name = ":authority", .value = "www.example.com", .size = 57},
				},
			},
			{
				.data = (char[]){0x82, 0x86, 0x84, 0xBE, 0x58, 0x86, 0xA8, 0xEB, 0x10, 0x64, 0x9C, 0xBF, 0x00}, .dataLen = 12,
				.fieldsLen = 5,
				.fields = (struct hpack_header_field[]){
					{.name = ":method",    .value = "GET", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = ":scheme",    .value = "http", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = ":path",      .value = "/", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = ":authority", .value = "www.example.com", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = "cache-control", .value = "no-cache", .type = HEADER_FIELD_WITH_INCREMENTAL},
				},
				.dynamicTableLen = 2, .dynamicTableSize = 110,
				.dynamicTable = (struct hpack_header_field[]){
					{.name = "cache-control", .value = "no-cache", .size = 53},
					{.name = ":authority", .value = "www.example.com", .size = 57},
				},
			},
			{
				.data = (char[]){0x82, 0x87, 0x85, 0xBF, 0x40, 0x88, 0x25, 0xA8, 0x49, 0xE9, 0x5B, 0xA9, 0x7D, 0x7F, 0x89, 0x25, 0xA8, 0x49, 0xE9, 0x5B, 0xB8, 0xE8, 0xB4, 0xBF, 0x00}, .dataLen = 24,
				.fieldsLen = 5,
				.fields = (struct hpack_header_field[]){
					{.name = ":method",    .value = "GET", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = ":scheme",    .value = "https", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = ":path",      .value = "/index.html", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = ":authority", .value = "www.example.com", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = "custom-key", .value = "custom-value", .type = HEADER_FIELD_WITH_INCREMENTAL},
				},
				.dynamicTableLen = 3, .dynamicTableSize = 164,
				.dynamicTable = (struct hpack_header_field[]){
					{.name = "custom-key", .value = "custom-value", .size = 54},
					{.name = "cache-control", .value = "no-cache", .size = 53},
					{.name = ":authority", .value = "www.example.com", .size = 57},
				},
			},
		},
	},
	{
		// case 6: RFC7541 C.5 Response Examples without Huffman Coding
		.unitLen = 3,
		.maxDynamicTableSize = 256,
		.huffmanEnc = 0,
		.hasEncodeTest = 1,
		.hasDecodeTest = 1,
		.unit = (struct hpack_decode_sample_unit[]){
			{
				.data = (char[]){0x48, 0x03, 0x33, 0x30, 0x32, 0x58, 0x07, 0x70, 0x72, 0x69, 0x76, 0x61, 0x74, 0x65, 0x61, 0x1D, 0x4D, 0x6F, 0x6E, 0x2C, 0x20, 0x32, 0x31, 0x20, 0x4F, 0x63, 0x74, 0x20, 0x32, 0x30, 0x31, 0x33, 0x20, 0x32, 0x30, 0x3A, 0x31, 0x33, 0x3A, 0x32, 0x31, 0x20, 0x47, 0x4D, 0x54, 0x6E, 0x17, 0x68, 0x74, 0x74, 0x70, 0x73, 0x3A, 0x2F, 0x2F, 0x77, 0x77, 0x77, 0x2E, 0x65, 0x78, 0x61, 0x6D, 0x70, 0x6C, 0x65, 0x2E, 0x63, 0x6F, 0x6D, 0x00}, .dataLen = 70,
				.fieldsLen = 4,
				.fields = (struct hpack_header_field[]){
					{.name = ":status",       .value = "302", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = "cache-control", .value = "private", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = "date",          .value = "Mon, 21 Oct 2013 20:13:21 GMT", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = "location",      .value = "https://www.example.com", .type = HEADER_FIELD_WITH_INCREMENTAL},
				},
				.dynamicTableLen = 4, .dynamicTableSize = 222,
				.dynamicTable = (struct hpack_header_field[]){
					{.name = "location",      .value = "https://www.example.com", .size = 63},
					{.name = "date",          .value = "Mon, 21 Oct 2013 20:13:21 GMT", .size = 65},
					{.name = "cache-control", .value = "private", .size = 52},
					{.name = ":status",       .value = "302", .size = 42},
				},
			},
			{
				.data = (char[]){0x48, 0x03, 0x33, 0x30, 0x37, 0xC1, 0xC0, 0xBF, 0x00}, .dataLen = 8,
				.fieldsLen = 4,
				.fields = (struct hpack_header_field[]){
					{.name = ":status",       .value = "307", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = "cache-control", .value = "private", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = "date",          .value = "Mon, 21 Oct 2013 20:13:21 GMT", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = "location",      .value = "https://www.example.com", .type = HEADER_FIELD_WITH_INCREMENTAL},
				},
				.dynamicTableLen = 4, .dynamicTableSize = 222,
				.dynamicTable = (struct hpack_header_field[]){
					{.name = ":status",       .value = "307", .size = 42},
					{.name = "location",      .value = "https://www.example.com", .size = 63},
					{.name = "date",          .value = "Mon, 21 Oct 2013 20:13:21 GMT", .size = 65},
					{.name = "cache-control", .value = "private", .size = 52},
				},
			},
			{
				.data = (char[]){0x88, 0xC1, 0x61, 0x1D, 0x4D, 0x6F, 0x6E, 0x2C, 0x20, 0x32, 0x31, 0x20, 0x4F, 0x63, 0x74, 0x20, 0x32, 0x30, 0x31, 0x33, 0x20, 0x32, 0x30, 0x3A, 0x31, 0x33, 0x3A, 0x32, 0x32, 0x20, 0x47, 0x4D, 0x54, 0xC0, 0x5A, 0x04, 0x67, 0x7A, 0x69, 0x70, 0x77, 0x38, 0x66, 0x6F, 0x6F, 0x3D, 0x41, 0x53, 0x44, 0x4A, 0x4B, 0x48, 0x51, 0x4B, 0x42, 0x5A, 0x58, 0x4F, 0x51, 0x57, 0x45, 0x4F, 0x50, 0x49, 0x55, 0x41, 0x58, 0x51, 0x57, 0x45, 0x4F, 0x49, 0x55, 0x3B, 0x20, 0x6D, 0x61, 0x78, 0x2D, 0x61, 0x67, 0x65, 0x3D, 0x33, 0x36, 0x30, 0x30, 0x3B, 0x20, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x3D, 0x31, 0x00}, .dataLen = 98,
				.fieldsLen = 6,
				.fields = (struct hpack_header_field[]){
					{.name = ":status", .value = "200", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = "cache-control", .value = "private", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = "date", .value = "Mon, 21 Oct 2013 20:13:22 GMT", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = "location", .value = "https://www.example.com", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = "content-encoding", .value = "gzip", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = "set-cookie", .value = "foo=ASDJKHQKBZXOQWEOPIUAXQWEOIU; max-age=3600; version=1", .type = HEADER_FIELD_WITH_INCREMENTAL},
				},
				.dynamicTableLen = 3, .dynamicTableSize = 215,
				.dynamicTable = (struct hpack_header_field[]){
					{.name = "set-cookie", .value = "foo=ASDJKHQKBZXOQWEOPIUAXQWEOIU; max-age=3600; version=1", .size = 98},
					{.name = "content-encoding", .value = "gzip", .size = 52},
					{.name = "date",          .value = "Mon, 21 Oct 2013 20:13:22 GMT", .size = 65},
				},
			},
		},
	},
	{
		// case 7: RFC7541 C.6 Response Examples with Huffman Coding
		.unitLen = 3,
		.maxDynamicTableSize = 256,
		.huffmanEnc = 1,
		.hasEncodeTest = 1,
		.hasDecodeTest = 1,
		.unit = (struct hpack_decode_sample_unit[]){
			{
				.data = (char[]){0x48, 0x82, 0x64, 0x02, 0x58, 0x85, 0xAE, 0xC3, 0x77, 0x1A, 0x4B, 0x61, 0x96, 0xD0, 0x7A, 0xBE, 0x94, 0x10, 0x54, 0xD4, 0x44, 0xA8, 0x20, 0x05, 0x95, 0x04, 0x0B, 0x81, 0x66, 0xE0, 0x82, 0xA6, 0x2D, 0x1B, 0xFF, 0x6E, 0x91, 0x9D, 0x29, 0xAD, 0x17, 0x18, 0x63, 0xC7, 0x8F, 0x0B, 0x97, 0xC8, 0xE9, 0xAE, 0x82, 0xAE, 0x43, 0xD3, 0x00}, .dataLen = 54,
				.fieldsLen = 4,
				.fields = (struct hpack_header_field[]){
					{.name = ":status",       .value = "302", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = "cache-control", .value = "private", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = "date",          .value = "Mon, 21 Oct 2013 20:13:21 GMT", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = "location",      .value = "https://www.example.com", .type = HEADER_FIELD_WITH_INCREMENTAL},
				},
				.dynamicTableLen = 4, .dynamicTableSize = 222,
				.dynamicTable = (struct hpack_header_field[]){
					{.name = "location",      .value = "https://www.example.com", .size = 63},
					{.name = "date",          .value = "Mon, 21 Oct 2013 20:13:21 GMT", .size = 65},
					{.name = "cache-control", .value = "private", .size = 52},
					{.name = ":status",       .value = "302", .size = 42},
				},
			},
			{
				.data = (char[]){0x48, 0x83, 0x64, 0x0E, 0xFF, 0xC1, 0xC0, 0xBF, 0x00}, .dataLen = 8,
				.fieldsLen = 4,
				.fields = (struct hpack_header_field[]){
					{.name = ":status",       .value = "307", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = "cache-control", .value = "private", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = "date",          .value = "Mon, 21 Oct 2013 20:13:21 GMT", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = "location",      .value = "https://www.example.com", .type = HEADER_FIELD_WITH_INCREMENTAL},
				},
				.dynamicTableLen = 4, .dynamicTableSize = 222,
				.dynamicTable = (struct hpack_header_field[]){
					{.name = ":status",       .value = "307", .size = 42},
					{.name = "location",      .value = "https://www.example.com", .size = 63},
					{.name = "date",          .value = "Mon, 21 Oct 2013 20:13:21 GMT", .size = 65},
					{.name = "cache-control", .value = "private", .size = 52},
				},
			},
			{
				.data = (char[]){0x88, 0xC1, 0x61, 0x96, 0xD0, 0x7A, 0xBE, 0x94, 0x10, 0x54, 0xD4, 0x44, 0xA8, 0x20, 0x05, 0x95, 0x04, 0x0B, 0x81, 0x66, 0xE0, 0x84, 0xA6, 0x2D, 0x1B, 0xFF, 0xC0, 0x5A, 0x83, 0x9B, 0xD9, 0xAB, 0x77, 0xAD, 0x94, 0xE7, 0x82, 0x1D, 0xD7, 0xF2, 0xE6, 0xC7, 0xB3, 0x35, 0xDF, 0xDF, 0xCD, 0x5B, 0x39, 0x60, 0xD5, 0xAF, 0x27, 0x08, 0x7F, 0x36, 0x72, 0xC1, 0xAB, 0x27, 0x0F, 0xB5, 0x29, 0x1F, 0x95, 0x87, 0x31, 0x60, 0x65, 0xC0, 0x03, 0xED, 0x4E, 0xE5, 0xB1, 0x06, 0x3D, 0x50, 0x07, 0x00}, .dataLen = 79,
				.fieldsLen = 6,
				.fields = (struct hpack_header_field[]){
					{.name = ":status", .value = "200", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = "cache-control", .value = "private", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = "date", .value = "Mon, 21 Oct 2013 20:13:22 GMT", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = "location", .value = "https://www.example.com", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = "content-encoding", .value = "gzip", .type = HEADER_FIELD_WITH_INCREMENTAL},
					{.name = "set-cookie", .value = "foo=ASDJKHQKBZXOQWEOPIUAXQWEOIU; max-age=3600; version=1", .type = HEADER_FIELD_WITH_INCREMENTAL},
				},
				.dynamicTableLen = 3, .dynamicTableSize = 215,
				.dynamicTable = (struct hpack_header_field[]){
					{.name = "set-cookie", .value = "foo=ASDJKHQKBZXOQWEOPIUAXQWEOIU; max-age=3600; version=1", .size = 98},
					{.name = "content-encoding", .value = "gzip", .size = 52},
					{.name = "date",          .value = "Mon, 21 Oct 2013 20:13:22 GMT", .size = 65},
				},
			},
		},
	},

	{
		// case 8
		.unitLen = 1,
		.maxDynamicTableSize = 0,
		.huffmanEnc = 0,
		.hasEncodeTest = 1,
		.hasDecodeTest = 1,
		.unit = (struct hpack_decode_sample_unit[]){
			{
				.data = (char[]){0x6E, 0x01, 0x2F, 0x00}, .dataLen = 3,
				.fieldsLen = 1,
				.fields = (struct hpack_header_field[]){
					{.name = "location", .value = "/",},
				},
				.dynamicTableLen = 0, .dynamicTableSize = 0,
			},
		},
	},

	{
		// case 9
		.unitLen = 1,
		.maxDynamicTableSize = 102400,
		.huffmanEnc = 1,
		.hasEncodeTest = 0,
		.hasDecodeTest = 1,
		.unit = (struct hpack_decode_sample_unit[]){
			{
				.data = (char[]){0x82, 0x04, 0x81, 0x63, 0x41, 0x8E, 0xA0, 0xE4, 0x1D, 0x0B, 0xA4, 0xA8, 0x4A, 0xE4, 0x3D, 0x37, 0x1E, 0x69, 0xA6, 0x7F, 0x87, 0x7A, 0xBD, 0xD0, 0x7F, 0x66, 0xA2, 0x81, 0xB0, 0xDA, 0xE0, 0x53, 0xFA, 0xD0, 0x32, 0x1A, 0xA4, 0x9D, 0x13, 0xFD, 0xA9, 0x92, 0xA4, 0x96, 0x85, 0x34, 0x0C, 0x8A, 0x6A, 0xDC, 0xA7, 0xE2, 0x81, 0x02, 0xE1, 0x0F, 0xDA, 0x96, 0x77, 0xB8, 0xD3, 0xAB, 0x83, 0xFB, 0x53, 0x11, 0x49, 0xD4, 0xEC, 0x08, 0x01, 0x00, 0x02, 0x00, 0xA9, 0x84, 0xD6, 0x16, 0x53, 0xF9, 0x61, 0xA7, 0x57, 0x07, 0x53, 0xB0, 0x49, 0x7C, 0xA5, 0x89, 0xD3, 0x4D, 0x1F, 0x43, 0xAE, 0xBA, 0x0C, 0x41, 0xA4, 0xC7, 0xA9, 0x8F, 0x33, 0xA6, 0x9A, 0x3F, 0xDF, 0x9A, 0x68, 0xFA, 0x1D, 0x75, 0xD0, 0x62, 0x0D, 0x26, 0x3D, 0x4C, 0x79, 0xA6, 0x8F, 0xBE, 0xD0, 0x01, 0x77, 0xFE, 0xBE, 0x58, 0xF9, 0xFB, 0xED, 0x00, 0x17, 0x7B, 0x51, 0x8B, 0x2D, 0x4B, 0x70, 0xDD, 0xF4, 0x5A, 0xBE, 0xFB, 0x40, 0x05, 0xDB, 0x50, 0x8D, 0x9B, 0xD9, 0xAB, 0xFA, 0x52, 0x42, 0xCB, 0x40, 0xD2, 0x5F, 0xA5, 0x23, 0xB3, 0x58, 0x87, 0xA4, 0x7E, 0x56, 0x1C, 0xC5, 0x80, 0x1F, 0x00}, .dataLen = 171,
				.fieldsLen = 9,
				.fields = (struct hpack_header_field[]){
					{.name = ":method", .value  ="GET"},
					{.name = ":path", .value = "/"},
					{.name = ":authority", .value  ="local.test.com:8443"},
					{.name = ":scheme", .value  ="https"},
					{.name = "user-agent", .value  ="Mozilla/5.0 (Macintosh; Intel Mac OS X 10.11; rv:47.0) Gecko/20100101 Firefox/47.0"},
					{.name = "accept", .value  ="text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8"},
					{.name = "accept-language", .value  ="en-US,en;q=0.5"},
					{.name = "accept-encoding", .value  ="gzip, deflate, br"},
					{.name = "cache-control", .value  ="max-age=0"},
				},
				.dynamicTableLen = 0, .dynamicTableSize = 0,
			},
		},
	},

	{
		// case 10
		.unitLen = 1,
		.maxDynamicTableSize = 102400,
		.huffmanEnc = 1,
		.hasEncodeTest = 1,
		.hasDecodeTest = 1,
		.unit = (struct hpack_decode_sample_unit[]){
			{
				.data = (char[]){0x88, 0x5F, 0x92, 0x49, 0x7C, 0xA5, 0x89, 0xD3, 0x4D, 0x1F, 0x6A, 0x12, 0x71, 0xD8, 0x82, 0xA6, 0x0B, 0x53, 0x2A, 0xCF, 0x7F, 0x5C, 0x83, 0x13, 0xEC, 0xFF, 0x00}, .dataLen = 26,
				.fieldsLen = 3,
				.fields = (struct hpack_header_field[]){
					{.name = ":status", .value = "200"},
					{.name = "content-type", .value = "text/html; charset=utf-8"},
					{.name = "content-length", .value = "293"},
				},
				.dynamicTableLen = 0, .dynamicTableSize = 0,
			},
		},
	},
};

void test_hpack_decode()
{
	printf("== test_hpack_decode\n");
	for (size_t i = 0; i < sizeof(hpack_decode_samples)/sizeof(struct hpack_decode_sample); i++) {
		// if (i != 10) {
		// 	continue;
		// }
		dprintf("case = %zu\n", i);
		struct hpack_decode_sample s = hpack_decode_samples[i];
		if (!s.hasDecodeTest) {
			dprintf("hasDecodeTest = false, skipped.");
			continue;
		}
		struct hpack *h = hpack_new();
		if (s.maxDynamicTableSize) {
			h->maxTableSize = s.maxDynamicTableSize;
		}
		for (size_t j = 0; j < s.unitLen; j++) {
			struct hpack_decode_sample_unit unit = s.unit[j];
			int len = 0;
			struct hpack_header_field *list = hpack_decode(h, unit.data, unit.dataLen, &len);
			if (len != unit.fieldsLen) {
				printf("case[%zu][%zu]: fieldsLen = %d; want %d\n", i, j, len, unit.fieldsLen);
				exitCode = 1;
			}
			for (int k = 0; k < len; k++) {
				if (strcmp(list[k].name, unit.fields[k].name)) {
					printf("header[%zu][%zu][%d].name = %s; want %s\n", i, j, k, list[k].name, unit.fields[k].name);
					exitCode = 1;
				}
				if (strcmp(list[k].value, unit.fields[k].value)) {
					printf("header[%zu][%zu][%d].value = %s; want %s\n", i, j, k, list[k].value, unit.fields[k].value);
					exitCode = 1;
				}
			}
			if (!unit.dynamicTableSize) {
				continue;
			}
			if (h->dynamicTableLen != unit.dynamicTableLen) {
				printf("case[%zu][%zu]h->dynamicTableLen = %d; want %d\n", i, j, h->dynamicTableLen, unit.dynamicTableLen);
				exitCode = 1;
			}
			if (h->dynamicTableSize != unit.dynamicTableSize) {
				printf("case[%zu][%zu]h->dynamicTableSize = %d; want %d\n", i, j, h->dynamicTableSize, unit.dynamicTableSize);
				exitCode = 1;
			}
			struct hpack_header_field got = *h->dynamicTable;
			for (int k = 0; k < h->dynamicTableLen; k++) {
				struct hpack_header_field want = unit.dynamicTable[k];
				if (strcmp(got.name, want.name)) {
					printf("dyntable[%zu][%zu][%d].name = %s; want %s\n", i, j, k, got.name, want.name);
					exitCode = 1;
				}
				if (strcmp(got.value, want.value)) {
					printf("dyntable[%zu][%zu][%d].value = %s; want %s\n", i, j, k, got.value, want.value);
					exitCode = 1;
				}
				if (got.size != want.size) {
					printf("dyntable[%zu][%zu][%d].size = %d; want %d\n", i, j, k, got.size, want.size);
					exitCode = 1;
				}
				got = *got.next;
			}
		}
	}
}

void test_hpack_encode()
{
	printf("== test_hpack_encode\n");
	for (size_t i = 0; i < sizeof(hpack_decode_samples)/sizeof(struct hpack_decode_sample); i++) {
		// if (i != 10) {
		// 	continue;
		// }
		dprintf("case %zu\n", i);
		struct hpack_decode_sample s = hpack_decode_samples[i];
		if (!s.hasEncodeTest) {
			dprintf("hasEncodeTest = false, skipped.");
			continue;
		}

		struct hpack *h = hpack_new();
		if (s.maxDynamicTableSize) {
			h->maxTableSize = s.maxDynamicTableSize;
		}
		for (size_t j = 0; j < s.unitLen; j++) {
			// printf("----------------\n\n");
			// printf("case %zu %zu\n", i, j);
			struct hpack_decode_sample_unit unit = s.unit[j];
			int len = 0;
			char *data = hpack_encode(h, unit.fields, unit.fieldsLen, s.huffmanEnc, &len);
			if (len != unit.dataLen) {
				printf("case[%zu][%zu].len = %d; want %d\n", i, j, len, unit.dataLen);
				exitCode = 1;
			}
			if (strcmp(data, unit.data)) {
				printf("case[%zu][%zu]:\n", i, j);
				printf("data = ");
				print_hex(data, len);
				printf("want = ");
				print_hex(unit.data, unit.dataLen);
				exitCode = 1;
			}
			if (!unit.dynamicTableSize) {
				continue;
			}
			if (h->dynamicTableLen != unit.dynamicTableLen) {
				printf("case[%zu][%zu]h->dynamicTableLen = %d; want %d\n", i, j, h->dynamicTableLen, unit.dynamicTableLen);
				exitCode = 1;
			}
			if (h->dynamicTableSize != unit.dynamicTableSize) {
				printf("case[%zu][%zu]h->dynamicTableSize = %d; want %d\n", i, j, h->dynamicTableSize, unit.dynamicTableSize);
				exitCode = 1;
			}
			struct hpack_header_field got = *h->dynamicTable;
			for (int k = 0; k < h->dynamicTableLen; k++) {
				struct hpack_header_field want = unit.dynamicTable[k];
				if (strcmp(got.name, want.name)) {
					printf("dyntable[%zu][%zu][%d].name = %s; want %s\n", i, j, k, got.name, want.name);
					exitCode = 1;
				}
				if (strcmp(got.value, want.value)) {
					printf("dyntable[%zu][%zu][%d].value = %s; want %s\n", i, j, k, got.value, want.value);
					exitCode = 1;
				}
				if (got.size != want.size) {
					printf("dyntable[%zu][%zu][%d].size = %d; want %d\n", i, j, k, got.size, want.size);
					exitCode = 1;
				}
				got = *got.next;
			}
		}
	}
}

int main(void)
{
	init_huffman_tree();
	test_decode();
	test_encode();
	test_hpack_encode_integer();
	test_hpack_decode_integer();
	test_hpack_decode();
	test_hpack_encode();

	exit(exitCode);
}
