#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "hpack.h"

char *huffman_encode(char *data, int len, int *size)
{
	int hlen = 0;
	for (int i = 0; i < len; i++) {
		hlen += huffmanCodeLen[(int)data[i]];
	}
	*size = (hlen+7)/8;
	char *result = calloc((int)*size+1, sizeof(char));

	int rembits = 8;
	int rlen = 0;
	for (int i = 0; i < len; i++) {
		int dbits = huffmanCode[(int)data[i]];
		int dlen = huffmanCodeLen[(int)data[i]];
		while (dlen > 0) {
			int cbits = dbits;
			// printf("dlen = %d rlen = %d rembits = %d\n", dlen, rlen, rembits);
			if (dlen > rembits) {
				cbits = dbits >> (dlen - rembits);
				dlen -= rembits;
				rembits = 0;
			} else {
				cbits = dbits << (rembits - dlen);
				rembits -= dlen;
				dlen = 0;
			}

			result[rlen] |= cbits;

			if (rembits == 0) {
				rlen++;
				rembits = 8;
			}
		}
	}
	if (rembits > 0) {
		result[rlen] |= (1 << rembits) - 1;
	}
	return result;
}

struct huffman_node{
	struct huffman_node *zero;
	struct huffman_node *one;
	char val;
} huffman_tree;

void init_huffman_tree()
{
	huffman_tree.zero = NULL;
	huffman_tree.one = NULL;
	huffman_tree.val = -1;
	for (int i = 0; i < 256; i++) {
		int code = huffmanCode[i];
		int len = huffmanCodeLen[i];
		struct huffman_node *cur = &huffman_tree;
		for (int i = len-1; i >= 0; i--) {
			if ((code >> i) & 1) {
				if (!cur->one) {
					cur->one = (struct huffman_node*)calloc(1, sizeof(struct huffman_node));
					cur->one->zero = NULL;
					cur->one->one = NULL;
					cur->one->val = -1;
				}
				cur = cur->one;
			} else {
				if (!cur->zero) {
					cur->zero = (struct huffman_node*)calloc(1, sizeof(struct huffman_node));
					cur->zero->zero = NULL;
					cur->zero->one = NULL;
					cur->zero->val = -1;
				}
				cur = cur->zero;
			}
		}
		cur->val = i;
	}
}

char *huffman_decode(char *data, int len, int *size)
{
	int cap = 1024;
	char *result = calloc(cap, sizeof(char));
	*size = 0;
	struct huffman_node *cur = &huffman_tree;
	for (int i = 0; i < len; i++) {
		// printf("i = %d\n", i);
		char code = data[i];
		for (int j = 7; j >= 0; j--) {
			if ((code >> j) & 1) {
				cur = cur->one;
			} else {
				cur = cur->zero;
			}
			if (cur->val == -1) {
				continue;
			}
			result[*size] = cur->val;
			*size += 1;
			// printf("size: %d\n", *size);
			cur = &huffman_tree;
			if (*size >= cap) {
				cap *= 2;
				result = realloc(result, cap * sizeof(char));
			}
		}
	}
	return result;
}

// Pseudocode to represent an integer I is as follows:
// if I < 2^N - 1, encode I on N bits
// else
//     encode (2^N - 1) on N bits
//     I = I - (2^N - 1)
//     while I >= 128
//          encode (I % 128 + 128) on 8 bits
//          I = I / 128
//     encode I on 8 bits

char *hpack_encode_integer(int n, int val, int *size)
{
	char *result = calloc(4, sizeof(char));
	int cap = 4;
	*size = 0;
	if (val < ((1 << n) - 1)) {
		*size = 1;
		result[0] |= val;
	} else {
		result[0] |= (1 << n) - 1;
		val = val - ((1 << n) -1);
		*size = *size + 1;
		while (val > 128) {
			result[*size] = (val % 128) + 128;
			val = val / 128;
			*size = *size + 1;
			if (*size > cap) {
				cap *= 2;
				result = realloc(result, cap * sizeof(char));
			}
		}
		result[*size] = val;
		*size = *size + 1;
	}
	return result;
}

