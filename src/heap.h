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
    struct type_traits {
        typedef cds::container::opt::v::dynamic_buffer<void *>  buffer      ;
        typedef cds::container::opt::none           compare     ;
        typedef std::greater<uint32_t>        less        ; /* We need a min-heap. */
        typedef cds::lock::Spin          lock_type   ;
        typedef cds::backoff::yield      back_off    ;
        typedef cds::opt::v::default_swap_policy    swap_policy ;
        typedef cds::opt::v::assignment_move_policy  move_policy ;
        typedef cds::intrusive::opt::v::empty_disposer  disposer ;
    };

private:
    typedef cds::container::MSPriorityQueue<uint32_t, type_traits>
            pq_t;

    pq_t m_q;
};

#endif /* __HEAP_H */
