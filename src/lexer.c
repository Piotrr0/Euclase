#include "lexer.h"
#include "string_view.h"
#include "token.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

Token make_token(TokenType type, StringView lexeme, int line, int column) {
    Token t;
    memset(&t, 0, sizeof(t));
    
    t.type = type;
    t.lexeme = lexeme;
    t.line = line;
    t.column = column;
    return t;
}

Tokens* create_tokens() 
{
    Tokens* tokens = malloc(sizeof(Tokens));
    if (tokens == NULL) 
        return NULL;
    
    tokens->tokens = malloc(INITIAL_CAPACITY * sizeof(Token));
    if (tokens->tokens == NULL) {
        free(tokens);
        return NULL;
    }
    
    tokens->token_count = 0;
    tokens->capacity = INITIAL_CAPACITY;
    return tokens;
}

void add_token(Tokens* tokens, Token token) 
{
    if (tokens == NULL) 
        return;
    
    if (tokens->token_count >= tokens->capacity) {
        int new_capacity = tokens->capacity * 2;
        Token* new_tokens = realloc(tokens->tokens, new_capacity * sizeof(Token));
        if (new_tokens == NULL) {
            fprintf(stderr, "Failed to reallocate tokens array\n");
            return;
        }

        tokens->tokens = new_tokens;
        tokens->capacity = new_capacity;
    }
    
    tokens->tokens[tokens->token_count++] = token;
}

void free_tokens(Tokens* tokens) {
    if (tokens == NULL)
        return;
    
    free(tokens->tokens);
    free(tokens);
}

void init_lexer(Lexer* lexer, const char* source) {
    lexer->source = source;
    lexer->position = 0;
    lexer->line = 1;
    lexer->column = 1;

    lexer->keywords_trie = create_trie_node();
    lexer->operator_trie = create_trie_node();
    build_operator_trie(lexer->operator_trie);
    build_keyword_trie(lexer->keywords_trie);
}

void cleanup_lexer(Lexer* lexer) {
    if (lexer->keywords_trie)
    {
        free_trie(lexer->keywords_trie);
        lexer->keywords_trie = NULL;
    }

    if (lexer->operator_trie)
    {
        free_trie(lexer->operator_trie);
        lexer->operator_trie = NULL;
    }
}

void skip_whitespaces(Lexer* lexer) {
    while (peek(lexer) == ' ' || peek(lexer) == '\t' || peek(lexer) == '\n' || peek(lexer) == '\r') {
        get(lexer);
    }
}

Token lex_number(Lexer* lexer) {
    const int line = lexer->line;
    const int col = lexer->column;
    const char* start = &lexer->source[lexer->position];

    int has_dot = 0;
    
    while (isdigit(peek(lexer)) || peek(lexer) == '.') {
        if (peek(lexer) == '.') {
            if (has_dot) break;
            has_dot = 1;
        }
        get(lexer);
    }
    
    char suffix = '\0';
    char next = peek(lexer);
    if (next == 'f' || next == 'F' || next == 'd' || next == 'D') {
        suffix = get(lexer);
    }
    
    size_t length = (size_t)(&lexer->source[lexer->position] - start);
    StringView lexeme = sv_from_parts(start, length);

    TokenType type = TOK_NUMBER_INT;
    if (has_dot || suffix != '\0') {
        if (suffix == 'f' || suffix == 'F')
            type = TOK_NUMBER_FLOAT;
        else
            type = TOK_NUMBER_DOUBLE;
    }

    return make_token(type, lexeme, line, col);
}

Token lex_string_literal(Lexer* lexer) {
    const int line = lexer->line;
    const int col = lexer->column;
    
    get(lexer);
    const char* start = &lexer->source[lexer->position];
    
    while (peek(lexer) != '"' && peek(lexer) != '\0') {
        get(lexer);
    }
    
    if (peek(lexer) == '"') {
        StringView lexeme = sv_from_parts(start, &lexer->source[lexer->position] - start);
        get(lexer);
        return make_token(TOK_STRING_LITERAL, lexeme, line, col);
    }

    return make_token(TOK_ERROR, sv_from_cstr("invalid string literal"), line, col);
}

