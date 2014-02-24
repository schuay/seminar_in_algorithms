#ifndef __HEAP_H
#define __HEAP_H

#include "libcds/cds/container/mspriority_queue.h"

class Heap
{
public:
	Heap(const size_t capacity);

	void insert(const uint32_t v);
	bool delete_min(uint32_t &v);

private:
	typedef cds::container::MSPriorityQueue<uint32_t, cds::container::mspriority_queue::type_traits>
			pq_t;

	pq_t m_q;
};

#endif /* __HEAP_H */