// Pseudocode to decode an integer I is as follows:
// decode I from the next N bits
// if I < 2^N - 1, return I
// else
//     M = 0
//     repeat
//         B = next octet
//         I = I + (B & 127) * 2^M
//         M = M + 7
//     while B & 128 == 128
//     return I

// readLen should be initialized by the caller, not by this hpack_decode_integer.
int hpack_decode_integer(int n, char *code, int codeLen, int *readLen)
{
	*readLen += 1;
	int val = (code[0] & ((1 << n) - 1));
	if (codeLen == 1 || val < ((1 << n) - 1) || !(code[1] & 128)) {
		return val;
	}
	int m = 0;
	for (int i = 1; i < codeLen; i++) {
		char b = code[i];
		val += (b & 127) * (1 << m);
		m += 7;
		(*readLen)++;
		if ((b & 128) != 128) {
			break;
		}
	}
	return val;
}

// returnLen should be initialized by the caller, not by this hpack_decode_string.
char *hpack_decode_string(char *data, int dataLen, int *returnLen)
{
	int readLen = 0;
	int valLen = hpack_decode_integer(7, data, dataLen, &readLen);

	char *result = calloc(valLen+1, sizeof(char));
	strlcpy(result, &data[readLen], valLen+1);

	(*returnLen) += readLen + valLen;

	if (data[0] & 128) {
		int size = 0;
		char *nresult = huffman_decode(result, valLen, &size);
		free(result);
		result = nresult;
	}

	return result;
}

struct hpack *hpack_new()
{
	struct hpack* h = calloc(1, sizeof(struct hpack));
	h->dynamicTable = NULL;
	h->maxTableSize = 4 * 1024; // 4K
	h->dynamicTableLen = 0;
	h->dynamicTableSize = 0;
	h->fieldsLen = 0;
	return h;
}

struct hpack_header_field hpack_lookup_field(struct hpack *h, int index)
{
	struct hpack_header_field field;
	if (index < 62) {
		field = hpackStaticTable[index];
	} else {
		index -= 62;
		struct hpack_header_field *fieldptr = h->dynamicTable;
		while (index >= 0 && fieldptr) {
			index--;
			field = *fieldptr;
			fieldptr = fieldptr->next;
		};
		// if (index != 0) {
		// 	field.name = "";
		// }
	}

	field.prev = NULL;
	field.next = NULL;
	field.size = 0;
	field.name = field.ltrName ? field.name : strdup(field.name);
	field.value = field.ltrValue ? field.value : strdup(field.value);

	return field;
}

int hpack_add_field(struct hpack *h, struct hpack_header_field field)
{
	// printf("field.name = %s\n", field.name);
	// printf("field.value = %s\n", field.value);
	field.size = strlen(field.name) + strlen(field.value) + 32;
	field.prev = NULL;
	field.next = NULL;

	// printf("field.size = %d\n", field.size);

	if (field.size > h->maxTableSize) {
		return 1;
	}

	while (h->dynamicTableSize + field.size > h->maxTableSize) {
		struct hpack_header_field *field = h->dynamicTableTail;
		if (field == NULL) {
			break;
		}

		h->dynamicTableTail = field->prev;
		if (h->dynamicTableTail) {
			h->dynamicTableTail->next = NULL;
		}
		h->dynamicTableSize -= field->size;
		h->dynamicTableLen--;
		hpack_header_field_free(field);
	}

	h->dynamicTableSize += field.size;

	struct hpack_header_field *fieldptr = calloc(1, sizeof(struct hpack_header_field));
	*fieldptr = field;
	fieldptr->name = field.ltrName ? field.name : strdup(field.name);
	fieldptr->value = field.ltrValue ? field.value : strdup(field.value);

	fieldptr->next = h->dynamicTable;
	if (h->dynamicTable) {
		h->dynamicTable->prev = fieldptr;
	}
	h->dynamicTable = fieldptr;
	if (h->dynamicTableTail == NULL) {
		h->dynamicTableTail = fieldptr;
	}
	h->dynamicTableLen++;
	return 0;
}

