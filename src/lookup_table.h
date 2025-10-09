#ifndef LOOKUP_TABLE_H
#define LOOKUP_TABLE_H

#include "lexer.h"
#include <llvm-c/Types.h>

#define HASH_TABLE_SIZE 32
#define MAX_SCOPE_DEPTH 64

typedef struct SymbolEntry {
    char* name;
    TokenType base_type;
    int pointer_level;
    int is_global;
    LLVMValueRef alloc;
    struct SymbolEntry* next;
} SymbolEntry;

typedef struct HashTable {
    SymbolEntry* buckets[HASH_TABLE_SIZE];
} HashTable;

typedef struct Scope {
    HashTable* table;
    struct Scope* parent;
    int depth;
} Scope;

typedef struct SymbolTable {
    Scope* scopes[MAX_SCOPE_DEPTH];
    Scope* current_scope;
    int scope_count;
} SymbolTable;

SymbolTable* init_symbol_table();
HashTable* create_hash_table();
Scope* create_scope(Scope* parent, int depth);


#endif