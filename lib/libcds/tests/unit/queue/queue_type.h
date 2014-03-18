//$$CDS-header$$

#ifndef __UNIT_QUEUE_TYPES_H
#define __UNIT_QUEUE_TYPES_H

#include <cds/container/msqueue.h>
#include <cds/container/moir_queue.h>
#include <cds/container/rwqueue.h>
#include <cds/container/optimistic_queue.h>
#include <cds/container/tsigas_cycle_queue.h>
#include <cds/container/vyukov_mpmc_cycle_queue.h>
#include <cds/container/basket_queue.h>

#include <cds/container/michael_deque.h>

#include <cds/gc/hp.h>
#include <cds/gc/hrc.h>
#include <cds/gc/ptb.h>

#include "queue/std_queue.h"
#include "lock/win32_lock.h"
#include "michael_alloc.h"
#include "print_deque_stat.h"
#include <boost/thread/mutex.hpp>

namespace queue {

    namespace details {
        // MichaelDeque, push right/pop left
        template <typename GC, typename T, CDS_DECL_OPTIONS7>
        class MichaelDequeR: public cds::container::MichaelDeque< GC, T, CDS_OPTIONS7>
        {
            typedef cds::container::MichaelDeque< GC, T, CDS_OPTIONS7> base_class ;
        public:
            MichaelDequeR( size_t nMaxItemCount )
                : base_class( (unsigned int) nMaxItemCount, 4 )
            {}
            MichaelDequeR()
                : base_class( 64 * 1024, 4 )
            {}

            bool push( T const& v )
            {
                return base_class::push_back( v )   ;
            }
            bool enqueue( T const& v )
            {
                return push( v )    ;
            }

            bool pop( T& v )
            {
                return base_class::pop_front( v )    ;
            }
            bool deque( T& v )
            {
                return pop(v)   ;
            }
        };

        // MichaelDeque, push left/pop right
        template <typename GC, typename T, CDS_DECL_OPTIONS7>
        class MichaelDequeL: public cds::container::MichaelDeque< GC, T, CDS_OPTIONS7>
        {
            typedef cds::container::MichaelDeque< GC, T, CDS_OPTIONS7> base_class ;
        public:
            MichaelDequeL( size_t nMaxItemCount )
                : base_class( (unsigned int) nMaxItemCount, 4 )
            {}
            MichaelDequeL()
                : base_class( 64 * 1024, 4 )
            {}

            bool push( T const& v )
            {
                return base_class::push_front( v )   ;
            }
            bool enqueue( T const& v )
            {
                return push( v )    ;
            }

            bool pop( T& v )
            {
                return base_class::pop_back( v )    ;
            }
            bool deque( T& v )
            {
                return pop(v)   ;
            }
        };

    }

    template <typename Value>
    struct Types {

        // MSQueue
        typedef cds::container::MSQueue<
            cds::gc::HP , Value
        >   MSQueue_HP  ;

        typedef cds::container::MSQueue<
            cds::gc::HP , Value
            ,cds::opt::allocator< memory::MichaelAllocator<int> >
        >   MSQueue_HP_michaelAlloc  ;

        typedef cds::container::MSQueue<
            cds::gc::HP, Value
            ,cds::opt::memory_model< cds::opt::v::sequential_consistent >
        >   MSQueue_HP_seqcst  ;

        typedef cds::container::MSQueue< cds::gc::HRC,
            Value
        >   MSQueue_HRC  ;

        typedef cds::container::MSQueue< cds::gc::HRC,
            Value
            ,cds::opt::allocator< memory::MichaelAllocator<int> >
        >   MSQueue_HRC_michaelAlloc  ;

        typedef cds::container::MSQueue< cds::gc::HRC,
            Value
            ,cds::opt::memory_model< cds::opt::v::sequential_consistent >
        >   MSQueue_HRC_seqcst  ;

        typedef cds::container::MSQueue< cds::gc::PTB,
            Value
        >   MSQueue_PTB  ;

        typedef cds::container::MSQueue< cds::gc::PTB,
            Value
            ,cds::opt::allocator< memory::MichaelAllocator<int> >
        >   MSQueue_PTB_michaelAlloc  ;

