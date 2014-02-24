#include <cstdio>

#include "heap.h"
#include "linden.h"
#include "noble.h"

template <class T>
static void generic_pq_use(T &pq)
{
	pq.insert(1);
	pq.insert(2);
	pq.insert(3);

	uint32_t v;
	const bool ret = pq.delete_min(v);
	printf("delete_min -> (%d, %d)\n", ret, v);
}

int
main(int argc __attribute__ ((unused)),
	 char **argv __attribute__ ((unused)))
{
	/* TODO: Different comparators are used. Heap: MAX, Noble: MIN, Linden: MIN. */

	Heap heap(42);
	generic_pq_use(heap);

	Noble noble;
	generic_pq_use(noble);

	Linden linden(42);
	generic_pq_use(linden);

    return 0;
}
