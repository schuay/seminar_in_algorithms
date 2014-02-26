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

void Heap::init(const size_t size)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis;

    for (int i = 0; i < size; i++) {
        insert(dis(gen));
    }
}