        typedef cds::container::MSQueue< cds::gc::PTB,
            Value
            ,cds::opt::memory_model< cds::opt::v::sequential_consistent >
        >   MSQueue_PTB_seqcst  ;

        // MSQueue + item counter
        typedef cds::container::MSQueue< cds::gc::HP,
            Value
            ,cds::opt::item_counter< cds::atomicity::item_counter >
        >   MSQueue_HP_ic  ;

        typedef cds::container::MSQueue< cds::gc::HRC,
            Value
            ,cds::opt::item_counter< cds::atomicity::item_counter >
        >   MSQueue_HRC_ic  ;

        typedef cds::container::MSQueue< cds::gc::PTB,
            Value
            ,cds::opt::item_counter< cds::atomicity::item_counter >
        >   MSQueue_PTB_ic  ;

        // MSQueue + stat
        typedef cds::container::MSQueue< cds::gc::HP,
            Value
            ,cds::opt::stat< cds::intrusive::queue_stat<> >
        >   MSQueue_HP_stat  ;

        typedef cds::container::MSQueue< cds::gc::HRC,
            Value
            ,cds::opt::stat< cds::intrusive::queue_stat<> >
        >   MSQueue_HRC_stat  ;

        typedef cds::container::MSQueue< cds::gc::PTB,
            Value
            ,cds::opt::stat< cds::intrusive::queue_stat<> >
        >   MSQueue_PTB_stat  ;


        // MoirQueue
        typedef cds::container::MoirQueue< cds::gc::HP,
            Value
        >   MoirQueue_HP  ;

        typedef cds::container::MoirQueue< cds::gc::HP,
            Value
            ,cds::opt::allocator< memory::MichaelAllocator<int> >
        >   MoirQueue_HP_michaelAlloc  ;

        typedef cds::container::MoirQueue< cds::gc::HP,
            Value
            ,cds::opt::memory_model< cds::opt::v::sequential_consistent >
        >   MoirQueue_HP_seqcst  ;

        typedef cds::container::MoirQueue< cds::gc::HRC,
            Value
        >   MoirQueue_HRC  ;

        typedef cds::container::MoirQueue< cds::gc::HRC,
            Value
            ,cds::opt::allocator< memory::MichaelAllocator<int> >
        >   MoirQueue_HRC_michaelAlloc  ;

        typedef cds::container::MoirQueue< cds::gc::HRC,
            Value
            ,cds::opt::memory_model< cds::opt::v::sequential_consistent >
        >   MoirQueue_HRC_seqcst  ;

        typedef cds::container::MoirQueue< cds::gc::PTB,
            Value
        >   MoirQueue_PTB  ;

        typedef cds::container::MoirQueue< cds::gc::PTB,
            Value
            ,cds::opt::allocator< memory::MichaelAllocator<int> >
        >   MoirQueue_PTB_michaelAlloc  ;

        typedef cds::container::MoirQueue< cds::gc::PTB,
            Value
            ,cds::opt::memory_model< cds::opt::v::sequential_consistent >
        >   MoirQueue_PTB_seqcst  ;

        // MoirQueue + item counter
        typedef cds::container::MoirQueue< cds::gc::HP,
            Value
            ,cds::opt::item_counter< cds::atomicity::item_counter >
        >   MoirQueue_HP_ic  ;

        typedef cds::container::MoirQueue< cds::gc::HRC,
            Value
            ,cds::opt::item_counter< cds::atomicity::item_counter >
        >   MoirQueue_HRC_ic  ;

        typedef cds::container::MoirQueue< cds::gc::PTB,
            Value
            ,cds::opt::item_counter< cds::atomicity::item_counter >
        >   MoirQueue_PTB_ic  ;

        // MoirQueue + stat
        typedef cds::container::MoirQueue< cds::gc::HP,
            Value
            ,cds::opt::stat< cds::intrusive::queue_stat<> >
        >   MoirQueue_HP_stat  ;

        typedef cds::container::MoirQueue< cds::gc::HRC,
            Value
            ,cds::opt::stat< cds::intrusive::queue_stat<> >
        >   MoirQueue_HRC_stat  ;

