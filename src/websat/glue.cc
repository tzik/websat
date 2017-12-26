
#include "irt/ffi.h"
#include "irt/irt.h"

#include "minisat/core/Solver.h"

Minisat::Solver* g_solver = nullptr;
EXPORT void init() {
  g_solver = new Minisat::Solver;
}

EXPORT int32_t newLiteral() {
  return Minisat::toInt(g_solver->newVar()) + 1;
}

EXPORT void addClause(int32_t* clause, size_t length) {
  Minisat::vec<Minisat::Lit> c;
  for (size_t i = 0; i < length; ++i) {
    int32_t x = clause[i];
    c.push(Minisat::mkLit((x < 0 ? -x : x) - 1, x < 0));
  }
  g_solver->addClause(c);
}

EXPORT void reset() {
  delete g_solver;
  g_solver = new Minisat::Solver;
}

EXPORT bool solve() {
  return g_solver->solve();
}

EXPORT size_t getNVars() {
  return g_solver->nVars();
}

EXPORT void extract(uint8_t* buf, size_t length) {
  for (size_t i = 0; i < length; ++i)
    buf[i] = toInt(g_solver->modelValue(i));
}
