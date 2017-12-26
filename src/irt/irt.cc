
#include "irt/irt.h"

#include "irt/env.h"
#include "irt/ffi.h"
#include "irt/types.h"

int puts(const char* str) {
  print(str, strlen(str));
  return 0;
}

size_t strlen(const char* str) {
  int len = 0;
  while (str[len])
    ++len;
  return len;
}

void* memcpy(void* dest, const void* src, size_t n) {
  char* d = static_cast<char*>(dest);
  const char* s = static_cast<const char*>(src);
  for (size_t i = 0; i < n; ++i)
    d[i] = s[i];
  return dest;
}

void* memset(void* s, int c, size_t n) {
  unsigned char* d = static_cast<unsigned char*>(s);
  for (size_t i = 0; i < n; ++i)
    d[i] = c;
  return s;
}

void trap(const char* msg,
          const char* filename,
          int line_number) {
  throwError(msg, strlen(msg), filename, strlen(filename), line_number);
}

void trap(const char* msg) {
  throwError(msg, strlen(msg), nullptr, 0, 0);
}

void* operator new(size_t size) {
  void* p = malloc(size);
  if (p)
    return p;

#if __has_feature(cxx_exceptions)
  throw std::bad_alloc();
#else
  trap("OOM");
#endif
}

void operator delete(void* p) noexcept {
  return free(p);
}

void* operator new(size_t, void* p) {
  return p;
}
