#include "heap.h"

#include <cstdint>
#include <random>

Heap::Heap(const size_t capacity) :
    m_q(capacity)
{
}

void
Heap::insert(const uint32_t v)
{
    m_q.push(v);
}

bool
Heap::delete_min(uint32_t &v)
{
    return m_q.pop(v);
}
