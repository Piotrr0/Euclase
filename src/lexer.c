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
    {',', TOK_COMMA},
    {';', TOK_SEMICOLON},
    {'=', TOK_ASSIGNMENT},
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

void skip_whitespaces_from_pos(int* pos)
{
    while (peek(pos) == ' ' || peek(pos) == '\t' || peek(pos) == '\n' || peek(pos) == '\r') {
        get(pos);
    }
}

void skip_whitespaces()
{
    skip_whitespaces_from_pos(&lexer.position);
}

Token lex_symbol_from_pos(int* pos) {
    char c = get(pos);
    TokenType type = lookup_for_symbol(c);
    return make_token(type, NULL, VAL_NONE);
}

Token lex_symbol() {
    return lex_symbol_from_pos(&lexer.position);
}

Token lex_number_from_pos(int* pos) {
    char buffer[MAX_TOKEN_LEN];
    int i = 0;
    int has_dot = 0;

    while ((isdigit(peek(pos)) || peek(pos) == '.') && i < MAX_TOKEN_LEN - 1) {
        if (peek(pos) == '.') {
            if (has_dot) break;
            has_dot = 1;
        }
        buffer[i++] = get(pos);
    }
    buffer[i] = '\0';

    char suffix = peek(pos);
    if (suffix == 'f' || suffix == 'd') get(pos);

    if (!has_dot && suffix != 'f' && suffix != 'd') 
        return make_int_token(atoi(buffer));
    else if (suffix == 'f') 
        return make_float_token((float)atof(buffer));
    else if (suffix == 'd')
        return make_double_token(atof(buffer));

    return make_token(TOK_ERROR, NULL, VAL_NONE);
}

Token lex_number() {
    return lex_number_from_pos(&lexer.position);
}

Token lex_keyword_from_pos(int* pos) {
    char buffer[MAX_TOKEN_LEN];
    int i = 0;

    while ((isalnum(peek(pos)) || peek(pos) == '_') && i < MAX_TOKEN_LEN - 1) {
        buffer[i++] = get(pos);
    }
    buffer[i] = '\0';

    TokenType type = lookup_for_keyword(buffer);
    return make_token(type, buffer, VAL_NONE);
}

Token lex_keyword() {
    return lex_keyword_from_pos(&lexer.position);
}

Token debug_return(Token token) {
    printf("lexer: current token: %s", token_type_name(token.type));
    if (token.text != NULL) {
        printf("  (lexeme: %s)", token.text);
    }
    printf("\n");
    return token;
}

Token get_token_from_pos(int* pos) {
    skip_whitespaces_from_pos(pos);
    char c = peek(pos);

    if (c == '\0') return make_token(TOK_EOF, NULL, VAL_NONE);
    if (c == '=' && peek_ahead(pos, 1) == '=') 
    {
        get(pos); get(pos);
        printf("test");
        return make_token(TOK_EQUAL, "==", VAL_NONE);
    }

    if (c == '!' && peek_ahead(pos,1) == '=') 
    {
        get(pos); get(pos);
        return make_token(TOK_NOT_EQUAL, "!=", VAL_NONE);
    }

    if (lookup_for_symbol(c) != TOK_ERROR)
        return lex_symbol_from_pos(pos);

    if (isdigit(c))
        return lex_number_from_pos(pos);

    if (isalpha(c) || c == '_')
        return lex_keyword_from_pos(pos);

    get(pos);
    return make_token(TOK_ERROR, NULL, VAL_NONE);
}

Token get_token() {
    return debug_return(get_token_from_pos(&lexer.position));
}

Token peek_token(int steps) {
    int saved_pos = lexer.position;
    Token token = make_token(TOK_EOF, NULL, VAL_NONE);
    
    for (int i = 0; i < steps; i++) {
        free_token(&token);
        token = get_token_from_pos(&saved_pos);
        if (token.type == TOK_EOF) break;
    }
    
    return token;
}

char peek_ahead(int* pos, int offset)
{
    return lexer.source[*pos + offset];
}

char peek(int* pos)
{
    return lexer.source[*pos];
}

char get(int* pos)
{
    return lexer.source[(*pos)++];
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