Token lex_char_literal(Lexer* lexer) {
    const int line = lexer->line;
    const int col = lexer->column;
    
    get(lexer);
    const char* start = &lexer->source[lexer->position];
    
    if (peek(lexer) != '\'' && peek(lexer) != '\0') {
        get(lexer);
    }
    
    if (peek(lexer) == '\'') {
        StringView lexeme = sv_from_parts(start, &lexer->source[lexer->position] - start);
        get(lexer);
        return make_token(TOK_CHAR_LITERAL, lexeme, line, col);
    }

    return make_token(TOK_ERROR, sv_from_cstr("invalid char literal"), line, col);
}

TrieMatch trie_match(TrieNode* root, Lexer* lexer)
{
    TrieMatch result = {TOK_NONE, 0};
    TrieMatch last_valid = {TOK_NONE, 0};
    
    TrieNode* current = root;
    int chars_consumed = 0;
    
    while (1) {
        char c = peek_ahead(lexer, chars_consumed);
        
        if (current->is_terminal) {
            last_valid.type = current->token_type;
            last_valid.length = chars_consumed;
        }
        
        if (c == '\0' || current->children[(unsigned char)c] == NULL) {
            break;
        }
        
        current = current->children[(unsigned char)c];
        chars_consumed++;
    }
    
    if (current->is_terminal) {
        last_valid.type = current->token_type;
        last_valid.length = chars_consumed;
    }
    
    return last_valid;
}

Token lex_operator_trie(Lexer* lexer, TrieNode* trie_root) {
    const int line = lexer->line;
    const int col = lexer->column;
    const char* start = &lexer->source[lexer->position];
    
    TrieMatch match = trie_match(trie_root, lexer);
    
    if (match.type == TOK_NONE || match.length == 0) {
        get(lexer);
        return make_token(TOK_ERROR, sv_from_parts(start, 1), line, col);
    }

    for (int i = 0; i < match.length; i++) {
        get(lexer);
    }
    
    StringView lexeme = sv_from_parts(start, match.length);
    return make_token(match.type, lexeme, line, col);
}

Token lex_identifier_or_keyword(Lexer* lexer, TrieNode* keyword_trie) {
    const int line = lexer->line;
    const int col = lexer->column;
    const char* start = &lexer->source[lexer->position];

    while (isalnum(peek(lexer)) || peek(lexer) == '_') {
        get(lexer);
    }
    
    StringView lexeme = sv_from_parts(start, &lexer->source[lexer->position] - start);

    TrieNode* current = keyword_trie;
    int matched_length = 0;

    for (size_t j = 0; j < lexeme.length; j++)
    {
        unsigned char ch = (unsigned char)lexeme.data[j];
        if (current->children[ch] == NULL) {
            break;
        }
        current = current->children[ch];
        matched_length++;
    }
    
    if (matched_length == lexeme.length && current->is_terminal) {
        return make_token(current->token_type, lexeme, line, col);
    }
    
    return make_token(TOK_IDENTIFIER, lexeme, line, col);
}

Token lex_next_token(Lexer* lexer) {
    skip_whitespace_and_comments(lexer);
    const int line = lexer->line;
    const int col = lexer->column;
    char c = peek(lexer);
    
    if (c == '\0') {
        StringView empty = sv_from_parts(NULL, 0);
        return make_token(TOK_EOF, empty, line, col);
    }

    if (c == '"')
        return lex_string_literal(lexer);

    if (c == '\'')
        return lex_char_literal(lexer);

    if (isdigit(c))
        return lex_number(lexer);

    if (isalpha(c) || c == '_')
        return lex_identifier_or_keyword(lexer, lexer->keywords_trie);

    return lex_operator_trie(lexer, lexer->operator_trie);
}

void skip_line_comment(Lexer* lexer) {
    if (peek(lexer) == '/' && peek_ahead(lexer, 1) == '/')
    {
        while (peek(lexer) != '\n' && peek(lexer) != '\0') {
            get(lexer);
        }
    }
}

void skip_block_comment(Lexer* lexer)
{
    if (peek(lexer) == '/' && peek_ahead(lexer, 1) == '*') {
        get(lexer); get(lexer);

        while (peek(lexer) != '\0')
        {
            if (peek(lexer) == '*' && peek_ahead(lexer, 1) != '\0' && peek_ahead(lexer, 1) == '/') {
                get(lexer); get(lexer);
                return;
            }
            get(lexer);
        }
    }
}

void skip_whitespace_and_comments(Lexer* lexer) {
    int changed;
    do {
        changed = 0;
        int before = lexer->position;

        skip_whitespaces(lexer);
        skip_line_comment(lexer);
        skip_block_comment(lexer);

        if (lexer->position != before)
            changed = 1;
    } while (changed);
}