void hpack_header_field_free(struct hpack_header_field *field)
{
	if (!field->ltrName) {
		free(field->name);
	}
	if (!field->ltrValue) {
		free(field->value);
	}
	free(field);
}

void hpack_decode_field(struct hpack *h, char *data, int dataLen, int *i, struct hpack_header_field *field, int prefix)
{
	if (data[*i] & ((1 << prefix) - 1)) {
		int readLen = 0;
		int index = hpack_decode_integer(prefix, &data[*i], dataLen-*i, &readLen);
		(*i) += readLen;

		// TODO: handle field not found
		*field = hpack_lookup_field(h, index);
		dprintf("  Indexed name (idx = %d)\n    %s\n", index, field->name);
	} else {
		(*i)++; // skip header field
		int len = 0;
		field->name = hpack_decode_string(&data[(*i)], dataLen-(*i), &len);
		(*i) += len;
		dprintf("  Literal name (len = %d)\n    %s\n", len, field->name);
	}

	int len = 0;
	field->value = hpack_decode_string(&data[*i], dataLen-(*i), &len);
	(*i) += len;
	dprintf("  Literal value (len = %d)\n    %s\n", len, field->value);
	dprintf("--> %s: %s\n", field->name, field->value);
}

// 40                                      | == Literal indexed ==
// 0a                                      |   Literal name (len = 10)
// 6375 7374 6f6d 2d6b 6579                | custom-key
// 0d                                      |   Literal value (len = 13)
// 6375 7374 6f6d 2d68 6561 6465 72        | custom-header
//                                         | -> custom-key:
//                                         |   custom-header
struct hpack_header_field *hpack_decode(struct hpack *h, char *data, int dataLen, int *fieldLen)
{
	int cap = 8;
	struct hpack_header_field *list = calloc(cap, sizeof(struct hpack_header_field));
	*fieldLen = 0;
	for (int i = 0; i < dataLen;) {
		char code = data[i];
		struct hpack_header_field field = {
			.name = "", .value = "",
			.prev = NULL, .next = NULL,
			.ltrName = 0, .ltrValue = 0, .size = 0,
		};

		if (code & 128) {       // INDEXED_HEADER_FIELD 1
			int readLen = 0;
			int index = hpack_decode_integer(7, &data[i], dataLen-i, &readLen);
			i += readLen;

			// TODO: handle field not found
			field = hpack_lookup_field(h, index);

			dprintf("== Indexed - Add ==\n");
			dprintf("  idx = %d\n", index);
			dprintf("--> %s: %s\n", field.name, field.value);
		} else if (code & 64) { // HEADER_FIELD_WITH_INCREMENTAL 01
			dprintf("== Literal indexed ==\n");
			hpack_decode_field(h, data, dataLen, &i, &field, 6);
			int err = hpack_add_field(h, field);
			if (err) {
				return NULL;
			}
		} else if (code & 32) { // DYNAMIC_TABLE_SIZE_UPDATE 001
			// TODO
			field = (struct hpack_header_field){};
		} else if (code & 16) { // HEADER_FIELD_NEVER_INDEXED 0001
			dprintf("== Literal never indexed ==\n");
			hpack_decode_field(h, data, dataLen, &i, &field, 4);
		} else {                // HEADER_FIELD_WITHOUT_INDEXING 0000
			dprintf("== Literal not indexed ==\n");
			hpack_decode_field(h, data, dataLen, &i, &field, 4);
		}

		list[(*fieldLen)++] = field;
		if (*fieldLen > cap) {
			cap *= 2;
			list = realloc(list, cap * sizeof(struct hpack_header_field));
		}
	}
	return list;

	// printf("-------------------\n");
	// for (int i = 0; i < *fieldLen; i++) {
	// 	printf("%s: %s\n", list[i].name, list[i].value);
	// }
	// printf("-------------------\n");
}

#define dynamicTableBeginIndex 62

