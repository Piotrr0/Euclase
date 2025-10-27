#include "string_view.h"
#include <string.h>
#include <stdio.h>

StringView sv_create(const char* data, size_t len) {
    StringView sv;
    sv.data = data;
    sv.len = len;
    return sv;
}

StringView sv_from_cstr(const char* cstr)
{
    StringView sv;
    if (cstr != NULL) {
        sv.data = cstr;
        sv.len = strlen(cstr);
    } 
    else {
        sv.data = NULL;
        sv.len = 0;
    }
    return sv;
}


void sv_print(StringView sv) {
    if (sv.data == NULL)
        return;

    printf("%s", sv.data);
}