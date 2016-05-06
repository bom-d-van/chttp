#include <stdlib.h>
#include "util.h"
#include "chttp.h"

int main(void)
{
	Frame frame;
	int len = 0;
	frame.len = 0x10;
	frame.type = FT_SETTINGS;
	frame.flags = FLAG_SETTTINGS_ACK;
	frame.id = 2;

	char wantResult[] = {0x00, 0x00, 0x10, 0x4, 0x01, 0x00, 0x00, 0x00, 0x02};

	char result[9] = {0};
	Frame_encode_header(&frame, result);

	if (strcmp(result, wantResult)) {
		printf("result = "); print_hex(result, 9);
		printf("want "); print_hex(wantResult, 9);
		exit(1);
	}

	Frame nframe;
	Frame_decode_header(&nframe, result);

	int wantFlags = FLAG_SETTTINGS_ACK;
	int wantType = FT_SETTINGS;
	int wantID = 2;
	int wantLen = 0x10;
	if (nframe.flags != wantFlags) {printf("nframe.flags = %d; want %d\n", nframe.flags,  wantFlags); exit(1);}
	if (nframe.type != wantType)   {printf("nframe.type = %d; want %d\n", nframe.type,  wantType); exit(1);}
	if (nframe.id != wantID)       {printf("nframe.id = %d; want %d\n", nframe.id,  wantID); exit(1);}
	if (nframe.len != wantLen)     {printf("nframe.len = %d; want %d\n", nframe.len,  wantLen); exit(1);}

	bzero(result, sizeof(char)*9);
	Frame_encode_header(&nframe, result);
	if (strcmp(result, wantResult)) {
		printf("result = "); print_hex(result, 9);
		printf("want "); print_hex(wantResult, 9);
		exit(1);
	}

	return 0;
}