        typedef cds::container::MoirQueue< cds::gc::PTB,
            Value
            ,cds::opt::stat< cds::intrusive::queue_stat<> >
        >   MoirQueue_PTB_stat  ;

        // OptimisticQueue
        typedef cds::container::OptimisticQueue< cds::gc::HP,
            Value
        >   OptimisticQueue_HP  ;

        typedef cds::container::OptimisticQueue< cds::gc::HP,
            Value
            ,cds::opt::allocator< memory::MichaelAllocator<int> >
        >   OptimisticQueue_HP_michaelAlloc  ;

        typedef cds::container::OptimisticQueue< cds::gc::HP,
            Value
            ,cds::opt::memory_model< cds::opt::v::sequential_consistent >
        >   OptimisticQueue_HP_seqcst  ;

        typedef cds::container::OptimisticQueue< cds::gc::PTB,
            Value
        >   OptimisticQueue_PTB  ;

        typedef cds::container::OptimisticQueue< cds::gc::PTB,
            Value
            ,cds::opt::allocator< memory::MichaelAllocator<int> >
        >   OptimisticQueue_PTB_michaelAlloc ;

        typedef cds::container::OptimisticQueue< cds::gc::PTB,
            Value
            ,cds::opt::memory_model< cds::opt::v::sequential_consistent >
        >   OptimisticQueue_PTB_seqcst  ;


        // OptimisticQueue + item counter
        typedef cds::container::OptimisticQueue< cds::gc::HP,
            Value
            ,cds::opt::item_counter< cds::atomicity::item_counter >
        >   OptimisticQueue_HP_ic  ;

        typedef cds::container::OptimisticQueue< cds::gc::PTB,
            Value
            ,cds::opt::item_counter< cds::atomicity::item_counter >
        >   OptimisticQueue_PTB_ic  ;

        // OptimisticQueue + stat
        typedef cds::container::OptimisticQueue< cds::gc::HP,
            Value
            ,cds::opt::stat< cds::intrusive::queue_stat<> >
        >   OptimisticQueue_HP_stat  ;

        typedef cds::container::OptimisticQueue< cds::gc::PTB,
            Value
            ,cds::opt::stat< cds::intrusive::queue_stat<> >
        >   OptimisticQueue_PTB_stat  ;

        // TsigasCycleQueue
        class TsigasCycleQueue_dyn
            : public cds::container::TsigasCycleQueue<
                Value
                ,cds::opt::buffer< cds::opt::v::dynamic_buffer< int > >
            >
        {
            typedef cds::container::TsigasCycleQueue<
                Value
                ,cds::opt::buffer< cds::opt::v::dynamic_buffer< int > >
            > base_class ;
        public:
            TsigasCycleQueue_dyn()
                : base_class( 1024 * 64 )
            {}

            TsigasCycleQueue_dyn( size_t nCapacity )
                : base_class( nCapacity )
            {}

            cds::opt::none statistics() const
            {
                return cds::opt::none() ;
            }
        };

        class TsigasCycleQueue_dyn_michaelAlloc
            : public cds::container::TsigasCycleQueue<
                Value
                ,cds::opt::buffer< cds::opt::v::dynamic_buffer< int > >
                ,cds::opt::allocator< memory::MichaelAllocator<int> >
            >
        {
            typedef cds::container::TsigasCycleQueue<
                Value
                ,cds::opt::buffer< cds::opt::v::dynamic_buffer< int > >
                ,cds::opt::allocator< memory::MichaelAllocator<int> >
            > base_class ;
        public:
            TsigasCycleQueue_dyn_michaelAlloc()
                : base_class( 1024 * 64 )
            {}

            TsigasCycleQueue_dyn_michaelAlloc( size_t nCapacity )
                : base_class( nCapacity )
            {}

            cds::opt::none statistics() const
            {
                return cds::opt::none() ;
            }
        };

