#ifndef _trie_h
#define _trie_h

#include "request.h"

// typedef struct Trie {
// 	TrieNode *root;
// } Trie;

typedef struct TrieNode {
	// char *url;
	handler handler;
	struct TrieNode **children;
} TrieNode;

TrieNode *TrieNode_find(TrieNode *node, char *key);
TrieNode *TrieNode_new();
int TrieNode_insert(TrieNode *root, char *key, handler *handler);

#endif
