#ifndef __NOBLE_H
#define __NOBLE_H

#include <cstdint>

#include "noble/NobleCPP.h"

class Noble
{
public:
    Noble();
    virtual ~Noble();

    void insert(const uint32_t v);
    bool delete_min(uint32_t &v);

private:
    typedef NBL::PQueue<uint32_t, uint32_t> pq_t;

    pq_t *m_q;
};
#endif /* __NOBLE_H */
