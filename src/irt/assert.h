#pragma once

extern "C" {
[[noreturn]] void __assertion_failure(const char*, const char*, int);
}

#if defined(NDEBUG)
#define assert(x) ((void)0)
#else
#define assert(x) ((x) ? ((void)0) : __assertion_failure(#x, __FILE__, __LINE__))
#endif
