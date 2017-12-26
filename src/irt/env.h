#pragma once

#include "irt/ffi.h"
#include "irt/types.h"

IMPORT void print(const char* str, size_t length);
IMPORT double pow(double, double);

[[noreturn]] IMPORT void throwError(const char* msg,
                                    size_t msg_length,
                                    const char* filename,
                                    size_t filename_length,
                                    int line_number);
