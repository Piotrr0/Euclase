#include "lexer.h"
#include <string.h>
#include <stdio.h>

Lexer lexer;

void init_lexer(const char* source)
{
    if(source == NULL)
        printf("source is NULL");

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

Token get_token()
{
    skip_whitespaces();
    const char c = peek();

    switch (c) {
        case '\0': return (Token){TOK_EOF, NULL, 0};
        case '{': get(); return (Token){TOK_LBRACE, NULL, 0};
        case '}': get(); return (Token){TOK_RBRACE, NULL, 0};
        case '(': get(); return (Token){TOK_LPAREN, NULL, 0};
        case ')': get(); return (Token){TOK_RPAREN, NULL, 0};
        case ';': get(); return (Token){TOK_SEMICOLON, NULL, 0}; 
    }

    if (c >= '0' && c <= '9') {
        int value = get() - '0';
        while (peek() >= '0' && peek() <= '9') {
            value = value * 10 + (get() - '0');
        }
        return (Token){TOK_NUMBER, NULL, value};
    }

    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
        char buffer[64];
        int i = 0;
        while ((peek() >= 'a' && peek() <= 'z') ||
               (peek() >= 'A' && peek() <= 'Z') ||
               (peek() >= '0' && peek() <= '9') ||
               peek() == '_') {
            buffer[i++] = get();
        }
        buffer[i] = '\0';

        if (strcmp(buffer, "return") == 0)      return (Token){TOK_RETURN, strdup(buffer), 0};
        if (strcmp(buffer, "namespace") == 0)   return (Token){TOK_NAMESPACE, strdup(buffer), 0};
        if (strcmp(buffer, "void") == 0)        return (Token){TOK_VOID, strdup(buffer), 0};
        if (strcmp(buffer, "int") == 0)         return (Token){TOK_INT, strdup(buffer), 0};
        if (strcmp(buffer, "uint") == 0)        return (Token){TOK_UINT, strdup(buffer), 0};
        if (strcmp(buffer, "float") == 0)       return (Token){TOK_FLOAT, strdup(buffer), 0};
        if (strcmp(buffer, "ufloat") == 0)      return (Token){TOK_UFLOAT, strdup(buffer), 0};
        if (strcmp(buffer, "double") == 0)      return (Token){TOK_DOUBLE, strdup(buffer), 0};
        if (strcmp(buffer, "udouble") == 0)     return (Token){TOK_UDOUBLE, strdup(buffer), 0};
        if (strcmp(buffer, "char") == 0)        return (Token){TOK_CHAR, strdup(buffer), 0};
        if (strcmp(buffer, "uchar") == 0)       return (Token){TOK_UCHAR, strdup(buffer), 0};

        return (Token){TOK_IDENTIFIER, strdup(buffer), 0};
    }

    get();
    return (Token){TOK_ERROR, NULL, 0};
}

const char* token_type_name(TokenType type) {
    switch(type) {
        case TOK_LBRACE:        return "LBRACE";
        case TOK_RBRACE:        return "RBRACE";
        case TOK_LPAREN:        return "LPAREN";
        case TOK_RPAREN:        return "RPAREN";
        case TOK_SEMICOLON:     return "SEMICOLON";

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
        case TOK_NUMBER:        return "NUMBER";

        case TOK_NAMESPACE:     return "NAMESPACE";
        case TOK_RETURN:        return "RETURN";
        
        case TOK_EOF:           return "EOF";
        case TOK_ERROR:         return "ERROR";

        default:                return "UNKNOWN";
    }
}