        class TsigasCycleQueue_dyn_ic
            : public cds::container::TsigasCycleQueue<
                Value
                ,cds::opt::buffer< cds::opt::v::dynamic_buffer< int > >
                ,cds::opt::item_counter< cds::atomicity::item_counter >
            >
        {
            typedef cds::container::TsigasCycleQueue<
                Value
                ,cds::opt::buffer< cds::opt::v::dynamic_buffer< int > >
                ,cds::opt::item_counter< cds::atomicity::item_counter >
            > base_class ;
        public:
            TsigasCycleQueue_dyn_ic()
                : base_class( 1024 * 64 )
            {}
            TsigasCycleQueue_dyn_ic( size_t nCapacity )
                : base_class( nCapacity )
            {}

            cds::opt::none statistics() const
            {
                return cds::opt::none() ;
            }
        };

        // VyukovMPMCCycleQueue
        class VyukovMPMCCycleQueue_dyn
            : public cds::container::VyukovMPMCCycleQueue<
                Value
                ,cds::opt::buffer< cds::opt::v::dynamic_buffer< int > >
            >
        {
            typedef cds::container::VyukovMPMCCycleQueue<
                Value
                ,cds::opt::buffer< cds::opt::v::dynamic_buffer< int > >
            > base_class ;
        public:
            VyukovMPMCCycleQueue_dyn()
                : base_class( 1024 * 64 )
            {}
            VyukovMPMCCycleQueue_dyn( size_t nCapacity )
                : base_class( nCapacity )
            {}

            cds::opt::none statistics() const
            {
                return cds::opt::none() ;
            }
        };

        class VyukovMPMCCycleQueue_dyn_michaelAlloc
            : public cds::container::VyukovMPMCCycleQueue<
            Value
            ,cds::opt::buffer< cds::opt::v::dynamic_buffer< int, memory::MichaelAllocator<int> > >
            >
        {
            typedef cds::container::VyukovMPMCCycleQueue<
                Value
                ,cds::opt::buffer< cds::opt::v::dynamic_buffer< int, memory::MichaelAllocator<int> > >
            > base_class ;
        public:
            VyukovMPMCCycleQueue_dyn_michaelAlloc()
                : base_class( 1024 * 64 )
            {}
            VyukovMPMCCycleQueue_dyn_michaelAlloc( size_t nCapacity )
                : base_class( nCapacity )
            {}

            cds::opt::none statistics() const
            {
                return cds::opt::none() ;
            }
        };

        class VyukovMPMCCycleQueue_dyn_ic
            : public cds::container::VyukovMPMCCycleQueue<
                Value
                ,cds::opt::buffer< cds::opt::v::dynamic_buffer< int > >
                ,cds::opt::item_counter< cds::atomicity::item_counter >
            >
        {
            typedef cds::container::VyukovMPMCCycleQueue<
                Value
                ,cds::opt::buffer< cds::opt::v::dynamic_buffer< int > >
                ,cds::opt::item_counter< cds::atomicity::item_counter >
            > base_class ;
        public:
            VyukovMPMCCycleQueue_dyn_ic()
                : base_class( 1024 * 64 )
            {}
            VyukovMPMCCycleQueue_dyn_ic( size_t nCapacity )
                : base_class( nCapacity )
            {}

            cds::opt::none statistics() const
            {
                return cds::opt::none() ;
            }
        };


        // MichaelDeque
        typedef details::MichaelDequeR< cds::gc::HP, Value >    MichaelDequeR_HP ;
        typedef details::MichaelDequeR< cds::gc::HP, Value
            ,cds::opt::item_counter< cds::atomicity::item_counter >
        >    MichaelDequeR_HP_ic ;
        typedef details::MichaelDequeR< cds::gc::HP, Value
            ,cds::opt::allocator< memory::MichaelAllocator<int> >
        >    MichaelDequeR_HP_michaelAlloc ;
        typedef details::MichaelDequeR< cds::gc::HP, Value
            ,cds::opt::stat< cds::intrusive::michael_deque::stat<> >
        >    MichaelDequeR_HP_stat ;

        typedef details::MichaelDequeR< cds::gc::PTB, Value >    MichaelDequeR_PTB ;
        typedef details::MichaelDequeR< cds::gc::PTB, Value
            ,cds::opt::item_counter< cds::atomicity::item_counter >
        >    MichaelDequeR_PTB_ic ;
        typedef details::MichaelDequeR< cds::gc::PTB, Value
            ,cds::opt::allocator< memory::MichaelAllocator<int> >
        >    MichaelDequeR_PTB_michaelAlloc ;
        typedef details::MichaelDequeR< cds::gc::PTB, Value
            ,cds::opt::stat< cds::intrusive::michael_deque::stat<> >
        >    MichaelDequeR_PTB_stat ;

