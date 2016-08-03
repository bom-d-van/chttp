# -rdynamic
CC=cc -g -O2 -Wall -Wextra -Wno-unused -fPIC -I/usr/local/opt/openssl/include -c $(OPTFLAGS)

default: chttp
	# $(CC) reader.c -o bin/reader.o
	# $(CC) chttp.c -o bin/chttp.o
	# cc bin/reader.o bin/chttp.o -o bin/chttp.bin

reader:
	$(CC) reader.c -o bin/reader.o
	$(CC) reader_test.c -o bin/reader_test.o
	cc bin/reader.o bin/reader_test.o -o bin/reader_test.bin

request:
	$(CC) reader.c -o bin/reader.o
	$(CC) request.c -o bin/request.o
	$(CC) request_test.c -o bin/request_test.o
	cc bin/reader.o bin/request.o bin/request_test.o -o bin/request_test.bin

trie:
	# $(CC) request.c -o bin/request.o
	# $(CC) request.c -o bin/request.o
	$(CC) trie.c -o bin/trie.o
	$(CC) trie_test.c -o bin/trie_test.o
	cc bin/trie.o bin/trie_test.o -o bin/trie_test.bin

frame:
	$(CC) frame.c -o bin/frame.o
	$(CC) util.c -o bin/util.o
	$(CC) frame_test.c -o bin/frame_test.o
	cc bin/util.o bin/frame.o bin/frame_test.o -o bin/frame_test.bin

chttp:
	$(CC) reader.c -o bin/reader.o
	$(CC) request.c -o bin/request.o
	$(CC) trie.c -o bin/trie.o
	$(CC) chttp.c -o bin/chttp.o
	$(CC) util.c -o bin/util.o
	$(CC) frame.c -o bin/frame.o
	$(CC) chttp_test.c -o bin/chttp_test.o
	$(CC) hpack/tables.c -o bin/tables.o
	$(CC) hpack/hpack.c -o bin/hpack.o
	cc $(OPTFLAGS) -L/usr/local/opt/openssl/lib -I/usr/local/opt/openssl/include -lssl -lcrypto -lpthread -lm bin/reader.o bin/request.o bin/trie.o bin/frame.o bin/util.o bin/chttp.o bin/chttp_test.o bin/tables.o bin/hpack.o -o bin/chttp_test.bin

hpacks:
	$(CC) hpack/tables.c -o bin/tables.o
	$(CC) hpack/hpack.c -o bin/hpack.o
	$(CC) util.c -o bin/util.o
	$(CC) hpack/hpack_test.c -o bin/hpack_test.o
	cc $(OPTFLAGS) bin/tables.o bin/util.o bin/hpack.o bin/hpack_test.o -o bin/hpack_test.bin
