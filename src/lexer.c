#include "lexer.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

Lexer lexer;

#define MAX_TOKEN_LEN 64

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
    {NULL,       TOK_IDENTIFIER}
};

Symbol symbols[] = {
    {'{', TOK_LBRACE},
    {'}', TOK_RBRACE},
    {'(', TOK_LPAREN},
    {')', TOK_RPAREN},
    {';', TOK_SEMICOLON},
    {'=', TOK_ASSIGNMENT},
    {'\0',TOK_EOF}
};

Token make_token(TokenType type, const char* text, ValueType vtype) {
    Token t;
    t.type = type;
    t.text = text ? strdup(text) : NULL;
    t.value_type = vtype;
    memset(&t.value, 0, sizeof(t.value));
    return t;
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

void init_lexer(const char* source) {
    lexer.source = source;
    lexer.position = 0;
}

char peek()
{
    return lexer.source[lexer.position];
}

char get()
{
    return lexer.source[lexer.position++];
}

void skip_whitespaces()
{
    while (peek() == ' ' || peek() == '\t' || peek() == '\n' || peek() == '\r') {
        get();
    }
}

Token lex_symbol() {
    char c = get();
    TokenType type = lookup_for_symbol(c);
    return make_token(type, NULL, VAL_NONE);
}

Token lex_number() {
    char buffer[MAX_TOKEN_LEN];
    int i = 0;
    int has_dot = 0;

    while ((isdigit(peek()) || peek() == '.') && i < MAX_TOKEN_LEN - 1) {
        if (peek() == '.') {
            if (has_dot) break;
            has_dot = 1;
        }
        buffer[i++] = get();
    }
    buffer[i] = '\0';

    char suffix = peek();
    if (suffix == 'f' || suffix == 'd') get();

    if (!has_dot && suffix != 'f' && suffix != 'd') 
        return make_int_token(atoi(buffer));
    else if (suffix == 'f') 
        return make_float_token((float)atof(buffer));
    else if (suffix == 'd')
        return make_double_token(atof(buffer));

    return make_token(TOK_ERROR, NULL, VAL_NONE);
}

Token lex_keyword() {
    char buffer[MAX_TOKEN_LEN];
    int i = 0;

    while ((isalnum(peek()) || peek() == '_') && i < MAX_TOKEN_LEN - 1) {
        buffer[i++] = get();
    }
    buffer[i] = '\0';

    TokenType type = lookup_for_keyword(buffer);
    return make_token(type, buffer, VAL_NONE);
}

Token get_token() {
    skip_whitespaces();
    char c = peek();

    if (lookup_for_symbol(c) != TOK_ERROR)
        return lex_symbol();

    if (isdigit(c))
        return lex_number();

    if (isalpha(c) || c == '_')
        return lex_keyword();

    get();
    return make_token(TOK_ERROR, NULL, VAL_NONE);
}

const char* token_type_name(TokenType type) {
    switch(type) {
        case TOK_LBRACE:        return "LBRACE";
        case TOK_RBRACE:        return "RBRACE";
        case TOK_LPAREN:        return "LPAREN";
        case TOK_RPAREN:        return "RPAREN";
        case TOK_SEMICOLON:     return "SEMICOLON";
        case TOK_ASSIGNMENT:    return "ASSIGNMENT";

        case TOK_VOID:          return "VOID";
        case TOK_INT:           return "INT";
        case TOK_UINT:          return "UINT";
        case TOK_FLOAT:         return "FLOAT";
        case TOK_UFLOAT:        return "UFLOAT";
        case TOK_DOUBLE:        return "DOUBLE";
        case TOK_UDOUBLE:       return "UDOUBLE";
        case TOK_CHAR:          return "CHAR";
        case TOK_UCHAR:         return "UCHAR";

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