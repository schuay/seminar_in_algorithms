//$$CDS-header$$

#include "priority_queue/hdr_pqueue.h"
#include <cds/container/mspriority_queue.h>
#include <cds/details/std/mutex.h>

namespace priority_queue {
    namespace pqueue {
        template <typename T, typename Traits>
        struct constants<cds::container::MSPriorityQueue<T, Traits> > {
            static size_t const nCapacity = c_nCapacity - 1 ;
        };
    }

    namespace {
        typedef cds::opt::v::dynamic_buffer< char > buffer_type ;
    }

    void PQueueHdrTest::MSPQueue_dyn()
    {
        typedef cds::container::MSPriorityQueue< PQueueHdrTest::value_type,
            cds::container::mspriority_queue::make_traits<
                cds::opt::buffer< buffer_type >
            >::type
        > pqueue    ;

        test_msq_dyn<pqueue>() ;
    }

    void PQueueHdrTest::MSPQueue_dyn_cmp()
    {
        typedef cds::container::MSPriorityQueue< PQueueHdrTest::value_type,
            cds::container::mspriority_queue::make_traits<
                cds::opt::buffer< buffer_type >
                ,cds::opt::compare< PQueueHdrTest::compare >
            >::type
        > pqueue    ;

        test_msq_dyn<pqueue>() ;
    }

    void PQueueHdrTest::MSPQueue_dyn_less()
    {
        typedef cds::container::MSPriorityQueue< PQueueHdrTest::value_type,
            cds::container::mspriority_queue::make_traits<
                cds::opt::buffer< buffer_type >
                ,cds::opt::less< PQueueHdrTest::less >
            >::type
        > pqueue    ;

        test_msq_dyn<pqueue>() ;
    }

    void PQueueHdrTest::MSPQueue_dyn_cmpless()
    {
        typedef cds::container::MSPriorityQueue< PQueueHdrTest::value_type,
            cds::container::mspriority_queue::make_traits<
                cds::opt::buffer< buffer_type >
                ,cds::opt::less< PQueueHdrTest::less >
                ,cds::opt::compare< PQueueHdrTest::compare >
            >::type
        > pqueue    ;

        test_msq_dyn<pqueue>() ;
    }

    void PQueueHdrTest::MSPQueue_dyn_cmp_mtx()
    {
        typedef cds::container::MSPriorityQueue< PQueueHdrTest::value_type,
            cds::container::mspriority_queue::make_traits<
                cds::opt::buffer< buffer_type >
                ,cds::opt::compare< PQueueHdrTest::compare >
                ,cds::opt::lock_type<cds_std::mutex>
            >::type
        > pqueue    ;

        test_msq_dyn<pqueue>() ;
    }

} // namespace priority_queue
