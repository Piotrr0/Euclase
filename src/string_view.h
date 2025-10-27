#ifndef STRING_VIEW_H
#define STRING_VIEW_H

#include <stdio.h>

typedef struct {
    const char* data;
    size_t len;
} StringView;

StringView sv_create(const char* data, size_t len);
StringView sv_from_cstr(const char* cstr);

#endif