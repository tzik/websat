
#include "irt/assert.h"

#include "irt/irt.h"

void __assertion_failure(const char* msg, const char* file, int line) {
  trap(msg, file, line);
}
