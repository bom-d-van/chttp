#ifndef _huffman_h
#define _huffman_h

char *huffman_encode(char *data, int len, int *size);

// An indexed header field starts with the '1' 1-bit pattern, followed by the index of the matching header field, represented as an integer with a 7-bit prefix (see Section 5.1).
// A literal header field with incremental indexing representation starts with the '01' 2-bit pattern.
// A literal header field without indexing representation starts with the '0000' 4-bit pattern.
// A literal header field never-indexed representation starts with the '0001' 4-bit pattern.
// A dynamic table size update starts with the '001' 3-bit pattern, followed by the new maximum size, represented as an integer with a 5-bit prefix (see Section 5.1).
//
// custom-key: custom-header
//
// 40
// 0a637573746f6d2d6b6579
// 0d637573746f6d2d686561646572

void init_huffman_tree();
char *huffman_decode(char *data, int len, int *size);

char *hpack_encode_integer(int n, int val, int *size);
int hpack_decode_integer(int n, char *code, int codeLen, int *readLen);

extern int huffmanCode[];
extern int huffmanCodeLen[];

struct hpack_header_field {
	char *name;
	char *value;
	int size;
	int ltrName;  // is literal name
	int ltrValue; // is literal value

	// For Dynamic Table
	struct hpack_header_field *prev;
	struct hpack_header_field *next;

	int type;
};

void hpack_header_field_free(struct hpack_header_field *field);
struct hpack_header_field *hpack_get_header(struct hpack_header_field *header, int len, char *name);
void hpack_header_fields_dump(struct hpack_header_field *header, int len);

struct hpack {
	struct hpack_header_field *dynamicTable;
	struct hpack_header_field *dynamicTableTail;
	int dynamicTableLen;
	int dynamicTableSize;
	int maxTableSize;
};

struct hpack_header_field *hpackStaticTable;

struct hpack *hpack_new();
void hpack_free(struct hpack * h);
struct hpack_header_field *hpack_decode(struct hpack *h, char *data, int dataLen, int *fieldLen);
char *hpack_encode(struct hpack *h, struct hpack_header_field *header, int headerLen, int huffmanEnc, int *dataLen);

#define INDEXED_HEADER_FIELD          0x0 // prefix: 1
#define HEADER_FIELD_WITH_INCREMENTAL 0x1 // prefix: 01
#define HEADER_FIELD_WITHOUT_INDEXING 0x2 // prefix: 0000
#define HEADER_FIELD_NEVER_INDEXED    0x3 // prefix: 0001
#define DYNAMIC_TABLE_SIZE_UPDATE     0x4 // prefix: 001

#ifdef DEBUG
	#define dprintf(M, ...) printf(M, ##__VA_ARGS__)
#else
	#define dprintf(...)
#endif

#endif
