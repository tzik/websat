#include "irt/malloc.h"

#include "irt/assert.h"
#include "irt/env.h"
#include "irt/irt.h"
#include "irt/types.h"

namespace {

using page_count_t = intptr_t;

enum class ChunkState {
  Special = 0,
  Free = 1,
  InUse = 2,
  InUseLeading = 3,
  Max,
};

struct Chunk;
struct FreeChunk;
struct InUse;
struct InUseLeading;

constexpr uintptr_t kPageSize = 0x10000;  // 64KiB
constexpr int kNumStateBits = 3;
constexpr uintptr_t kAlignment = 8;
constexpr uintptr_t kOverhead = sizeof(uintptr_t);
constexpr page_count_t kInitialPageCount = 1;
constexpr uintptr_t kMinimumAllocation = 3 * sizeof(uintptr_t);
constexpr uintptr_t kStateBitMask = (1u << kNumStateBits) - 1;
static_assert(static_cast<int>(ChunkState::Max) <= (1 << kNumStateBits));
static_assert(kAlignment >= kOverhead);
static_assert(kPageSize % kAlignment == 0);

FreeChunk* g_first_free_chunk = nullptr;
FreeChunk* g_last_free_chunk = nullptr;
Chunk* g_last_chunk = nullptr;

struct Chunk {
  uintptr_t data;

  ChunkState state() const {
    return static_cast<ChunkState>(data & kStateBitMask);
  }

  void set_state(ChunkState s) {
    data = (data & ~kStateBitMask) | static_cast<uintptr_t>(s);
  }

  Chunk* next_chunk() {
    uintptr_t p = data & ~kStateBitMask;
    return p ? reinterpret_cast<Chunk*>(p - kOverhead) : nullptr;
  }

  void set_next_chunk(Chunk* c) {
    assert(c || state() == ChunkState::Special);
    assert(!c || this < c);
    uintptr_t p = c ? reinterpret_cast<uintptr_t>(c) + kOverhead : 0;
    assert(!(p & kStateBitMask));
    data = p | (data & kStateBitMask);
  }

  size_t data_size(){
    uintptr_t n = reinterpret_cast<uintptr_t>(next_chunk());
    return n - reinterpret_cast<uintptr_t>(this + 1);
  }

  bool IsInUse() const {
    return state() == ChunkState::InUse || state() == ChunkState::InUseLeading;
  }

#if !defined(NDEBUG)
  bool IsAligned() const {
    return reinterpret_cast<uintptr_t>(this + 1) % kAlignment == 0;
  }
#endif

  void* ToAllocated() {
    assert(IsInUse());
    assert(IsAligned());
    return this + 1;
  }

  static Chunk* FromAllocated(void* allocated) {
    Chunk* c = static_cast<Chunk*>(allocated) - 1;
    assert(c->IsInUse());
    return c;
  }

  template <typename Dest>
  Dest* As() {
    uintptr_t p = reinterpret_cast<uintptr_t>(&data);
    p -= __builtin_offsetof(Dest, data);

    Dest* res = reinterpret_cast<Dest*>(p);
    assert(state() == Dest::kStateTag);
    assert(&data == &res->data);
    return res;
  }

  template <typename Src>
  static Chunk* From(Src* s) {
    uintptr_t p = reinterpret_cast<uintptr_t>(&s->data);
    p -= __builtin_offsetof(Chunk, data);

    Chunk* res = reinterpret_cast<Chunk*>(p);
    assert(&res->data == &s->data);
    return res;
  }
};

struct InUse {
  static constexpr ChunkState kStateTag = ChunkState::InUse;

  uintptr_t data;
};

struct InUseLeading {
  static constexpr ChunkState kStateTag = ChunkState::InUseLeading;

  FreeChunk* head;
  uintptr_t data;
};

struct FreeChunk {
  static constexpr ChunkState kStateTag = ChunkState::Free;

  static FreeChunk* ConstructAt(Chunk* c,
                                Chunk* n,
                                FreeChunk* pf,
                                FreeChunk* nf) {
    c->set_state(ChunkState::Free);
    c->set_next_chunk(n);

    FreeChunk* f = c->As<FreeChunk>();
    f->set_prev(pf);
    f->set_next(nf);

    if (n == g_last_chunk)
      return f;

    assert(n->IsInUse());
    if (n->state() == ChunkState::InUse)
      n->set_state(ChunkState::InUseLeading);
    n->As<InUseLeading>()->head = f;
    return f;
  }

  void set_prev(FreeChunk* pf) {
    assert(pf < this);
    prev = pf;
    if (pf)
      pf->next = this;
    else
      g_first_free_chunk = this;
  }

  void set_next(FreeChunk* nf) {
    assert(!nf || this < nf);
    next = nf;
    if (nf)
      nf->prev = this;
    else
      g_last_free_chunk = this;
  }

