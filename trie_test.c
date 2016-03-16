#include <stdio.h>
#include "trie.h"

int handler_home(Request *r) {
	// printf("calling home\n");
	return 0;
}

int handler_test(Request *r) {
	// printf("calling test\n");
	return 0;
}

int handler_test_hello(Request *r) {
	// printf("calling test_hello\n");
	return 0;
}

int handler_test2_hello(Request *r) {
	// printf("calling test2_hello\n");
	return 0;
}

int main(int argc, char const *argv[])
{
	TrieNode *tree = TrieNode_new();
	TrieNode_insert(tree, "/", handler_home);
	TrieNode_insert(tree, "/test", handler_test);
	TrieNode_insert(tree, "/test/hello", handler_test_hello);
	TrieNode_insert(tree, "/test2/hello", handler_test2_hello);

	TrieNode *node = TrieNode_find(tree, "/");
	if (node->handler == handler_home) {
		Request r;
		node->handler(NULL);
	} else {
		return 1;
	}
	node = TrieNode_find(tree, "/test2/hello");
	if (node->handler == handler_test2_hello) {
		Request r;
		node->handler(NULL);
	} else {
		return 1;
	}
	node = TrieNode_find(tree, "/null");
	if (node != NULL) {
		return 1;
	}
	return 0;
}
