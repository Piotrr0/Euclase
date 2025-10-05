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
    {"uchar",    TOK_UCHAR},
    {"if",       TOK_IF},
    {"else",     TOK_ELSE},
    {NULL,       TOK_IDENTIFIER}
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

Token make_token(TokenType type, const char* text, ValueType vtype) {
    Token t;
    memset(&t, 0, sizeof(t));
    
    t.type = type;
    t.text = text ? strdup(text) : NULL;
    t.value.type = vtype;
    return t;
}

void free_token(Token* token) {
    if (token && token->text) {
        free(token->text);
        token->text = NULL;
    }
}

Token make_int_token(int value) {
    Token t = make_token(TOK_NUMBER_INT, NULL, VAL_INT);
    t.value.int_val = value;
    return t;
}

Token make_float_token(float value) {
    Token t = make_token(TOK_NUMBER_FLOAT, NULL, VAL_FLOAT);
    t.value.float_val = value;
    return t;
}

Token make_double_token(double value) {
    Token t = make_token(TOK_NUMBER_DOUBLE, NULL, VAL_DOUBLE);
    t.value.double_val = value;
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
}

void skip_whitespaces(Lexer* lexer) {
    while (peek(lexer) == ' ' || peek(lexer) == '\t' || peek(lexer) == '\n' || peek(lexer) == '\r') {
        get(lexer);
    }
}

Token lex_symbol(Lexer* lexer) {
    char c = get(lexer);
    TokenType type = lookup_for_symbol(c);
    return make_token(type, NULL, VAL_NONE);
}

Token lex_number(Lexer* lexer) {
    char buffer[MAX_TOKEN_LEN];
    int i = 0;
    int has_dot = 0;

    while ((isdigit(peek(lexer)) || peek(lexer) == '.') && i < MAX_TOKEN_LEN - 1) {
        if (peek(lexer) == '.') {
            if (has_dot) break;
            has_dot = 1;
        }
        buffer[i++] = get(lexer);
    }
    buffer[i] = '\0';

    char suffix = peek(lexer);
    if (suffix == 'f' || suffix == 'd') get(lexer);

    if (!has_dot && suffix != 'f' && suffix != 'd') 
        return make_int_token(atoi(buffer));
    else if (suffix == 'f') 
        return make_float_token((float)atof(buffer));
    else if (suffix == 'd')
        return make_double_token(atof(buffer));

    return make_token(TOK_ERROR, NULL, VAL_NONE);
}

Token lex_keyword(Lexer* lexer) {
    char buffer[MAX_TOKEN_LEN];
    int i = 0;

    while ((isalnum(peek(lexer)) || peek(lexer) == '_') && i < MAX_TOKEN_LEN - 1) {
        buffer[i++] = get(lexer);
    }
    buffer[i] = '\0';

    TokenType type = lookup_for_keyword(buffer);
    return make_token(type, buffer, VAL_NONE);
}

Token lex_next_token(Lexer* lexer) {
    skip_whitespaces(lexer);
    char c = peek(lexer);

    if (c == '\0') return make_token(TOK_EOF, NULL, VAL_NONE);
    
    if (c == '=' && peek_ahead(lexer, 1) == '=')
    {
        get(lexer); get(lexer);
        return make_token(TOK_EQUAL, "==", VAL_NONE);
    }

    if (c == '!' && peek_ahead(lexer, 1) == '=')
    {
        get(lexer); get(lexer);
        return make_token(TOK_NOT_EQUAL, "!=", VAL_NONE);
    }

    if (lookup_for_symbol(c) != TOK_ERROR)
        return lex_symbol(lexer);

    if (isdigit(c))
        return lex_number(lexer);

    if (isalpha(c) || c == '_')
        return lex_keyword(lexer);

    get(lexer);
    return make_token(TOK_ERROR, NULL, VAL_NONE);
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
            if (token.text != NULL) {
                printf("  (lexeme: %s)", token.text);
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
    return lexer->source[lexer->position++];
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