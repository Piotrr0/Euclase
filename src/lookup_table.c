#include "lookup_table.h"
#include <stdlib.h>

SymbolTable* init_symbol_table() {
    SymbolTable* st = (SymbolTable*)malloc(sizeof(SymbolTable));
    st->scope_count = 0;

    Scope* global_scope = create_scope(NULL, 0);
    st->scopes[st->scope_count++] = global_scope;
    st->current_scope = global_scope;

    return st;
}

HashTable* create_hash_table() {
    HashTable* table = (HashTable*)malloc(sizeof(HashTable));
    return table;
}

Scope* create_scope(Scope* parent, int depth)
{
    Scope* scope = (Scope*)malloc(sizeof(Scope));
    scope->table = create_hash_table();
    scope->parent = parent;
    scope->depth = depth;
    return scope;
}