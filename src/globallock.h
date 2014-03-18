#ifndef __GLOBALLOCK_H
#define __GLOBALLOCK_H

#include <mutex>
#include <queue>

class GlobalLock
{
public:
    void init_thread(const size_t) { }

    void insert(const uint32_t v);
    bool delete_min(uint32_t &v);

private:
    typedef std::priority_queue<uint32_t> pq_t;

    std::mutex m_mutex;
    pq_t m_q;
};

#endif /* __GLOBALLOCK_H */
