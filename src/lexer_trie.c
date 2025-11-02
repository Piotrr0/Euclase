#include "lexer_trie.h"
#include <stdlib.h>

TrieNode* create_trie_node() {
    TrieNode* node = (TrieNode*)calloc(1, sizeof(TrieNode));
    if (node == NULL)
        return NULL;

    node->token_type = TOK_NONE;
    node->is_terminal = 0;
    return node;
}

void trie_insert(TrieNode* root, const char* op_string, TokenType type) {
    TrieNode* current = root;
    
    for (int i = 0; op_string[i] != '\0'; i++) {
        unsigned char ch = op_string[i];
        
        if (current->children[ch] == NULL) {
            current->children[ch] = create_trie_node();
        }
        current = current->children[ch];
    }
    
    current->is_terminal = 1;
    current->token_type = type;
}

TrieNode* build_operator_trie(TrieNode* root) {    
    trie_insert(root, "=", TOK_ASSIGNMENT);
    trie_insert(root, "<", TOK_LESS);
    trie_insert(root, ">", TOK_GREATER);
    trie_insert(root, "+", TOK_ADDITION);
    trie_insert(root, "-", TOK_SUBTRACTION);
    trie_insert(root, "*", TOK_MULTIPLICATION);
    trie_insert(root, "/", TOK_DIVISION);
    trie_insert(root, "%", TOK_MODULO);
    trie_insert(root, "&", TOK_AMPERSAND);
    trie_insert(root, "(", TOK_LPAREN);
    trie_insert(root, ")", TOK_RPAREN);
    trie_insert(root, "{", TOK_LBRACE);
    trie_insert(root, "}", TOK_RBRACE);
    trie_insert(root, ",", TOK_COMMA);
    trie_insert(root, ".", TOK_DOT);
    trie_insert(root, ";", TOK_SEMICOLON);
    
    trie_insert(root, "==", TOK_EQUAL);
    trie_insert(root, "!=", TOK_NOT_EQUAL);
    trie_insert(root, "<=", TOK_LESS_EQUALS);
    trie_insert(root, ">=", TOK_GREATER_EQUALS);
    trie_insert(root, "+=", TOK_ASSIGNMENT_ADDITION);
    trie_insert(root, "-=", TOK_ASSIGNMENT_SUBTRACTION);
    trie_insert(root, "*=", TOK_ASSIGNMENT_MULTIPLICATION);
    trie_insert(root, "/=", TOK_ASSIGNMENT_DIVISION);
    trie_insert(root, "%=", TOK_ASSIGNMENT_MODULO);
    trie_insert(root, "++", TOK_INCREMENT);
    trie_insert(root, "--", TOK_DECREMENT);
    
    return root;
}

TrieNode* build_keyword_trie(TrieNode* root)
{   
    trie_insert(root, "if", TOK_IF);
    trie_insert(root, "else", TOK_ELSE);
    trie_insert(root, "for", TOK_FOR);
    trie_insert(root, "while", TOK_WHILE);
    trie_insert(root, "return", TOK_RETURN);
    
    trie_insert(root, "void", TOK_VOID);
    trie_insert(root, "int", TOK_INT);
    trie_insert(root, "uint", TOK_UINT);
    trie_insert(root, "float", TOK_FLOAT);
    trie_insert(root, "ufloat", TOK_UFLOAT);
    trie_insert(root, "double", TOK_DOUBLE);
    trie_insert(root, "udouble", TOK_UDOUBLE);
    trie_insert(root, "char", TOK_CHAR);
    trie_insert(root, "uchar", TOK_UCHAR);
    
    trie_insert(root, "struct", TOK_STRUCT);
    trie_insert(root, "namespace", TOK_NAMESPACE);
    
    return root;
}

void free_trie(TrieNode* node) {
    if (node == NULL)
        return;
    
    for (int i = 0; i < MAX_CHILDREN; i++) {
        if (node->children[i]) {
            free_trie(node->children[i]);
        }
    }
    free(node);
}