        typedef details::MichaelDequeL< cds::gc::HP, Value >    MichaelDequeL_HP ;
        typedef details::MichaelDequeL< cds::gc::HP, Value
            ,cds::opt::item_counter< cds::atomicity::item_counter >
        >    MichaelDequeL_HP_ic ;
        typedef details::MichaelDequeL< cds::gc::HP, Value
            ,cds::opt::allocator< memory::MichaelAllocator<int> >
        >    MichaelDequeL_HP_michaelAlloc ;
        typedef details::MichaelDequeL< cds::gc::HP, Value
            ,cds::opt::stat< cds::intrusive::michael_deque::stat<> >
        >    MichaelDequeL_HP_stat ;


        typedef details::MichaelDequeL< cds::gc::PTB, Value >    MichaelDequeL_PTB ;
        typedef details::MichaelDequeL< cds::gc::PTB, Value
            ,cds::opt::item_counter< cds::atomicity::item_counter >
        >    MichaelDequeL_PTB_ic ;
        typedef details::MichaelDequeL< cds::gc::PTB, Value
            ,cds::opt::allocator< memory::MichaelAllocator<int> >
        >    MichaelDequeL_PTB_michaelAlloc ;
        typedef details::MichaelDequeL< cds::gc::PTB, Value
            ,cds::opt::stat< cds::intrusive::michael_deque::stat<> >
        >    MichaelDequeL_PTB_stat ;

        // BasketQueue
        typedef cds::container::BasketQueue<
            cds::gc::HP , Value
        >   BasketQueue_HP  ;

        typedef cds::container::BasketQueue<
            cds::gc::HP , Value
            ,cds::opt::allocator< memory::MichaelAllocator<int> >
        >   BasketQueue_HP_michaelAlloc  ;

        typedef cds::container::BasketQueue<
            cds::gc::HP, Value
            ,cds::opt::memory_model< cds::opt::v::sequential_consistent >
        >   BasketQueue_HP_seqcst  ;

        typedef cds::container::BasketQueue< cds::gc::HRC,
            Value
        >   BasketQueue_HRC  ;

        typedef cds::container::BasketQueue< cds::gc::HRC,
            Value
            ,cds::opt::allocator< memory::MichaelAllocator<int> >
        >   BasketQueue_HRC_michaelAlloc  ;

        typedef cds::container::BasketQueue< cds::gc::HRC,
            Value
            ,cds::opt::memory_model< cds::opt::v::sequential_consistent >
        >   BasketQueue_HRC_seqcst  ;

        typedef cds::container::BasketQueue< cds::gc::PTB,
            Value
        >   BasketQueue_PTB  ;

        typedef cds::container::BasketQueue< cds::gc::PTB,
            Value
            ,cds::opt::allocator< memory::MichaelAllocator<int> >
        >   BasketQueue_PTB_michaelAlloc  ;

        typedef cds::container::BasketQueue< cds::gc::PTB,
            Value
            ,cds::opt::memory_model< cds::opt::v::sequential_consistent >
        >   BasketQueue_PTB_seqcst  ;

        // BasketQueue + item counter
        typedef cds::container::BasketQueue< cds::gc::HP,
            Value
            ,cds::opt::item_counter< cds::atomicity::item_counter >
        >   BasketQueue_HP_ic  ;

        typedef cds::container::BasketQueue< cds::gc::HRC,
            Value
            ,cds::opt::item_counter< cds::atomicity::item_counter >
        >   BasketQueue_HRC_ic  ;

        typedef cds::container::BasketQueue< cds::gc::PTB,
            Value
            ,cds::opt::item_counter< cds::atomicity::item_counter >
        >   BasketQueue_PTB_ic  ;

        // BasketQueue + stat
        typedef cds::container::BasketQueue< cds::gc::HP,
            Value
            ,cds::opt::stat< cds::intrusive::queue_stat<> >
        >   BasketQueue_HP_stat  ;

