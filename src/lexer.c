#include "lexer.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_TOKEN_LEN 64
#define INITIAL_CAPACITY 32

Keyword keywords[] = {
    {"return",    TOK_RETURN},
    {"namespace", TOK_NAMESPACE},
    {"void",      TOK_VOID},
    {"int",       TOK_INT},
    {"uint",      TOK_UINT},
    {"float",     TOK_FLOAT},
    {"ufloat",    TOK_UFLOAT},
    {"double",    TOK_DOUBLE},
    {"udouble",   TOK_UDOUBLE},
    {"char",      TOK_CHAR},
    {"uchar",     TOK_UCHAR},
    {"if",        TOK_IF},
    {"else",      TOK_ELSE},
    {"for",       TOK_FOR},
    {"while",     TOK_WHILE},
    {NULL,        TOK_IDENTIFIER}
};

Symbol symbols[] = {
    {'{', TOK_LBRACE},
    {'}', TOK_RBRACE},
    {'(', TOK_LPAREN},
    {')', TOK_RPAREN},
    {',', TOK_COMMA},
    {';', TOK_SEMICOLON},
    {'=', TOK_ASSIGNMENT},
    {'<', TOK_LESS},
    {'>', TOK_GREATER},
    {'&', TOK_AMPERSAND},

    {'+', TOK_ADDITION},
    {'-', TOK_SUBTRACTION},
    {'*', TOK_MULTIPLICATION},
    {'/', TOK_DIVISION},
    {'%', TOK_MODULO},

    {'\0',TOK_EOF},
};

Token make_token(TokenType type, const char* lexeme, int line, int column) {
    Token t;
    memset(&t, 0, sizeof(t));
    
    t.type = type;
    t.lexme = lexeme ? strdup(lexeme) : NULL;
    t.line = line;
    t.column = column;
    return t;
}

void free_token(Token* token) {
    if (token && token->lexme) {
        free(token->lexme);
        token->lexme = NULL;
    }
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
    
    for (int i = 0; i < tokens->token_count; i++) {
        free_token(&tokens->tokens[i]);
    }
    
    free(tokens->tokens);
    free(tokens);
}

TokenType lookup_for_symbol(char c) {
    for (int i = 0; symbols[i].ch; i++) {
        if (symbols[i].ch == c)
            return symbols[i].type;
    }
    return TOK_ERROR;
}

TokenType lookup_for_keyword(const char* str) {
    for (int i = 0; keywords[i].name; i++) {
        if (strcmp(str, keywords[i].name) == 0)
            return keywords[i].type;
    }
    return TOK_IDENTIFIER;
}

void init_lexer(Lexer* lexer, const char* source) {
    lexer->source = source;
    lexer->position = 0;
    lexer->line = 1;
    lexer->column = 1;
}

void skip_whitespaces(Lexer* lexer) {
    while (peek(lexer) == ' ' || peek(lexer) == '\t' || peek(lexer) == '\n' || peek(lexer) == '\r') {
        get(lexer);
    }
}

Token lex_symbol(Lexer* lexer) {
    const int line = lexer->line;
    const int col = lexer->column;

    char c = get(lexer);
    TokenType type = lookup_for_symbol(c);
    return make_token(type, NULL, line, col);
}

Token lex_number(Lexer* lexer) {
    const int line = lexer->line;
    const int col = lexer->column;

    char buffer[MAX_TOKEN_LEN];
    int i = 0;
    int has_dot = 0;
    int has_suffix = 0;
    char suffix = '\0';

    while ((isdigit(peek(lexer)) || peek(lexer) == '.') && i < MAX_TOKEN_LEN - 1) {
        if (peek(lexer) == '.') {
            if (has_dot) break;
            has_dot = 1;
        }
        buffer[i++] = get(lexer);
    }
    buffer[i] = '\0';

    char next = peek(lexer);
    if (next == 'f' || next == 'F' || next == 'd' || next == 'D') {
        suffix = next;
        has_suffix = 1;
        buffer[i++] = get(lexer);
    }

    buffer[i] = '\0';

    TokenType type;
    
    if (has_dot || has_suffix) {
        if (suffix == 'f')
            type = TOK_NUMBER_FLOAT;
        else if (suffix == 'd')
            type = TOK_NUMBER_DOUBLE;
        else if (has_dot)
            type = TOK_NUMBER_DOUBLE;
    } 
    else {
        type = TOK_NUMBER_INT;
    }

    return make_token(type, buffer, line, col);
}

