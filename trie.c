#include <stdlib.h>
#include <string.h>
#include "trie.h"

TrieNode *TrieNode_find(TrieNode *node, char *key)
{
	int count = strlen(key);
	for (int i = 0; i < count; i++) {
		node = node->children[(int)key[i]];
		if (node == NULL) {
			return NULL;
		}
	}
	return node;
}

TrieNode *TrieNode_new()
{
	TrieNode *node = calloc(1, sizeof(TrieNode));
	node->children = calloc(256, sizeof(TrieNode*));
	return node;
}

// TODO: insert existed node?
int TrieNode_insert(TrieNode *root, char *key, handler handler)
{
	TrieNode *node = root;
	int i = 0;
	int n = strlen(key);

	while (i < n) {
		if (node->children[(int)key[i]]) {
			node = node->children[(int)key[i]];
			i++;
		} else {
			break;
		}
	}

	while (i < n) {
		node->children[(int)key[i]] = TrieNode_new();
		node = node->children[(int)key[i]];
		i++;
	}

	node->handler = handler;
	return 0;
}
