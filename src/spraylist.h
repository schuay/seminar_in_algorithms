#ifndef __SPRAYLIST_H
#define __SPRAYLIST_H

extern "C" {
#include "spraylist/pqueue.h"
}

class SprayList
{
public:
    SprayList();
    virtual ~SprayList();

    void init_thread(const size_t nthreads);

    void insert(const uint32_t v);
    bool delete_min(uint32_t &v);

private:
    typedef sl_intset_t pq_t;

    pq_t *m_q;
};

#endif /* __SPRAYLIST_H */