        typedef cds::container::BasketQueue< cds::gc::HRC,
            Value
            ,cds::opt::stat< cds::intrusive::queue_stat<> >
        >   BasketQueue_HRC_stat  ;

        typedef cds::container::BasketQueue< cds::gc::PTB,
            Value
            ,cds::opt::stat< cds::intrusive::queue_stat<> >
        >   BasketQueue_PTB_stat  ;


        // RWQueue
        typedef cds::container::RWQueue<
            Value
        > RWQueue_Spin    ;

        typedef cds::container::RWQueue<
            Value
            ,cds::opt::item_counter< cds::atomicity::item_counter >
        > RWQueue_Spin_ic    ;

        typedef cds::container::RWQueue<
            Value
            ,cds::opt::stat< cds::intrusive::queue_stat<> >
        > RWQueue_Spin_stat    ;

        typedef StdQueue_deque<Value>               StdQueue_deque_Spinlock     ;
        typedef StdQueue_list<Value>                StdQueue_list_Spinlock      ;
        typedef StdQueue_deque<Value, boost::mutex> StdQueue_deque_BoostMutex   ;
        typedef StdQueue_list<Value, boost::mutex>  StdQueue_list_BoostMutex    ;
#ifdef UNIT_LOCK_WIN_CS
        typedef StdQueue_deque<Value, lock::win::CS>    StdQueue_deque_WinCS    ;
        typedef StdQueue_list<Value, lock::win::CS>     StdQueue_list_WinCS     ;
        typedef StdQueue_deque<Value, lock::win::Mutex> StdQueue_deque_WinMutex    ;
        typedef StdQueue_list<Value, lock::win::Mutex>  StdQueue_list_WinMutex     ;
#endif
    };
}


// *********************************************
// Queue statistics
namespace std {

    // cds::intrusive::queue_stat
    template <typename Counter>
    static inline std::ostream& operator <<( std::ostream& o, cds::intrusive::queue_stat<Counter> const & s )
    {
        return o
            << "\tStatistics:\n"
            << "\t\t     Enqueue count: " << s.m_EnqueueCount.get() << "\n"
            << "\t\t      Enqueue race: " << s.m_EnqueueRace.get()  << "\n"
            << "\t\t     Dequeue count: " << s.m_DequeueCount.get() << "\n"
            << "\t\t      Dequeue race: " << s.m_DequeueRace.get()  << "\n"
            << "\t\tAdvance tail error: " << s.m_AdvanceTailError.get() << "\n"
            << "\t\t          Bad tail: " << s.m_BadTail.get() << "\n"
            ;
    }

    static inline std::ostream& operator <<( std::ostream& o, cds::intrusive::queue_dummy_stat const & s )
    {
        return o ;
    }

    static inline std::ostream& operator <<( std::ostream& o, cds::opt::none )
    {
        return o ;
    }

    // cds::intrusive::optimistic_queue::stat
    template <typename Counter>
    static inline std::ostream& operator <<( std::ostream& o, cds::intrusive::optimistic_queue::stat<Counter> const & s )
    {
        return o
            << static_cast<cds::intrusive::queue_stat<Counter> const &>( s )
            << "\t\t"
            << "\t\t    fix list call: " << s.m_FixListCount.get() << "\n"
            ;
    }

    static inline std::ostream& operator <<( std::ostream& o, cds::intrusive::optimistic_queue::dummy_stat const & s )
    {
        return o ;
    }

    // cds::intrusive::basket_queue::stat
    template <typename Counter>
    static inline std::ostream& operator <<( std::ostream& o, cds::intrusive::basket_queue::stat<Counter> const & s )
    {
        return o
            << static_cast<cds::intrusive::queue_stat<Counter> const &>( s )
            << "\t\tTry Add basket count: " << s.m_TryAddBasket.get() << "\n"
            << "\t\t    Add basket count: " << s.m_AddBasketCount.get() << "\n"
            ;
    }

    static inline std::ostream& operator <<( std::ostream& o, cds::intrusive::basket_queue::dummy_stat const & s )
    {
        return o ;
    }

}

#endif // #ifndef __UNIT_QUEUE_TYPES_H