Tokens* tokenize(Lexer* lexer, const char* source, int debug)
{
    init_lexer(lexer, source);

    Tokens* tokens = create_tokens();
    if (tokens == NULL) 
        return NULL;

    int generate_tokens = 1;
    while (generate_tokens)
    {
        Token token = lex_next_token(lexer);
        if(debug)
        {
            printf("lexer: current token: %s", token_type_name(token.type));
            if (token.lexeme.data != NULL && token.lexeme.length > 0) {
                printf("  (lexeme: %.*s)", (int)token.lexeme.length, token.lexeme.data);
            }
            printf("\n");
        }

        add_token(tokens, token);
        if (token.type == TOK_EOF || token.type == TOK_ERROR)
            generate_tokens = 0;
    }    
    return tokens;
}

char peek_ahead(Lexer* lexer, int offset)
{
    return lexer->source[lexer->position + offset];
}

char peek(Lexer* lexer)
{
    return lexer->source[lexer->position];
}

char get(Lexer* lexer)
{
    const char c = lexer->source[lexer->position++];
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    }
    else
        lexer->column++;

    return c;
}

const char* token_type_name(TokenType type) {
    switch(type) {
        case TOK_LBRACE:        return "LBRACE";
        case TOK_RBRACE:        return "RBRACE";
        case TOK_LPAREN:        return "LPAREN";
        case TOK_RPAREN:        return "RPAREN";
        case TOK_COMMA:         return "COMMA";
        case TOK_DOT:           return "DOT";
        case TOK_SEMICOLON:     return "SEMICOLON";
        case TOK_ASSIGNMENT:    return "ASSIGNMENT";
        case TOK_LESS:          return "LESS";
        case TOK_GREATER:       return "GREATER";
        case TOK_AMPERSAND:     return "AMPERSAND";
        case TOK_INCREMENT:     return "INCREMENT";
        case TOK_DECREMENT:     return "DECREMENT";

        case TOK_ADDITION:      return "ADDITION";
        case TOK_SUBTRACTION:   return "SUBTRACTION";
        case TOK_MULTIPLICATION:return "MULTIPLICATION";
        case TOK_DIVISION:      return "DIVISION";
        case TOK_MODULO:        return "MODULO";
        case TOK_EQUAL:         return "EQUAL";
        case TOK_NOT_EQUAL:     return "NOT_EQUAL";

        case TOK_ASSIGNMENT_ADDITION:         return "ASSIGNMENT_ADDITION";
        case TOK_ASSIGNMENT_SUBTRACTION:      return "ASSIGNMENT_SUBTRACTION";
        case TOK_ASSIGNMENT_MULTIPLICATION:   return "ASSIGNMENT_MULTIPLICATION";
        case TOK_ASSIGNMENT_DIVISION:         return "ASSIGNMENT_DIVISION";
        case TOK_ASSIGNMENT_MODULO:           return "ASSIGNMENT_MODULO";

        case TOK_STRING_LITERAL:return "TOK_STRING_LITERAL";
        case TOK_CHAR_LITERAL:  return "TOK_CHAR_LITERAL";            

        case TOK_VOID:          return "VOID";
        case TOK_INT:           return "INT";
        case TOK_UINT:          return "UINT";
        case TOK_FLOAT:         return "FLOAT";
        case TOK_UFLOAT:        return "UFLOAT";
        case TOK_DOUBLE:        return "DOUBLE";
        case TOK_UDOUBLE:       return "UDOUBLE";
        case TOK_CHAR:          return "CHAR";
        case TOK_UCHAR:         return "UCHAR";
        case TOK_IF:            return "IF";
        case TOK_ELSE:          return "ELSE";
        case TOK_FOR:           return "FOR";
        case TOK_WHILE:         return "WHILE";
        case TOK_STRUCT:        return "STRUCT";

        case TOK_IDENTIFIER:    return "IDENTIFIER";
        case TOK_NUMBER_INT:    return "NUMBER_INT";
        case TOK_NUMBER_FLOAT:  return "NUMBER_FLOAT";
        case TOK_NUMBER_DOUBLE: return "NUMBER_DOUBLE";

        case TOK_NAMESPACE:     return "NAMESPACE";
        case TOK_RETURN:        return "RETURN";
        
        case TOK_EOF:           return "EOF";
        case TOK_ERROR:         return "ERROR";

        default:                return "UNKNOWN";
    }
}