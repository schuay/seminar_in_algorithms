#include <noble.h>

static constexpr uint32_t K_STRIDE = 1 << 12;
static constexpr uint32_t K_SIZE   = 1 << 20;

Noble::Noble()
{
    /* Lock-free, bounded memory usage. */
    m_q = pq_t::CreateLF_EB();


    m_keys = new uint32_t[K_SIZE];
    for (uint32_t i = 0; i < K_SIZE; i++) {
        m_keys[i] = i * K_STRIDE;
    }
}

Noble::~Noble()
{
    bool nonempty;
    uint32_t v;
    do {
        nonempty = delete_min(v);
    } while (nonempty);

    delete m_q;
    delete[] m_keys;
}

void
Noble::insert(const uint32_t v)
{
    m_q->Insert(&m_keys[v / K_STRIDE], &m_keys[v / K_STRIDE]);
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

    return true;
}
