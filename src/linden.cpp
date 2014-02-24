#include "linden.h"

extern "C" {
#include "linden/gc/gc.h"
}

static inline void
linden_insert(pq_t *pq,
			  const uint32_t v)
{
	insert(pq, v, v);
}

Linden::Linden(const int max_offset)
{
	_init_gc_subsystem();
	m_q = pq_init(max_offset);
}

Linden::~Linden()
{
	pq_destroy(m_q);
}

void
Linden::insert(const uint32_t v)
{
	linden_insert(m_q, v);
	_destroy_gc_subsystem();
}

bool
Linden::delete_min(uint32_t &v)
{
	v = deletemin(m_q);
	return true;
}
