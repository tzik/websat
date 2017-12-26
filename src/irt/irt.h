#pragma once

#include "irt/types.h"
#include "irt/malloc.h"

extern "C" {
int puts(const char*);
size_t strlen(const char*);
void* memcpy(void*, const void*, size_t);
void* memset(void*, int, size_t);
}

[[noreturn]] void trap(const char* msg, const char* filename, int line_number);
[[noreturn]] void trap(const char* msg);

#if __has_feature(cxx_exceptions)
namespace std {
class bad_alloc {};
}
#endif

void* operator new(size_t size, void*);

void* operator new(size_t size);
void operator delete(void* p) noexcept;
