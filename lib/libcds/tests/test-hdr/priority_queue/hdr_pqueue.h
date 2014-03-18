//$$CDS-header$$

#ifndef CDSHDRTEST_PQUEUE_H
#define CDSHDRTEST_PQUEUE_H

#include "cppunit/cppunit_proxy.h"
#include "size_check.h"
#include <algorithm>

namespace priority_queue {

    namespace pqueue {
        static size_t const c_nCapacity = 1024 * 16 ;

        struct disposer {
            static size_t   m_nCallCount ;

            template <typename T>
            void operator()( T& p )
            {
                ++m_nCallCount  ;
            }
        };

        template <typename PQueue>
        struct constants {
            static size_t const nCapacity = c_nCapacity ;
        };
    } // namespace pqueue

    class PQueueHdrTest: public CppUnitMini::TestCase
    {
    public:
        static size_t const c_nCapacity = pqueue::c_nCapacity ;

        typedef int     key_type    ;
        static key_type const c_nMinValue = -123  ;

        struct value_type {
            key_type    k   ;
            int         v   ;

            value_type()
            {}

            value_type( value_type const& kv )
                : k(kv.k)
                , v(kv.v)
            {}

            value_type( key_type key )
                : k(key)
                , v(key)
            {}

            value_type( key_type key, int val )
                : k(key)
                , v(val)
            {}

            value_type( std::pair<key_type, int> const& p )
                : k(p.first)
                , v(p.second)
            {}
        };

        struct compare {
            int operator()( value_type k1, value_type k2 ) const
            {
                return k1.k - k2.k ;
            }
        };

        struct less {
            bool operator()( value_type k1, value_type k2 ) const
            {
                return k1.k < k2.k ;
            }
        };

        template <typename T>
        class data_array
        {
            T *     pFirst  ;
            T *     pLast   ;

        public:
            data_array( size_t nSize )
                : pFirst( new T[nSize] )
                , pLast( pFirst + nSize )
            {
                key_type i = c_nMinValue   ;
                for ( T * p = pFirst; p != pLast; ++p, ++i )
                    p->k = p->v = i  ;

                std::random_shuffle( pFirst, pLast )    ;
            }

            ~data_array()
            {
                delete [] pFirst ;
            }

            T * begin() { return pFirst; }
            T * end()   { return pLast ; }
            size_t size() const
            {
                return pLast - pFirst ;
            }
        };

        struct move_functor
        {
            void operator()( int& dest, value_type const& src ) const
            {
                dest = src.k    ;
            }
        };

