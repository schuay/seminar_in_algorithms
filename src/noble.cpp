#include <noble.h>

Noble::Noble()
{
    /* Lock-free, bounded memory usage. */
    m_q = pq_t::CreateLF_EB();
}

Noble::~Noble()
{
    bool nonempty;
    uint32_t v;
    do {
        nonempty = delete_min(v);
    } while (nonempty);

    delete m_q;
}

void
Noble::insert(const uint32_t v)
{
    uint32_t *u = new uint32_t { v };
    m_q->Insert(u, u);
}

bool
Noble::delete_min(uint32_t &v)
{
    /* We tried allocating the ints in advance, but this actually
     * resulted in lower throughput. Instead, simply alloc/dealloc
     * in each operation. */

    uint32_t *u;
    u = m_q->DeleteMin(nullptr);

    if (u == nullptr) {
        return false;
    }

    v = *u;
    delete u;

    return true;
}