int hpack_lookup_index(struct hpack *h, struct hpack_header_field *field, int *fullMatch)
{
	int index = 0;     // only name match
	int fullIndex = 0; // name and value match
	for (int i = 1; i < dynamicTableBeginIndex; i++) {
		if (!strcmp(hpackStaticTable[i].name, field->name) && !index) {
			index = i;
		}
		if (!strcmp(hpackStaticTable[i].value, field->value)) {
			fullIndex = i;
			(*fullMatch) = 1;
			break;
		}
	}

	if (!fullIndex) {
		struct hpack_header_field *f = h->dynamicTable;
		for (int i = 0; i < h->dynamicTableLen; i++) {
			if (!strcmp(f->name, field->name) && !strcmp(f->value, field->value)) {
				fullIndex = i + dynamicTableBeginIndex;
				(*fullMatch) = 1;
				break;
			}
			if (f != NULL) {
				f = f->next;
			}
		}
	}
	if (fullIndex) {
		return fullIndex;
	}
	return index;
}

char *append_string(char *dst, int *dlen, int *cap, char *src, int slen)
{
	if ((*cap) - (*dlen) - 1 < slen) {
		*cap = ((*cap) + slen) * 1.5;
		dst = realloc(dst, (*cap) * sizeof(char));
	}
	strcat(&dst[*dlen], src);
	(*dlen) += slen;
	dst[*dlen] = '\0';
	return dst;
}

char *hpack_encode_string(char *data, int dataLen, int huffmanEnc, int *returnLen)
{
	int cap = 256;
	char *result = calloc(cap, sizeof(char));
	// printf("huffmanEnc = %d\n", huffmanEnc);
	if (huffmanEnc) {
		int len = 0;
		data = huffman_encode(data, dataLen, &len);
		dataLen = len;
	}

	int ilen = 0;
	char *integer = hpack_encode_integer(7, dataLen, &ilen);
	result = append_string(result, returnLen, &cap, integer, ilen);

	// printf("data = %s\n", data);
	// printf("dataLen = %d\n", dataLen);

	result = append_string(result, returnLen, &cap, data, dataLen);

	if (huffmanEnc) {
		result[0] |= 0x80; // prefix: 1
	} else {
		result[0] &= 0X7F; // prefix: 1
	}

	return result;
}

char *hpack_encode(struct hpack *h, struct hpack_header_field *header, int headerLen, int huffmanEnc, int *dataLen)
{
	int cap = 1024;
	char *data = calloc(cap, sizeof(char));
	for (int i = 0; i < headerLen; i++) {
		struct hpack_header_field f = header[i];
		int fullMatch = 0;
		int index = hpack_lookup_index(h, &f, &fullMatch);
		// printf("index = %d\n", index);
		if (fullMatch) {
			int len = 0;
			char *integer = hpack_encode_integer(7, index, &len);
			integer[0] |= 0x80; // prefix: 1
			data = append_string(data, dataLen, &cap, integer, len);
			continue;
		}

		int prefix = 0x40;
		switch (f.type) {
		case HEADER_FIELD_WITHOUT_INDEXING:
			prefix = 0x00;
			break;
		case HEADER_FIELD_NEVER_INDEXED:
			prefix = 0x10;
			break;
		}

		if (index) {
			int len = 0;
			char *integer = hpack_encode_integer(6, index, &len);
			integer[0] |= prefix;
			data = append_string(data, dataLen, &cap, integer, len);
		} else {
			data[(*dataLen)++] = prefix;
			int len = 0;
			char *string = hpack_encode_string(f.name, strlen(f.name), huffmanEnc, &len);
			data = append_string(data, dataLen, &cap, string, len);
		}

		int len = 0;
		char *string = hpack_encode_string(f.value, strlen(f.value), huffmanEnc, &len);
		data = append_string(data, dataLen, &cap, string, len);

		if (f.type != HEADER_FIELD_WITH_INCREMENTAL) {
			continue;
		}

		int err = hpack_add_field(h, f);
		if (err) {
			return NULL;
		}
	}
	return data;
}
