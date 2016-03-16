CC=cc -ldl -g -O2 -Wall -Wextra -I /Users/bom_d_van/Code/c/lcthw/src -rdynamic -fPIC -c $(OPTFLAGS)

default:
	$(CC) reader.c -o reader.o
	$(CC) chttp.c -o chttp.o
	cc /Users/bom_d_van/Code/c/lcthw/src/darray.o reader.o chttp.o -o chttp.bin

reader:
	$(CC) reader.c -o reader.o
	$(CC) reader_test.c -o reader_test.o
	cc /Users/bom_d_van/Code/c/lcthw/src/darray.o reader.o reader_test.o -o reader_test.bin

request:
	$(CC) reader.c -o reader.o
	$(CC) request.c -o request.o
	$(CC) request_test.c -o request_test.o
	cc /Users/bom_d_van/Code/c/lcthw/src/darray.o reader.o request.o request_test.o -o request_test.bin

trie:
	# $(CC) request.c -o request.o
	$(CC) trie.c -o trie.o
	$(CC) trie_test.c -o trie_test.o
	# $(CC) request.c -o request.o
	cc /Users/bom_d_van/Code/c/lcthw/src/darray.o trie.o trie_test.o -o trie_test.bin

chttp:
	$(CC) reader.c -o reader.o
	$(CC) request.c -o request.o
	$(CC) trie.c -o trie.o
	$(CC) chttp.c -o chttp.o
	$(CC) chttp_test.c -o chttp_test.o
	cc /Users/bom_d_van/Code/c/lcthw/src/darray.o reader.o request.o trie.o chttp.o chttp_test.o -o chttp_test.bin
