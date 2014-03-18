//$$CDS-header$$

#include "priority_queue/hdr_intrusive_pqueue.h"
#include <cds/intrusive/mspriority_queue.h>
#include <cds/details/std/mutex.h>

namespace priority_queue {
    namespace intrusive_pqueue {
        template <typename T, typename Traits>
        struct constants<cds::intrusive::MSPriorityQueue<T, Traits> > {
            static size_t const nCapacity = c_nCapacity - 1 ;
        };
    }

    namespace {
        typedef cds::opt::v::static_buffer< char, IntrusivePQueueHdrTest::c_nCapacity > buffer_type ;
    }

    void IntrusivePQueueHdrTest::MSPQueue_st()
    {
        typedef cds::intrusive::MSPriorityQueue< IntrusivePQueueHdrTest::key_type,
            cds::intrusive::mspriority_queue::make_traits<
                cds::opt::buffer< buffer_type >
            >::type
        > pqueue    ;

        test_msq_stat<pqueue>() ;
    }

    void IntrusivePQueueHdrTest::MSPQueue_st_cmp()
    {
        typedef cds::intrusive::MSPriorityQueue< IntrusivePQueueHdrTest::key_type,
            cds::intrusive::mspriority_queue::make_traits<
                cds::opt::buffer< buffer_type >
                ,cds::opt::compare< IntrusivePQueueHdrTest::compare >
            >::type
        > pqueue    ;

        test_msq_stat<pqueue>() ;
    }

    void IntrusivePQueueHdrTest::MSPQueue_st_less()
    {
        typedef cds::intrusive::MSPriorityQueue< IntrusivePQueueHdrTest::key_type,
            cds::intrusive::mspriority_queue::make_traits<
                cds::opt::buffer< buffer_type >
                ,cds::opt::less< std::less<IntrusivePQueueHdrTest::key_type> >
            >::type
        > pqueue    ;

        test_msq_stat<pqueue>() ;
    }

    void IntrusivePQueueHdrTest::MSPQueue_st_less_disp()
    {
        typedef cds::intrusive::MSPriorityQueue< IntrusivePQueueHdrTest::key_type,
            cds::intrusive::mspriority_queue::make_traits<
                cds::opt::buffer< buffer_type >
                ,cds::opt::less< std::less<IntrusivePQueueHdrTest::key_type> >
                ,cds::intrusive::opt::disposer< intrusive_pqueue::disposer >
            >::type
        > pqueue    ;

        test_msq_stat<pqueue>() ;
    }

    void IntrusivePQueueHdrTest::MSPQueue_st_cmpless()
    {
        typedef cds::intrusive::MSPriorityQueue< IntrusivePQueueHdrTest::key_type,
            cds::intrusive::mspriority_queue::make_traits<
                cds::opt::buffer< buffer_type >
                ,cds::opt::less< std::less<IntrusivePQueueHdrTest::key_type> >
                ,cds::opt::compare< IntrusivePQueueHdrTest::compare >
            >::type
        > pqueue    ;

        test_msq_stat<pqueue>() ;
    }

    void IntrusivePQueueHdrTest::MSPQueue_st_cmp_mtx()
    {
        typedef cds::intrusive::MSPriorityQueue< IntrusivePQueueHdrTest::key_type,
            cds::intrusive::mspriority_queue::make_traits<
                cds::opt::buffer< buffer_type >
                ,cds::opt::compare< IntrusivePQueueHdrTest::compare >
                ,cds::opt::lock_type<cds_std::mutex>
            >::type
        > pqueue    ;

        test_msq_stat<pqueue>() ;
    }

} // namespace priority_queue
