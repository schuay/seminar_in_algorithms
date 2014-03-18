//$$CDS-header$$

#ifndef __CDS_BACKOFF_STRATEGY_H
#define __CDS_BACKOFF_STRATEGY_H

/*
    Filename: backoff_strategy.h
    Created 2007.03.01 by Maxim.Khiszinsky

    Description:
         Generic back-off strategies

    Editions:
    2007.03.01  Maxim Khiszinsky    Created
    2008.10.02  Maxim Khiszinsky    Backoff action transfers from contructor to operator() for all backoff schemas
    2009.09.10  Maxim Khiszinsky    reset() function added
*/

#include <cds/details/defs.h>
#include <cds/compiler/backoff.h>
#include <cds/os/thread.h>

namespace cds {
    /// Different backoff schemes
    /**
        Back-off schema may be used in lock-free algorithms when the algorithm cannot perform some action because a conflict
        with the other concurrent operation is encountered. In this case current thread can do another work or can call
        processor's performance hint.

        The interface of back-off strategy is following:
        \code
            struct backoff_strategy {
                void operator()()   ;
                void reset() ;
            };
        \endcode

        \p operator() operator calls back-off strategy's action. It is main part of back-off strategy.

        \p reset() function resets internal state of back-off strategy to initial state. It is required for some
        back-off strategies, for example, exponential back-off.
    */
    namespace backoff {

        /// Empty backoff strategy. Do nothing
        struct empty {
            //@cond
            void operator ()()
            {}

            void reset()
            {}
            //@endcond
        };

        /// Switch to another thread (yield). Good for thread preemption architecture.
        struct yield {
            //@cond
            void operator ()() { OS::yield(); }
            void reset() {}
            //@endcond
        };

        /// Random pause
        /**
            This back-off strategy calls processor-specific pause hint instruction
            if one is available for the processor architecture.
        */
        struct pause {
            //@cond
            void operator ()()
            {
#            ifdef CDS_backoff_pause_defined
                platform::backoff_pause();
#            endif
            }

            void reset()
            {}
            //@endcond
        };

        /// Processor hint back-off
        /**
            This back-off schema calls performance hint instruction if it is available for current processor.
            Otherwise, it calls \p nop.
        */
        struct hint
        {
        //@cond
            void operator ()()
            {
#           if defined(CDS_backoff_hint_defined)
                platform::backoff_hint()    ;
#           elif defined(CDS_backoff_nop_defined)
                platform::backoff_nop()     ;
#           endif
            }

            void reset()
            {}
        //@endcond
        };

        /// Exponential back-off
        /**
            This back-off strategy is composite. It consists of \p SpinBkoff and \p YieldBkoff
            back-off strategy. In first, the strategy tries to apply repeatedly \p SpinBkoff
            (spinning phase) until internal counter of failed attempts reaches its maximum
            spinning value. Then, the strategy transits to high-contention phase
            where it applies \p YieldBkoff until \p reset() is called.
            On each spinning iteration the internal spinning counter is doubled.

            Choosing the best value for maximum spinning bound is platform and task specific.
            In this implementation, the default values for maximum and minimum spinning is statically
            declared so you can set its value globally for your platform.
            The third template argument, \p Tag, is used to separate implementation. For
            example, you may define two \p exponential back-offs that is the best for your task A and B:
            \code

            #include <cds/backoff_strategy.h>
            namespace bkoff = cds::backoff  ;

            struct tagA ;   // tag to select task A implementation
            struct tagB ;   // tag to select task B implementation

            // // define your back-off specialization
            typedef bkoff::exponential<bkoff::hint, bkoff::yield, tagA> expBackOffA ;
            typedef bkoff::exponential<bkoff::hint, bkoff::yield, tagB> expBackOffB ;

            // // set up the best bounds for task A
            expBackOffA::s_nExpMin = 32      ;
            expBackOffA::s_nExpMax = 1024    ;

            // // set up the best bounds for task B
            expBackOffB::s_nExpMin = 2       ;
            expBackOffB::s_nExpMax = 512     ;

            \endcode

            Another way of solving this problem is subclassing \p exponential back-off class:
            \code
            #include <cds/backoff_strategy.h>
            namespace bkoff = cds::backoff  ;
            typedef bkoff::exponential<bkoff::hint, bkoff::yield>   base_bkoff  ;

            class expBackOffA: public base_bkoff
            {
            public:
                expBackOffA()
                    : base_bkoff( 32, 1024 )
                    {}
            };

            class expBackOffB: public base_bkoff
            {
            public:
                expBackOffB()
                    : base_bkoff( 2, 512 )
                    {}
            };
            \endcode
        */
        template <typename SpinBkoff, typename YieldBkoff, typename Tag=void>
        class exponential
        {
        public:
            typedef SpinBkoff  spin_backoff    ;   ///< spin back-off strategy
            typedef YieldBkoff yield_backoff   ;   ///< yield back-off strategy
            typedef Tag         impl_tag        ;   ///< implementation separation tag

            static size_t s_nExpMin ;   ///< Default minimum spinning bound (16)
            static size_t s_nExpMax ;   ///< Default maximum spinning bound (16384)

        protected:
            size_t  m_nExpCur   ;   ///< Current spinning
            size_t  m_nExpMin   ;   ///< Minimum spinning bound
            size_t  m_nExpMax   ;   ///< Maximum spinning bound

            spin_backoff    m_bkSpin    ;   ///< Spinning (fast-path) phase back-off strategy
            yield_backoff   m_bkYield   ;   ///< Yield phase back-off strategy

        public:
            /// Initializes m_nExpMin and m_nExpMax from default s_nExpMin and s_nExpMax respectively
            exponential()
                : m_nExpMin( s_nExpMin )
                , m_nExpMax( s_nExpMax )
            {
                m_nExpCur = m_nExpMin   ;
            }

            /// Explicitly defined bounds of spinning
            /**
                The \p libcds library never calls this ctor.
            */
            exponential(
                size_t nExpMin,     ///< Minimum spinning
                size_t nExpMax      ///< Maximum spinning
                )
                : m_nExpMin( nExpMin )
                , m_nExpMax( nExpMax )
            {
                m_nExpCur = m_nExpMin   ;
            }

            //@cond
            void operator ()()
            {
                if ( m_nExpCur <= m_nExpMax ) {
                    for ( size_t n = 0; n < m_nExpCur; ++n )
                        m_bkSpin()  ;
                    m_nExpCur *= 2  ;
                }
                else
                    m_bkYield() ;
            }

            void reset()
            {
                m_nExpCur = m_nExpMin   ;
                m_bkSpin.reset()    ;
                m_bkYield.reset()   ;
            }
            //@endcond
        };

        //@cond
        template <typename SpinBkoff, typename YieldBkoff, typename Tag>
        size_t exponential<SpinBkoff, YieldBkoff, Tag>::s_nExpMin = 16 ;

        template <typename SpinBkoff, typename YieldBkoff, typename Tag>
        size_t exponential<SpinBkoff, YieldBkoff, Tag>::s_nExpMax = 16 * 1024 ;
        //@endcond

        /// Default backoff strategy
        typedef exponential<hint, yield>    Default    ;

        /// Default back-off strategy for lock primitives
        typedef exponential<hint, yield>    LockDefault ;

    } // namespace backoff
} // namespace cds


#endif // #ifndef __CDS_BACKOFF_STRATEGY_H
