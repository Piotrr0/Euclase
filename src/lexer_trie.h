#ifndef LEXER_TRIE_H
#define LEXER_TRIE_H

#include "token.h"

#define MAX_CHILDREN 128

typedef struct TrieNode {
    TokenType token_type;
    struct TrieNode* children[MAX_CHILDREN];
    int is_terminal;
} TrieNode;

typedef struct {
    TokenType type;
    int length;
} TrieMatch;

TrieNode* create_trie_node();
void trie_insert(TrieNode* root, const char* op_string, TokenType type);

TrieNode* build_operator_trie(TrieNode* root);
TrieNode* build_keyword_trie(TrieNode* root);
void free_trie(TrieNode* node);

#endif