    protected:
        template <class PQueue>
        void test_bounded_with( PQueue& pq )
        {
            data_array<value_type> arr( pq.capacity() ) ;
            value_type * pFirst = arr.begin()   ;
            value_type * pLast  = pFirst + pq.capacity() ;

            CPPUNIT_ASSERT( pq.empty() ) ;
            CPPUNIT_ASSERT( pq.size() == 0 ) ;
            CPPUNIT_ASSERT( pq.capacity() == pqueue::constants<PQueue>::nCapacity ) ;

            size_t nSize = 0 ;

            // Push test
            for ( value_type * p = pFirst; p < pLast; ++p ) {
#ifdef CDS_EMPLACE_SUPPORT
                switch ( pq.size() & 3 ) {
                    case 1:
                        CPPUNIT_ASSERT( pq.emplace( p->k, p->v )) ;
                        break ;
                    case 2:
                        CPPUNIT_ASSERT( pq.emplace( std::make_pair( p->k, p->v ) )) ;
                        break ;
                    default:
                        CPPUNIT_ASSERT( pq.push( *p ))  ;
                }
#else
                CPPUNIT_ASSERT( pq.push( *p ))  ;
#endif
                CPPUNIT_ASSERT( !pq.empty() ) ;
                CPPUNIT_ASSERT( pq.size() == ++nSize ) ;
            }

            CPPUNIT_ASSERT( pq.full() ) ;
            CPPUNIT_ASSERT( pq.size() == pq.capacity() ) ;

            // The queue is full
            key_type k = c_nMinValue + key_type(c_nCapacity)    ;
            CPPUNIT_ASSERT( !pq.push( k )) ;
            CPPUNIT_ASSERT( pq.full() ) ;
            CPPUNIT_ASSERT( pq.size() == pq.capacity() ) ;

            // Pop test
            key_type nPrev = c_nMinValue + key_type(pq.capacity()) - 1 ;
            value_type kv(0)    ;
            key_type   key;
            CPPUNIT_ASSERT( pq.pop(kv) );
            CPPUNIT_CHECK_EX( kv.k == nPrev, "Expected=" << nPrev << ", current=" << kv.k ) ;

            CPPUNIT_ASSERT( pq.size() == pq.capacity() - 1 ) ;
            CPPUNIT_ASSERT( !pq.full() ) ;
            CPPUNIT_ASSERT( !pq.empty() ) ;

            nSize = pq.size()  ;
            while ( pq.size() > 1 ) {
                if ( pq.size() & 1 ) {
                    CPPUNIT_ASSERT( pq.pop(kv) ) ;
                    CPPUNIT_CHECK_EX( kv.k == nPrev - 1, "Expected=" << nPrev - 1 << ", current=" << kv.k ) ;
                    nPrev = kv.k  ;
                }
                else {
                    CPPUNIT_ASSERT( pq.pop_with( key, move_functor() )) ;
                    CPPUNIT_CHECK_EX( key == nPrev - 1, "Expected=" << nPrev - 1 << ", current=" << key ) ;
                    nPrev = key ;
                }

                --nSize     ;
                CPPUNIT_ASSERT( pq.size() == nSize ) ;
            }

            CPPUNIT_ASSERT( !pq.full() ) ;
            CPPUNIT_ASSERT( !pq.empty() ) ;
            CPPUNIT_ASSERT( pq.size() == 1 ) ;

            CPPUNIT_ASSERT( pq.pop(kv) ) ;
            CPPUNIT_CHECK_EX( kv.k == c_nMinValue, "Expected=" << c_nMinValue << ", current=" << kv.k ) ;

            CPPUNIT_ASSERT( !pq.full() ) ;
            CPPUNIT_ASSERT( pq.empty() ) ;
            CPPUNIT_ASSERT( pq.size() == 0 ) ;

            // Clear test
            for ( value_type * p = pFirst; p < pLast; ++p ) {
                CPPUNIT_ASSERT( pq.push( *p ))  ;
            }
            CPPUNIT_ASSERT( !pq.empty() ) ;
            CPPUNIT_ASSERT( pq.full() ) ;
            CPPUNIT_ASSERT( pq.size() == pq.capacity() ) ;

            pq.clear() ;
            CPPUNIT_ASSERT( pq.empty() ) ;
            CPPUNIT_ASSERT( !pq.full() ) ;
            CPPUNIT_ASSERT( pq.size() == 0 ) ;

            // clear_with test
            for ( value_type * p = pFirst; p < pLast; ++p ) {
                CPPUNIT_ASSERT( pq.push( *p ))  ;
            }
            CPPUNIT_ASSERT( !pq.empty() ) ;
            CPPUNIT_ASSERT( pq.full() ) ;
            CPPUNIT_ASSERT( pq.size() == pq.capacity() ) ;

            pqueue::disposer::m_nCallCount = 0;
            pq.clear_with( pqueue::disposer() )  ;
            CPPUNIT_ASSERT( pq.empty() ) ;
            CPPUNIT_ASSERT( !pq.full() ) ;
            CPPUNIT_ASSERT( pq.size() == 0 ) ;
            CPPUNIT_ASSERT( pqueue::disposer::m_nCallCount == pq.capacity() ) ;
        }

        template <class PQueue>
        void test_msq_stat()
        {
            PQueue pq( 0 );   // argument should be ignored for static buffer
            test_bounded_with( pq )     ;
        }
        template <class PQueue>
        void test_msq_dyn()
        {
            PQueue pq( c_nCapacity )    ;
            test_bounded_with( pq )     ;
        }

    public:
        void MSPQueue_st()          ;
        void MSPQueue_st_cmp()      ;
        void MSPQueue_st_less()     ;
        void MSPQueue_st_cmpless()  ;
        void MSPQueue_st_cmp_mtx()  ;
        void MSPQueue_dyn()         ;
        void MSPQueue_dyn_cmp()     ;
        void MSPQueue_dyn_less()    ;
        void MSPQueue_dyn_cmpless() ;
        void MSPQueue_dyn_cmp_mtx() ;

        CPPUNIT_TEST_SUITE(PQueueHdrTest)
            CPPUNIT_TEST(MSPQueue_st)
            CPPUNIT_TEST(MSPQueue_st_cmp)
            CPPUNIT_TEST(MSPQueue_st_less)
            CPPUNIT_TEST(MSPQueue_st_cmpless)
            CPPUNIT_TEST(MSPQueue_st_cmp_mtx)
            CPPUNIT_TEST(MSPQueue_dyn)
            CPPUNIT_TEST(MSPQueue_dyn_cmp)
            CPPUNIT_TEST(MSPQueue_dyn_less)
            CPPUNIT_TEST(MSPQueue_dyn_cmpless)
            CPPUNIT_TEST(MSPQueue_dyn_cmp_mtx)
        CPPUNIT_TEST_SUITE_END()
    } ;

} // namespace priority_queue

namespace std {
    template<>
    struct less<priority_queue::PQueueHdrTest::value_type>
    {
        bool operator()( priority_queue::PQueueHdrTest::value_type const& v1, priority_queue::PQueueHdrTest::value_type const& v2) const
        {
            return priority_queue::PQueueHdrTest::less()( v1, v2 ) ;
        }
    };
}

#endif // #ifndef CDSHDRTEST_PQUEUE_H
