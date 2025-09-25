#include <string.h>
#include <stdio.h>

typedef enum {
    TOK_LBRACE, 
    TOK_RBRACE, 
    TOK_LPAREN, 
    TOK_RPAREN,
    TOK_SEMICOLON,

    TOK_NUMBER,

    TOK_NAMESPACE,
    TOK_MAIN,
    TOK_RETURN,
    TOK_IDENTIFIER,

    TOK_EOF,
    TOK_ERROR

} TokenType;

typedef struct {
    TokenType type;
    char* text;
    int value;
} Token;


typedef struct {
    const char* source;
    int position;
} Lexer;

Lexer lexer;
void init_lexer(const char* source)
{
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

    if (c == '\0') return (Token){TOK_EOF, NULL, 0};

    if (c == '{') { get(); return (Token){TOK_LBRACE, NULL, 0}; }
    if (c == '}') { get(); return (Token){TOK_RBRACE, NULL, 0}; }
    if (c == '(') { get(); return (Token){TOK_LPAREN, NULL, 0}; }
    if (c == ')') { get(); return (Token){TOK_RPAREN, NULL, 0}; }
    if (c == ';') { get(); return (Token){TOK_SEMICOLON, NULL, 0}; }

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

        if (strcmp(buffer, "return") == 0) return (Token){TOK_RETURN, NULL, 0};
        if (strcmp(buffer, "namespace") == 0) return (Token){TOK_NAMESPACE, NULL, 0};

        return (Token){TOK_IDENTIFIER, strdup(buffer), 0};
    }

    get();
    return (Token){TOK_EOF, NULL, 0};
}

const char* token_type_name(TokenType type) {
    switch(type) {
        case TOK_LBRACE: return "LBRACE";
        case TOK_RBRACE: return "RBRACE";
        case TOK_LPAREN: return "LPAREN";
        case TOK_RPAREN: return "RPAREN";
        case TOK_SEMICOLON: return "SEMICOLON";
        case TOK_NUMBER: return "NUMBER";
        case TOK_NAMESPACE: return "NAMESPACE";
        case TOK_RETURN: return "RETURN";
        case TOK_IDENTIFIER: return "IDENTIFIER";
        default: return "UNKNOWN";
    }
}

int main()
{
    const char* code = 
    "namespace main {" 
    "   int main() {"
    "       return 0;"
    "   }" 
    "}";
    
    init_lexer(code);

    Token tok;
    do {
        tok = get_token();
        printf("Token: %s, text: %s, value: %d\n", token_type_name(tok.type), tok.text, tok.value);
    } while (tok.type != TOK_EOF);

    return 0;
}