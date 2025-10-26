#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include "lexer_trie.h"

#define MAX_TOKEN_LEN 64
#define MAX_OPERATOR_LEN 16
#define MAX_STRING_LITERAL_LEN 256

#define CHAR_LITERAL_LEN 4

#define INITIAL_CAPACITY 32


typedef struct Lexer {
    const char* source;
    int position;
    int line;
    int column;

    TrieNode* keywords_trie;
    TrieNode* operator_trie;
} Lexer;

Token make_token(TokenType type, const char* lexeme, int line, int column);
void free_token(Token* token);

Tokens* create_tokens();
void add_token(Tokens* tokens, Token token);
void free_tokens(Tokens* tokens);

void init_lexer(Lexer* lexer, const char* source);
void cleanup_lexer(Lexer* lexer);
Tokens* tokenize(Lexer* lexer, const char* source, int debug);

Token lex_number(Lexer* lexer);
Token lex_string_literal(Lexer* lexer);
Token lex_char_literal(Lexer* lexer);
Token lex_next_token(Lexer* lexer);
Token lex_identifier_or_keyword(Lexer* lexer, TrieNode* keyword_trie);
Token lex_operator_trie(Lexer* lexer, TrieNode* trie_root);
TrieMatch trie_match(TrieNode* root, Lexer* lexer);
void skip_whitespaces(Lexer* lexer);
void skip_whitespace_and_comments(Lexer* lexer);
void skip_block_comment(Lexer* lexer);
void skip_line_comment(Lexer* lexer);
TrieMatch trie_match(TrieNode* root, Lexer* lexer);


char peek(Lexer* lexer);
char peek_ahead(Lexer* lexer, int offset);
char get(Lexer* lexer);

const char* token_type_name(TokenType type);

#endif