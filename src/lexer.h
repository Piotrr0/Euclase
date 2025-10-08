#ifndef LEXER_H
#define LEXER_H

typedef enum {
    TOK_LBRACE, 
    TOK_RBRACE, 
    TOK_LPAREN, 
    TOK_RPAREN,
    TOK_COMMA,
    TOK_SEMICOLON,
    TOK_ASSIGNMENT,
    TOK_AMPERSAND,
    TOK_LESS,
    TOK_GREATER,

    TOK_VOID,
    TOK_INT,
    TOK_UINT,
    TOK_FLOAT,
    TOK_UFLOAT,
    TOK_DOUBLE,
    TOK_UDOUBLE,
    TOK_CHAR,
    TOK_UCHAR,
    TOK_IF,
    TOK_ELSE,
    TOK_FOR,
    TOK_WHILE,

    TOK_ADDITION,
    TOK_SUBTRACTION,
    TOK_MULTIPLICATION,
    TOK_DIVISION,
    TOK_MODULO,
    TOK_EQUAL,
    TOK_NOT_EQUAL,

    TOK_IDENTIFIER,
    TOK_NUMBER_INT,
    TOK_NUMBER_FLOAT,
    TOK_NUMBER_DOUBLE,

    TOK_NAMESPACE,
    TOK_RETURN,

    TOK_EOF,
    TOK_ERROR

} TokenType;

typedef enum {
    VAL_INT,
    VAL_FLOAT,
    VAL_DOUBLE,
    VAL_NONE
} ValueType;

typedef struct Value
{
    ValueType type;
    union {
        int int_val;
        float float_val;
        double double_val;
    };

} Value;

typedef struct Token {
    TokenType type;
    char* text;
    Value value;
} Token;

typedef struct Tokens {
    Token* tokens;
    int token_count;
    int capacity;
} Tokens;

typedef struct Lexer {
    const char* source;
    int position;
} Lexer;

typedef struct Keyword {
    const char* name;
    TokenType type;
} Keyword;

typedef struct Symbol {
    char ch;
    TokenType type;
} Symbol;


extern Symbol symbols[];
extern Keyword keywords[];


Token make_token(TokenType type, const char* text, ValueType vtype);
Token make_int_token(int value);
Token make_float_token(float value);
Token make_double_token(double value);
void free_token(Token* token);

Tokens* create_tokens();
void add_token(Tokens* tokens, Token token);
void free_tokens(Tokens* tokens);

TokenType lookup_for_keyword(const char* str);
TokenType lookup_for_symbol(char c);

void init_lexer(Lexer* lexer, const char* source);

Tokens* tokenize(Lexer* lexer, const char* source, int debug);

Token lex_symbol(Lexer* lexer);
Token lex_number(Lexer* lexer);
Token lex_keyword(Lexer* lexer);
Token lex_next_token(Lexer* lexer);

void skip_whitespaces(Lexer* lexer);
char peek(Lexer* lexer);
char peek_ahead(Lexer* lexer, int offset);
char get(Lexer* lexer);

const char* token_type_name(TokenType type);

#endif