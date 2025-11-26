#ifndef CODEGEN_BINARY_UNARY_VISITOR_H
#define CODEGEN_BINARY_UNARY_VISITOR_H

#include <llvm-c/Core.h>
#include <llvm-c/Types.h>

int does_type_kind_match(LLVMValueRef left, LLVMValueRef right, LLVMTypeKind* out_kind);

#endif