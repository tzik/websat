#pragma once

#include "irt/ffi.h"
#include "irt/types.h"

EXPORT void* malloc(size_t);
EXPORT void free(void*);
EXPORT void* realloc(void*, size_t);
