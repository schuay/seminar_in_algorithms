#include <noble.h>

Noble::Noble()
{
    /* Lock-free, bounded memory usage. */
    m_q = pq_t::CreateLF_EB();
}

Noble::~Noble()
{
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
    /* TODO: Noble is based on pointers.
     * Think of some way to avoid losing performance opposed
     * to other implementations. */

    uint32_t *u;
    u = m_q->DeleteMin(nullptr);

    if (u == nullptr) {
        return false;
    }

    v = *u;
    delete u;

    return true;
}