  uintptr_t data;
  FreeChunk* next;
  FreeChunk* prev;
};

page_count_t GrowMemory(page_count_t count) {
  return __builtin_wasm_grow_memory(count);
}

bool Initialize(size_t size) {
  page_count_t delta = (size + kPageSize - 1) / kPageSize;
  if (delta < kInitialPageCount)
    delta = kInitialPageCount;
  page_count_t previous_page_count = GrowMemory(delta);
  if (previous_page_count < 0)
    return false;

  uintptr_t base_position = kPageSize * previous_page_count;
  uintptr_t allocation_size = kPageSize * delta;
  Chunk* first_chunk = reinterpret_cast<Chunk*>(
      base_position + kAlignment - kOverhead);
  g_last_chunk = reinterpret_cast<Chunk*>(
      base_position + allocation_size - kOverhead);

  g_last_chunk->set_state(ChunkState::Special);
  g_last_chunk->set_next_chunk(nullptr);

  FreeChunk::ConstructAt(first_chunk, g_last_chunk, nullptr, nullptr);
  return true;
}

bool GrowMallocHeap(size_t size) {
  page_count_t delta = (size + kPageSize - 1) / kPageSize;
  page_count_t previous_page_count = GrowMemory(delta);
  if (previous_page_count < 0)
    return false;
  assert(reinterpret_cast<uintptr_t>(g_last_chunk + 1) ==
         kPageSize * previous_page_count);

  uintptr_t base_position = kPageSize * previous_page_count;
  uintptr_t allocation_size = kPageSize * delta;

  FreeChunk* pf = g_last_free_chunk;
  Chunk* f = g_last_chunk;
  if (g_last_free_chunk) {
    Chunk* c = Chunk::From(g_last_free_chunk);
    if (c->next_chunk() == g_last_chunk) {
      f = c;
      pf = g_last_free_chunk->prev;
    }
  }
  
  g_last_chunk = reinterpret_cast<Chunk*>(
      base_position + allocation_size - kOverhead);
  g_last_chunk->set_state(ChunkState::Special);
  g_last_chunk->set_next_chunk(nullptr);

  FreeChunk::ConstructAt(f, g_last_chunk, pf, nullptr);
  return true;
}

void* AllocFromFreeList(size_t size) {
  FreeChunk* f = g_first_free_chunk;
  while (f && Chunk::From(f)->data_size() < size)
    f = f->next;
  if (!f)
    return nullptr;

  Chunk* c = Chunk::From(f);
  size_t data_size = c->data_size();
  FreeChunk* pf = f->prev;
  FreeChunk* nf = f->next;
  Chunk* n = c->next_chunk();
  assert(n);
  assert(n == g_last_chunk || n->state() == ChunkState::InUseLeading);

  if (data_size - size >= kMinimumAllocation + kOverhead) {
    Chunk* nc = reinterpret_cast<Chunk*>(
        reinterpret_cast<uintptr_t>(n) - (data_size - size));
    assert(nc->IsAligned());
    FreeChunk* new_f = FreeChunk::ConstructAt(nc, n, pf, nf);
    c->set_next_chunk(Chunk::From(new_f));
  } else {
    if (f->prev)
      f->prev->set_next(f->next);
    if (f->next)
      f->next->set_prev(f->prev);

    if (n != g_last_chunk) {
      assert(n->state() == ChunkState::InUseLeading);
      n->set_state(ChunkState::InUse);
    }
  }

  if (pf && Chunk::From(pf)->next_chunk() == c) {
    c->set_state(ChunkState::InUseLeading);
    c->As<InUseLeading>()->head = pf;
  } else {
    c->set_state(ChunkState::InUse);
  }
  return c->ToAllocated();
}

FreeChunk* SearchForNextUnused(Chunk* c) {
  while (c->IsInUse())
    c = c->next_chunk();
  assert(c == g_last_chunk || c->state() == ChunkState::Free);
  return c != g_last_chunk ? c->As<FreeChunk>() : nullptr;
}

#if !defined(NDEBUG)
void CheckInUseLeadingLink() {
  FreeChunk* f = g_first_free_chunk;
  while (f) {
    Chunk* c = Chunk::From(f)->next_chunk();
    if (c != g_last_chunk) {
      assert(c->state() == ChunkState::InUseLeading);
      assert(c->As<InUseLeading>()->head == f);
    }
    f = f->next;
  }
}

void CheckFreeChunkLink() {
  FreeChunk* f = g_first_free_chunk;
  if (!f) {
    assert(!g_last_free_chunk);
    return;
  }

  assert(!f->prev);
  while (f->next) {
    assert(f->next->prev == f);
    f = f->next;
  }
  assert(g_last_free_chunk == f);
}
#endif

}  // namespace

void* malloc(size_t size) {
  // Round up to `kAlignment * n + kOverhead`.
  size = ((size - kOverhead + kAlignment - 1) & ~(kAlignment - 1)) + kOverhead;
  if (size < kMinimumAllocation)
    size = kMinimumAllocation;

  if (!g_last_chunk && !Initialize(size))
    return nullptr;
  if (void* p = AllocFromFreeList(size))
    return p;
  if (!GrowMallocHeap(size))
    return nullptr;
  return AllocFromFreeList(size);
}

void free(void* p) {
  if (!p)
    return;

  Chunk* c = Chunk::FromAllocated(p);
  assert(c->IsInUse());

  Chunk* n = c->next_chunk();
  FreeChunk* nf;
  FreeChunk* pf;

  if (c->state() == ChunkState::InUseLeading) {
    InUseLeading* u = c->As<InUseLeading>();
    pf = u->head;
    nf = pf->next;
    assert(Chunk::From(pf)->next_chunk() == c);
  } else {
    assert(c->state() == ChunkState::InUse);
    nf = SearchForNextUnused(n);
    pf = nf ? nf->prev : g_last_free_chunk;
  }

  if (pf && Chunk::From(pf)->next_chunk() == c) {
    c = Chunk::From(pf);
    pf = pf->prev;
  }
  if (nf && Chunk::From(nf) == n) {
    n = n->next_chunk();
    nf = nf->next;
  }
    
  FreeChunk::ConstructAt(c, n, pf, nf);

#if !defined(NDEBUG)
  CheckFreeChunkLink();
  CheckInUseLeadingLink();
#endif
}

void* realloc(void* p, size_t size) {
  if (!p)
    return malloc(size);
  
  size_t prev_size = Chunk::FromAllocated(p)->data_size();
  if (prev_size >= size)
    return p;
  void* q = malloc(size);
  if (!q)
    return nullptr;

  memcpy(q, p, prev_size);
  free(p);
  return q;
}