Token lex_keyword(Lexer* lexer) {
    const int line = lexer->line;
    const int col = lexer->column;

    char buffer[MAX_TOKEN_LEN];
    int i = 0;

    while ((isalnum(peek(lexer)) || peek(lexer) == '_') && i < MAX_TOKEN_LEN - 1) {
        buffer[i++] = get(lexer);
    }
    buffer[i] = '\0';

    TokenType type = lookup_for_keyword(buffer);
    return make_token(type, buffer, line, col);
}

Token lex_next_token(Lexer* lexer) {
    skip_whitespace_and_comments(lexer);
    const int line = lexer->line;
    const int col = lexer->column;
    char c = peek(lexer);

    if (c == '\0') 
        return make_token(TOK_EOF, NULL, line, col);
    
    if (c == '=' && peek_ahead(lexer, 1) == '=') {
        get(lexer); get(lexer);
        return make_token(TOK_EQUAL, "==", line, col);
    }

    if (c == '!' && peek_ahead(lexer, 1) == '=') {
        get(lexer); get(lexer);
        return make_token(TOK_NOT_EQUAL, "!=", line, col);
    }

    if (c == '<' && peek_ahead(lexer, 1) == '=') {
        get(lexer); get(lexer);
        return make_token(TOK_LESS_EQUALS, "<=", line, col);
    }

    if (c == '>' && peek_ahead(lexer, 1) == '=') {
        get(lexer); get(lexer);
        return make_token(TOK_GREATER_EQUALS, ">=", line, col);
    }

    if (c == '+' && peek_ahead(lexer, 1) == '=' ) {
        get(lexer); get(lexer);
        return make_token(TOK_ASSIGNMENT_ADDITION, "+=", line, col);
    }

    if (c == '-' && peek_ahead(lexer, 1) == '=' ) {
        get(lexer); get(lexer);
        return make_token(TOK_ASSIGNMENT_SUBTRACTION, "-=", line, col);
    }

    if (c == '*' && peek_ahead(lexer, 1) == '=' ) {
        get(lexer); get(lexer);
        return make_token(TOK_ASSIGNMENT_MULTIPLICATION, "*=", line, col);
    }

    if (c == '/' && peek_ahead(lexer, 1) == '=' ) {
        get(lexer); get(lexer);
        return make_token(TOK_ASSIGNMENT_DIVISION, "/=", line, col);
    }

    if (c == '%' && peek_ahead(lexer, 1) == '=' ) {
        get(lexer); get(lexer);
        return make_token(TOK_ASSIGNMENT_MODULO, "%=", line, col);
    }

    if (lookup_for_symbol(c) != TOK_ERROR)
        return lex_symbol(lexer);

    if (isdigit(c))
        return lex_number(lexer);

    if (isalpha(c) || c == '_')
        return lex_keyword(lexer);

    get(lexer);
    return make_token(TOK_ERROR, NULL, line, col);
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
            if (token.lexme != NULL) {
                printf("  (lexeme: %s)", token.lexme);
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
        case TOK_SEMICOLON:     return "SEMICOLON";
        case TOK_ASSIGNMENT:    return "ASSIGNMENT";
        case TOK_LESS:          return "LESS";
        case TOK_GREATER:       return "GREATER";
        case TOK_AMPERSAND:     return "AMPERSAND";

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