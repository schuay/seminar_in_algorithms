//$$CDS-header$$

#include "set/hdr_intrusive_set.h"

namespace set {

    class IntrusiveSkipListSetRCU: public IntrusiveHashSetHdrTest
    {
        typedef IntrusiveHashSetHdrTest base_class ;

        static size_t const c_nArrSize = 1000;

        template <typename Set>
        struct extract_disposer {
            void operator()( typename Set::value_type * pVal ) const
            {
                pVal->nVal = 0 ;
            }
        };

    protected:
        struct other_key {
            int nKey ;

            other_key()
            {}

            other_key( int key )
                : nKey(key)
            {}

            template <typename Q>
            other_key& operator=( Q const& src )
            {
                nKey = src.nKey ;
                return *this;
            }
        };

        template <typename StoredType>
        struct other_key_less
        {
            bool operator ()( StoredType const& n, other_key k ) const
            {
                return n.nKey < k.nKey ;
            }
            bool operator ()( other_key k, StoredType const& n ) const
            {
                return k.nKey < n.nKey ;
            }
        };

        struct copy_other_key
        {
            template <typename Q>
            void operator()( other_key& dest, Q const& src ) const
            {
                dest.nKey = src.nKey ;
            }
        };

    protected:

        template <class Set, typename PrintStat>
        void test_skiplist()
        {
            {
                Set s ;
                base_class::test_int_with( s )  ;
            }

            test_skiplist_<Set, PrintStat >() ;
        }

        template <class Set, typename PrintStat>
        void test_skiplist_()
        {
            Set s ;
            s.clear() ;
            CPPUNIT_ASSERT( s.empty() ) ;
            CPPUNIT_ASSERT( check_size( s, 0 )) ;

            typedef typename Set::gc::scoped_lock   rcu_lock    ;
            typedef typename Set::value_type        value_type  ;
            typedef typename Set::iterator          set_iterator;
            typedef typename Set::const_iterator    const_set_iterator ;

            value_type  v[c_nArrSize]  ;
            int nCount = 0  ;
            int nPrevKey = 0;

            // Test iterator - ascending order
            for ( int i = 0; i < (int) (sizeof(v)/sizeof(v[0])); ++i ) {
                v[i].nKey = i       ;
                v[i].nVal = i * 2   ;

                CPPUNIT_ASSERT( s.insert( v[i] ))   ;
            }
            CPPUNIT_ASSERT( check_size( s, sizeof(v)/sizeof(v[0]) )) ;

            CPPUNIT_MSG( PrintStat()(s, "Iterator test, ascending insert order") ) ;

            nCount = 0  ;
            nPrevKey = 0;
            {
                rcu_lock l ;
                for ( set_iterator it = s.begin(), itEnd = s.end(); it != itEnd; ++it ) {
                    CPPUNIT_ASSERT( (*it).nKey * 2 == it->nVal )    ;
                    CPPUNIT_ASSERT( s.find( it->nKey )) ;
                    it->nVal = (*it).nKey   ;
                    ++nCount    ;
                    if ( it != s.begin() ) {
                        CPPUNIT_ASSERT( nPrevKey + 1 == it->nKey ) ;
                    }
                    nPrevKey = it->nKey ;
                }
            }
            CPPUNIT_ASSERT( check_size( s, sizeof(v)/sizeof(v[0]) )) ;
            CPPUNIT_ASSERT( nCount == sizeof(v)/sizeof(v[0])) ;

            nCount = 0  ;
            {
                rcu_lock l ;
                for ( const_set_iterator it = s.cbegin(), itEnd = s.cend(); it != itEnd; ++it ) {
                    CPPUNIT_ASSERT( (*it).nKey == it->nVal )    ;
                    ++nCount    ;
                    if ( it != s.cbegin() ) {
                        CPPUNIT_ASSERT( nPrevKey + 1 == it->nKey ) ;
                    }
                    nPrevKey = it->nKey ;
                }
            }
            CPPUNIT_ASSERT( check_size( s, sizeof(v)/sizeof(v[0]) )) ;
            CPPUNIT_ASSERT( nCount == sizeof(v)/sizeof(v[0])) ;

            for ( size_t i = 0; i < sizeof(v)/sizeof(v[0]); ++i ) {
                CPPUNIT_ASSERT( v[i].nKey == v[i].nVal )   ;
                CPPUNIT_ASSERT( s.find( v[i].nKey )) ;
            }

            s.clear() ;
            CPPUNIT_ASSERT( s.empty() ) ;
            CPPUNIT_ASSERT( check_size( s, 0)) ;
            Set::gc::force_dispose()  ;

            for ( size_t i = 0; i < (int) sizeof(v)/sizeof(v[0]); ++i ) {
                CPPUNIT_ASSERT( v[i].nDisposeCount == 1 )   ;
            }

            // Test iterator - descending order
            for ( int i = (int) sizeof(v)/sizeof(v[0]) - 1; i >= 0; --i ) {
                v[i].nKey = i       ;
                v[i].nVal = i * 2   ;

                CPPUNIT_ASSERT( s.insert( v[i] ))   ;
            }
            CPPUNIT_ASSERT( check_size( s, sizeof(v)/sizeof(v[0]) )) ;

            CPPUNIT_MSG( PrintStat()(s, "Iterator test, descending insert order") ) ;

            nCount = 0 ;
            {
                rcu_lock l ;
                for ( set_iterator it = s.begin(), itEnd = s.end(); it != itEnd; ++it ) {
                    CPPUNIT_ASSERT( (*it).nKey * 2 == it->nVal )    ;
                    it->nVal = (*it).nKey   ;
                    ++nCount    ;
                    if ( it != s.begin() ) {
                        CPPUNIT_ASSERT( nPrevKey + 1 == it->nKey ) ;
                    }
                    nPrevKey = it->nKey ;
                }
            }
            CPPUNIT_ASSERT( check_size( s, sizeof(v)/sizeof(v[0]) )) ;
            CPPUNIT_ASSERT( nCount == sizeof(v)/sizeof(v[0])) ;

            nCount = 0  ;
            {
                rcu_lock l ;
                for ( const_set_iterator it = s.cbegin(), itEnd = s.cend(); it != itEnd; ++it ) {
                    CPPUNIT_ASSERT( (*it).nKey == it->nVal )    ;
                    ++nCount    ;
                    if ( it != s.cbegin() ) {
                        CPPUNIT_ASSERT( nPrevKey + 1 == it->nKey ) ;
                    }
                    nPrevKey = it->nKey ;
                }
            }
            CPPUNIT_ASSERT( check_size( s, sizeof(v)/sizeof(v[0]) )) ;
            CPPUNIT_ASSERT( nCount == sizeof(v)/sizeof(v[0])) ;

            for ( size_t i = 0; i < sizeof(v)/sizeof(v[0]); ++i ) {
                CPPUNIT_ASSERT( v[i].nKey == v[i].nVal )   ;
            }

            s.clear() ;
            CPPUNIT_ASSERT( s.empty() ) ;
            CPPUNIT_ASSERT( check_size( s, 0 )) ;
            Set::gc::force_dispose()  ;

            for ( size_t i = 0; i < sizeof(v)/sizeof(v[0]); ++i ) {
                CPPUNIT_ASSERT( v[i].nDisposeCount == 2 )   ;
            }

            // Test iterator - random order
            fill_skiplist( s, v );
            CPPUNIT_ASSERT( check_size( s, sizeof(v)/sizeof(v[0]) )) ;
            CPPUNIT_MSG( PrintStat()(s, "Iterator test, random insert order") ) ;

            nCount = 0 ;
            {
                rcu_lock l ;
                for ( set_iterator it = s.begin(), itEnd = s.end(); it != itEnd; ++it ) {
                    CPPUNIT_ASSERT( (*it).nKey * 2 == it->nVal )    ;
                    it->nVal = (*it).nKey   ;
                    ++nCount    ;
                    if ( it != s.begin() ) {
                        CPPUNIT_ASSERT( nPrevKey + 1 == it->nKey ) ;
                    }
                    nPrevKey = it->nKey ;
                }
            }
            CPPUNIT_ASSERT( check_size( s, sizeof(v)/sizeof(v[0]) )) ;
            CPPUNIT_ASSERT( nCount == sizeof(v)/sizeof(v[0])) ;

            nCount = 0  ;
            {
                rcu_lock l ;
                for ( const_set_iterator it = s.cbegin(), itEnd = s.cend(); it != itEnd; ++it ) {
                    CPPUNIT_ASSERT( (*it).nKey == it->nVal )    ;
                    ++nCount    ;
                    if ( it != s.cbegin() ) {
                        CPPUNIT_ASSERT( nPrevKey + 1 == it->nKey ) ;
                    }
                    nPrevKey = it->nKey ;
                }
            }
            CPPUNIT_ASSERT( check_size( s, sizeof(v)/sizeof(v[0]) )) ;
            CPPUNIT_ASSERT( nCount == sizeof(v)/sizeof(v[0])) ;

            for ( size_t i = 0; i < sizeof(v)/sizeof(v[0]); ++i ) {
                CPPUNIT_ASSERT( v[i].nKey == v[i].nVal )   ;
            }

            s.clear() ;
            CPPUNIT_ASSERT( s.empty() ) ;
            CPPUNIT_ASSERT( check_size( s, 0 )) ;
            Set::gc::force_dispose()  ;

            for ( size_t i = 0; i < sizeof(v)/sizeof(v[0]); ++i ) {
                CPPUNIT_ASSERT( v[i].nDisposeCount == 3 )   ;
            }

            // extract/get test
            {
                // extract
                {
                    fill_skiplist( s, v ) ;
                    value_type * pVal       ;
                    for ( int i = c_nArrSize - 1; i >= 0; i -= 1 ) {
                        rcu_lock l;
                        pVal = s.get( i )   ;
                        CPPUNIT_ASSERT( pVal != NULL );
                        CPPUNIT_CHECK( pVal->nKey == i );
                        CPPUNIT_CHECK( pVal->nVal == i * 2 );
                        pVal->nVal *= 2 ;

                        pVal = s.extract( i );
                        CPPUNIT_ASSERT( pVal != NULL );
                        CPPUNIT_CHECK( pVal->nKey == i );
                        CPPUNIT_CHECK( pVal->nVal == i * 4 );

                        CPPUNIT_CHECK( s.get( i ) == NULL );
                        CPPUNIT_CHECK( s.extract( i ) == NULL );
                    }
                    s.force_dispose();
                    CPPUNIT_CHECK( s.empty() );
                }
                Set::gc::force_dispose()  ;

                // extract_with
                {
                    fill_skiplist( s, v ) ;
                    for ( int i = c_nArrSize - 1; i >= 0; i -= 1 ) {
                        rcu_lock l;
                        value_type * pVal = s.get_with( other_key(i), other_key_less<typename Set::value_type>() );
                        CPPUNIT_ASSERT( pVal != NULL ) ;
                        CPPUNIT_CHECK( pVal->nKey == i );
                        CPPUNIT_CHECK( pVal->nVal == i * 2 );
                        pVal->nVal *= 2 ;

                        pVal = s.extract_with( other_key(i), other_key_less<typename Set::value_type>() );
                        CPPUNIT_ASSERT( pVal != NULL ) ;
                        CPPUNIT_CHECK( pVal->nKey == i );
                        CPPUNIT_CHECK( pVal->nVal == i * 4 );

                        CPPUNIT_CHECK( s.get_with( other_key(i), other_key_less<typename Set::value_type>() ) == NULL ) ;
                        CPPUNIT_CHECK( s.extract_with( other_key(i), other_key_less<typename Set::value_type>() ) == NULL ) ;
                    }
                    s.force_dispose();
                    CPPUNIT_CHECK( s.empty() );
                }
                Set::gc::force_dispose()  ;

                // extract_min
                {
                    fill_skiplist( s, v )   ;
                    value_type * pVal       ;
                    value_type * pMin       ;
                    int nPrevKey ;
                    {
                        rcu_lock l;
                        pVal = s.extract_min();
                        CPPUNIT_ASSERT( pVal != NULL );
                        nPrevKey = pVal->nKey ;
                    }
                    while ( !s.empty() ) {
                        rcu_lock l;
                        pMin = s.get_min();
                        CPPUNIT_ASSERT( pMin != NULL );
                        CPPUNIT_CHECK( pMin->nKey == nPrevKey + 1 );
                        CPPUNIT_CHECK( pMin->nVal == (nPrevKey + 1) * 2 );

                        pVal = s.extract_min();
                        CPPUNIT_ASSERT( pVal != NULL );
                        CPPUNIT_CHECK( pMin == pVal );
                        CPPUNIT_CHECK( pVal->nKey == nPrevKey + 1 );
                        CPPUNIT_CHECK( pVal->nVal == (nPrevKey + 1) * 2 );
                        nPrevKey = pVal->nKey;
                    }
                    s.force_dispose();
                    {
                        rcu_lock l;
                        CPPUNIT_CHECK( s.get_min() == NULL );
                        CPPUNIT_CHECK( s.extract_min() == NULL );
                        CPPUNIT_CHECK( s.get_max() == NULL );
                        CPPUNIT_CHECK( s.extract_max() == NULL );
                    }
                }
                Set::gc::force_dispose()  ;

                // extract_min( Q&, Func )
                {
                    fill_skiplist( s, v ) ;
                    int nPrevKey ;
                    other_key k;
                    CPPUNIT_ASSERT( s.extract_min( k, copy_other_key() )) ;
                    nPrevKey = k.nKey ;
                    while ( !s.empty() ) {
                        CPPUNIT_CHECK( s.extract_min( k, copy_other_key() )) ;
                        CPPUNIT_CHECK( k.nKey == nPrevKey + 1 );
                        nPrevKey = k.nKey;
                    }
                    CPPUNIT_CHECK( !s.extract_min( k, copy_other_key() ));
                }
                Set::gc::force_dispose()  ;

                // extract_min( Q& )
                {
                    fill_skiplist( s, v ) ;
                    int nPrevKey ;
                    other_key k;
                    CPPUNIT_ASSERT( s.extract_min( k )) ;
                    nPrevKey = k.nKey ;
                    while ( !s.empty() ) {
                        CPPUNIT_CHECK( s.extract_min( k )) ;
                        CPPUNIT_CHECK( k.nKey == nPrevKey + 1 );
                        nPrevKey = k.nKey;
                    }
                    CPPUNIT_CHECK( !s.extract_min( k )) ;
                }
                Set::gc::force_dispose()  ;

                // extract_max
                {
                    fill_skiplist( s, v ) ;
                    value_type * pVal       ;
                    value_type * pMax       ;
                    int nPrevKey ;
                    {
                        rcu_lock l;
                        pVal = s.extract_max();
                        CPPUNIT_ASSERT( pVal != NULL ) ;
                        nPrevKey = pVal->nKey ;
                    }
                    while ( !s.empty() ) {
                        rcu_lock l;
                        pMax = s.get_max();
                        CPPUNIT_ASSERT( pMax != NULL );
                        CPPUNIT_CHECK( pMax->nKey == nPrevKey - 1 );
                        CPPUNIT_CHECK( pMax->nVal == (nPrevKey - 1) * 2 );

                        pVal = s.extract_max();
                        CPPUNIT_CHECK( pVal != NULL ) ;
                        CPPUNIT_CHECK( pVal == pMax ) ;
                        CPPUNIT_CHECK( pVal->nKey == nPrevKey - 1 );
                        CPPUNIT_CHECK( pVal->nVal == (nPrevKey - 1) * 2 );
                        nPrevKey = pVal->nKey;
                    }
                    s.force_dispose();
                }
                Set::gc::force_dispose()  ;
                {
                    rcu_lock l;
                    CPPUNIT_CHECK( s.get_min() == NULL );
                    CPPUNIT_CHECK( s.extract_min() == NULL );
                    CPPUNIT_CHECK( s.get_max() == NULL );
                    CPPUNIT_CHECK( s.extract_max() == NULL );
                }

                // extract_max( Q&, Func )
                {
                    fill_skiplist( s, v ) ;
                    int nPrevKey ;
                    other_key k;
                    CPPUNIT_ASSERT( s.extract_max( k, copy_other_key() )) ;
                    nPrevKey = k.nKey ;
                    while ( !s.empty() ) {
                        CPPUNIT_CHECK( s.extract_max( k, copy_other_key() )) ;
                        CPPUNIT_CHECK( k.nKey == nPrevKey - 1 );
                        nPrevKey = k.nKey;
                    }
                    CPPUNIT_CHECK( !s.extract_max( k, copy_other_key() )) ;
                }
                Set::gc::force_dispose()  ;

                // extract_max( Q& )
                {
                    fill_skiplist( s, v ) ;
                    int nPrevKey ;
                    other_key k;
                    CPPUNIT_ASSERT( s.extract_max( k )) ;
                    nPrevKey = k.nKey ;
                    while ( !s.empty() ) {
                        CPPUNIT_CHECK( s.extract_max( k )) ;
                        CPPUNIT_CHECK( k.nKey == nPrevKey - 1 );
                        nPrevKey = k.nKey;
                    }
                    CPPUNIT_CHECK( !s.extract_max( k )) ;
                }
                Set::gc::force_dispose()  ;
            }

            CPPUNIT_MSG( PrintStat()(s, "After extract test") ) ;
        }

        template <typename Set>
        void fill_skiplist( Set& s, typename Set::value_type * pArr )
        {
            int nRand[c_nArrSize] ;
            for ( int i = 0; i < (int) c_nArrSize; ++i ) {
                nRand[i] = i ;
            }
            std::random_shuffle( nRand, nRand + c_nArrSize ) ;

            for ( int i = 0; i < (int) c_nArrSize; ++i ) {
                pArr[i].nKey = nRand[i]       ;
                pArr[i].nVal = nRand[i] * 2   ;
                CPPUNIT_ASSERT( s.insert( pArr[i] ))   ;
            }
            CPPUNIT_CHECK( check_size( s, c_nArrSize )) ;
        }

    public:

        // Skip-list RCU
        void skiplist_rcu_gpi_base_cmp() ;
        void skiplist_rcu_gpi_base_less() ;
        void skiplist_rcu_gpi_base_cmpmix() ;
        void skiplist_rcu_gpi_base_cmp_stat() ;
        void skiplist_rcu_gpi_base_less_stat() ;
        void skiplist_rcu_gpi_base_cmpmix_stat() ;
        void skiplist_rcu_gpi_base_cmp_xorshift() ;
        void skiplist_rcu_gpi_base_less_xorshift() ;
        void skiplist_rcu_gpi_base_cmpmix_xorshift() ;
        void skiplist_rcu_gpi_base_cmp_xorshift_stat() ;
        void skiplist_rcu_gpi_base_less_xorshift_stat() ;
        void skiplist_rcu_gpi_base_cmpmix_xorshift_stat() ;
        void skiplist_rcu_gpi_base_cmp_pascal() ;
        void skiplist_rcu_gpi_base_less_pascal() ;
        void skiplist_rcu_gpi_base_cmpmix_pascal() ;
        void skiplist_rcu_gpi_base_cmp_pascal_stat() ;
        void skiplist_rcu_gpi_base_less_pascal_stat() ;
        void skiplist_rcu_gpi_base_cmpmix_pascal_stat() ;

        void skiplist_rcu_gpi_member_cmp() ;
        void skiplist_rcu_gpi_member_less() ;
        void skiplist_rcu_gpi_member_cmpmix() ;
        void skiplist_rcu_gpi_member_cmp_stat() ;
        void skiplist_rcu_gpi_member_less_stat() ;
        void skiplist_rcu_gpi_member_cmpmix_stat() ;
        void skiplist_rcu_gpi_member_cmp_xorshift() ;
        void skiplist_rcu_gpi_member_less_xorshift() ;
        void skiplist_rcu_gpi_member_cmpmix_xorshift() ;
        void skiplist_rcu_gpi_member_cmp_xorshift_stat() ;
        void skiplist_rcu_gpi_member_less_xorshift_stat() ;
        void skiplist_rcu_gpi_member_cmpmix_xorshift_stat() ;
        void skiplist_rcu_gpi_member_cmp_pascal() ;
        void skiplist_rcu_gpi_member_less_pascal() ;
        void skiplist_rcu_gpi_member_cmpmix_pascal() ;
        void skiplist_rcu_gpi_member_cmp_pascal_stat() ;
        void skiplist_rcu_gpi_member_less_pascal_stat() ;
        void skiplist_rcu_gpi_member_cmpmix_pascal_stat() ;

        // general_buffered
        void skiplist_rcu_gpb_base_cmp() ;
        void skiplist_rcu_gpb_base_less() ;
        void skiplist_rcu_gpb_base_cmpmix() ;
        void skiplist_rcu_gpb_base_cmp_stat() ;
        void skiplist_rcu_gpb_base_less_stat() ;
        void skiplist_rcu_gpb_base_cmpmix_stat() ;
        void skiplist_rcu_gpb_base_cmp_xorshift() ;
        void skiplist_rcu_gpb_base_less_xorshift() ;
        void skiplist_rcu_gpb_base_cmpmix_xorshift() ;
        void skiplist_rcu_gpb_base_cmp_xorshift_stat() ;
        void skiplist_rcu_gpb_base_less_xorshift_stat() ;
        void skiplist_rcu_gpb_base_cmpmix_xorshift_stat() ;
        void skiplist_rcu_gpb_base_cmp_pascal() ;
        void skiplist_rcu_gpb_base_less_pascal() ;
        void skiplist_rcu_gpb_base_cmpmix_pascal() ;
        void skiplist_rcu_gpb_base_cmp_pascal_stat() ;
        void skiplist_rcu_gpb_base_less_pascal_stat() ;
        void skiplist_rcu_gpb_base_cmpmix_pascal_stat() ;

        void skiplist_rcu_gpb_member_cmp() ;
        void skiplist_rcu_gpb_member_less() ;
        void skiplist_rcu_gpb_member_cmpmix() ;
        void skiplist_rcu_gpb_member_cmp_stat() ;
        void skiplist_rcu_gpb_member_less_stat() ;
        void skiplist_rcu_gpb_member_cmpmix_stat() ;
        void skiplist_rcu_gpb_member_cmp_xorshift() ;
        void skiplist_rcu_gpb_member_less_xorshift() ;
        void skiplist_rcu_gpb_member_cmpmix_xorshift() ;
        void skiplist_rcu_gpb_member_cmp_xorshift_stat() ;
        void skiplist_rcu_gpb_member_less_xorshift_stat() ;
        void skiplist_rcu_gpb_member_cmpmix_xorshift_stat() ;
        void skiplist_rcu_gpb_member_cmp_pascal() ;
        void skiplist_rcu_gpb_member_less_pascal() ;
        void skiplist_rcu_gpb_member_cmpmix_pascal() ;
        void skiplist_rcu_gpb_member_cmp_pascal_stat() ;
        void skiplist_rcu_gpb_member_less_pascal_stat() ;
        void skiplist_rcu_gpb_member_cmpmix_pascal_stat() ;

        // general_threaded
        void skiplist_rcu_gpt_base_cmp() ;
        void skiplist_rcu_gpt_base_less() ;
        void skiplist_rcu_gpt_base_cmpmix() ;
        void skiplist_rcu_gpt_base_cmp_stat() ;
        void skiplist_rcu_gpt_base_less_stat() ;
        void skiplist_rcu_gpt_base_cmpmix_stat() ;
        void skiplist_rcu_gpt_base_cmp_xorshift() ;
        void skiplist_rcu_gpt_base_less_xorshift() ;
        void skiplist_rcu_gpt_base_cmpmix_xorshift() ;
        void skiplist_rcu_gpt_base_cmp_xorshift_stat() ;
        void skiplist_rcu_gpt_base_less_xorshift_stat() ;
        void skiplist_rcu_gpt_base_cmpmix_xorshift_stat() ;
        void skiplist_rcu_gpt_base_cmp_pascal() ;
        void skiplist_rcu_gpt_base_less_pascal() ;
        void skiplist_rcu_gpt_base_cmpmix_pascal() ;
        void skiplist_rcu_gpt_base_cmp_pascal_stat() ;
        void skiplist_rcu_gpt_base_less_pascal_stat() ;
        void skiplist_rcu_gpt_base_cmpmix_pascal_stat() ;

        void skiplist_rcu_gpt_member_cmp() ;
        void skiplist_rcu_gpt_member_less() ;
        void skiplist_rcu_gpt_member_cmpmix() ;
        void skiplist_rcu_gpt_member_cmp_stat() ;
        void skiplist_rcu_gpt_member_less_stat() ;
        void skiplist_rcu_gpt_member_cmpmix_stat() ;
        void skiplist_rcu_gpt_member_cmp_xorshift() ;
        void skiplist_rcu_gpt_member_less_xorshift() ;
        void skiplist_rcu_gpt_member_cmpmix_xorshift() ;
        void skiplist_rcu_gpt_member_cmp_xorshift_stat() ;
        void skiplist_rcu_gpt_member_less_xorshift_stat() ;
        void skiplist_rcu_gpt_member_cmpmix_xorshift_stat() ;
        void skiplist_rcu_gpt_member_cmp_pascal() ;
        void skiplist_rcu_gpt_member_less_pascal() ;
        void skiplist_rcu_gpt_member_cmpmix_pascal() ;
        void skiplist_rcu_gpt_member_cmp_pascal_stat() ;
        void skiplist_rcu_gpt_member_less_pascal_stat() ;
        void skiplist_rcu_gpt_member_cmpmix_pascal_stat() ;

        // signal_buffered
        void skiplist_rcu_shb_base_cmp() ;
        void skiplist_rcu_shb_base_less() ;
        void skiplist_rcu_shb_base_cmpmix() ;
        void skiplist_rcu_shb_base_cmp_stat() ;
        void skiplist_rcu_shb_base_less_stat() ;
        void skiplist_rcu_shb_base_cmpmix_stat() ;
        void skiplist_rcu_shb_base_cmp_xorshift() ;
        void skiplist_rcu_shb_base_less_xorshift() ;
        void skiplist_rcu_shb_base_cmpmix_xorshift() ;
        void skiplist_rcu_shb_base_cmp_xorshift_stat() ;
        void skiplist_rcu_shb_base_less_xorshift_stat() ;
        void skiplist_rcu_shb_base_cmpmix_xorshift_stat() ;
        void skiplist_rcu_shb_base_cmp_pascal() ;
        void skiplist_rcu_shb_base_less_pascal() ;
        void skiplist_rcu_shb_base_cmpmix_pascal() ;
        void skiplist_rcu_shb_base_cmp_pascal_stat() ;
        void skiplist_rcu_shb_base_less_pascal_stat() ;
        void skiplist_rcu_shb_base_cmpmix_pascal_stat() ;

        void skiplist_rcu_shb_member_cmp() ;
        void skiplist_rcu_shb_member_less() ;
        void skiplist_rcu_shb_member_cmpmix() ;
        void skiplist_rcu_shb_member_cmp_stat() ;
        void skiplist_rcu_shb_member_less_stat() ;
        void skiplist_rcu_shb_member_cmpmix_stat() ;
        void skiplist_rcu_shb_member_cmp_xorshift() ;
        void skiplist_rcu_shb_member_less_xorshift() ;
        void skiplist_rcu_shb_member_cmpmix_xorshift() ;
        void skiplist_rcu_shb_member_cmp_xorshift_stat() ;
        void skiplist_rcu_shb_member_less_xorshift_stat() ;
        void skiplist_rcu_shb_member_cmpmix_xorshift_stat() ;
        void skiplist_rcu_shb_member_cmp_pascal() ;
        void skiplist_rcu_shb_member_less_pascal() ;
        void skiplist_rcu_shb_member_cmpmix_pascal() ;
        void skiplist_rcu_shb_member_cmp_pascal_stat() ;
        void skiplist_rcu_shb_member_less_pascal_stat() ;
        void skiplist_rcu_shb_member_cmpmix_pascal_stat() ;

        // signal_threaded
        void skiplist_rcu_sht_base_cmp() ;
        void skiplist_rcu_sht_base_less() ;
        void skiplist_rcu_sht_base_cmpmix() ;
        void skiplist_rcu_sht_base_cmp_stat() ;
        void skiplist_rcu_sht_base_less_stat() ;
        void skiplist_rcu_sht_base_cmpmix_stat() ;
        void skiplist_rcu_sht_base_cmp_xorshift() ;
        void skiplist_rcu_sht_base_less_xorshift() ;
        void skiplist_rcu_sht_base_cmpmix_xorshift() ;
        void skiplist_rcu_sht_base_cmp_xorshift_stat() ;
        void skiplist_rcu_sht_base_less_xorshift_stat() ;
        void skiplist_rcu_sht_base_cmpmix_xorshift_stat() ;
        void skiplist_rcu_sht_base_cmp_pascal() ;
        void skiplist_rcu_sht_base_less_pascal() ;
        void skiplist_rcu_sht_base_cmpmix_pascal() ;
        void skiplist_rcu_sht_base_cmp_pascal_stat() ;
        void skiplist_rcu_sht_base_less_pascal_stat() ;
        void skiplist_rcu_sht_base_cmpmix_pascal_stat() ;

        void skiplist_rcu_sht_member_cmp() ;
        void skiplist_rcu_sht_member_less() ;
        void skiplist_rcu_sht_member_cmpmix() ;
        void skiplist_rcu_sht_member_cmp_stat() ;
        void skiplist_rcu_sht_member_less_stat() ;
        void skiplist_rcu_sht_member_cmpmix_stat() ;
        void skiplist_rcu_sht_member_cmp_xorshift() ;
        void skiplist_rcu_sht_member_less_xorshift() ;
        void skiplist_rcu_sht_member_cmpmix_xorshift() ;
        void skiplist_rcu_sht_member_cmp_xorshift_stat() ;
        void skiplist_rcu_sht_member_less_xorshift_stat() ;
        void skiplist_rcu_sht_member_cmpmix_xorshift_stat() ;
        void skiplist_rcu_sht_member_cmp_pascal() ;
        void skiplist_rcu_sht_member_less_pascal() ;
        void skiplist_rcu_sht_member_cmpmix_pascal() ;
        void skiplist_rcu_sht_member_cmp_pascal_stat() ;
        void skiplist_rcu_sht_member_less_pascal_stat() ;
        void skiplist_rcu_sht_member_cmpmix_pascal_stat() ;

        CPPUNIT_TEST_SUITE(IntrusiveSkipListSetRCU)
            CPPUNIT_TEST(skiplist_rcu_gpi_base_cmp)
            CPPUNIT_TEST(skiplist_rcu_gpi_base_less)
            CPPUNIT_TEST(skiplist_rcu_gpi_base_cmpmix)
            CPPUNIT_TEST(skiplist_rcu_gpi_base_cmp_stat)
            CPPUNIT_TEST(skiplist_rcu_gpi_base_less_stat)
            CPPUNIT_TEST(skiplist_rcu_gpi_base_cmpmix_stat)
            CPPUNIT_TEST(skiplist_rcu_gpi_base_cmp_xorshift)
            CPPUNIT_TEST(skiplist_rcu_gpi_base_less_xorshift)
            CPPUNIT_TEST(skiplist_rcu_gpi_base_cmpmix_xorshift)
            CPPUNIT_TEST(skiplist_rcu_gpi_base_cmp_xorshift_stat)
            CPPUNIT_TEST(skiplist_rcu_gpi_base_less_xorshift_stat)
            CPPUNIT_TEST(skiplist_rcu_gpi_base_cmpmix_xorshift_stat)
            CPPUNIT_TEST(skiplist_rcu_gpi_base_cmp_pascal)
            CPPUNIT_TEST(skiplist_rcu_gpi_base_less_pascal)
            CPPUNIT_TEST(skiplist_rcu_gpi_base_cmpmix_pascal)
            CPPUNIT_TEST(skiplist_rcu_gpi_base_cmp_pascal_stat)
            CPPUNIT_TEST(skiplist_rcu_gpi_base_less_pascal_stat)
            CPPUNIT_TEST(skiplist_rcu_gpi_base_cmpmix_pascal_stat)

            CPPUNIT_TEST(skiplist_rcu_gpi_member_cmp)
            CPPUNIT_TEST(skiplist_rcu_gpi_member_less)
            CPPUNIT_TEST(skiplist_rcu_gpi_member_cmpmix)
            CPPUNIT_TEST(skiplist_rcu_gpi_member_cmp_stat)
            CPPUNIT_TEST(skiplist_rcu_gpi_member_less_stat)
            CPPUNIT_TEST(skiplist_rcu_gpi_member_cmpmix_stat)
            CPPUNIT_TEST(skiplist_rcu_gpi_member_cmp_xorshift)
            CPPUNIT_TEST(skiplist_rcu_gpi_member_less_xorshift)
            CPPUNIT_TEST(skiplist_rcu_gpi_member_cmpmix_xorshift)
            CPPUNIT_TEST(skiplist_rcu_gpi_member_cmp_xorshift_stat)
            CPPUNIT_TEST(skiplist_rcu_gpi_member_less_xorshift_stat)
            CPPUNIT_TEST(skiplist_rcu_gpi_member_cmpmix_xorshift_stat)
            CPPUNIT_TEST(skiplist_rcu_gpi_member_cmp_pascal)
            CPPUNIT_TEST(skiplist_rcu_gpi_member_less_pascal)
            CPPUNIT_TEST(skiplist_rcu_gpi_member_cmpmix_pascal)
            CPPUNIT_TEST(skiplist_rcu_gpi_member_cmp_pascal_stat)
            CPPUNIT_TEST(skiplist_rcu_gpi_member_less_pascal_stat)
            CPPUNIT_TEST(skiplist_rcu_gpi_member_cmpmix_pascal_stat)

            //
            CPPUNIT_TEST(skiplist_rcu_gpb_base_cmp)
            CPPUNIT_TEST(skiplist_rcu_gpb_base_less)
            CPPUNIT_TEST(skiplist_rcu_gpb_base_cmpmix)
            CPPUNIT_TEST(skiplist_rcu_gpb_base_cmp_stat)
            CPPUNIT_TEST(skiplist_rcu_gpb_base_less_stat)
            CPPUNIT_TEST(skiplist_rcu_gpb_base_cmpmix_stat)
            CPPUNIT_TEST(skiplist_rcu_gpb_base_cmp_xorshift)
            CPPUNIT_TEST(skiplist_rcu_gpb_base_less_xorshift)
            CPPUNIT_TEST(skiplist_rcu_gpb_base_cmpmix_xorshift)
            CPPUNIT_TEST(skiplist_rcu_gpb_base_cmp_xorshift_stat)
            CPPUNIT_TEST(skiplist_rcu_gpb_base_less_xorshift_stat)
            CPPUNIT_TEST(skiplist_rcu_gpb_base_cmpmix_xorshift_stat)
            CPPUNIT_TEST(skiplist_rcu_gpb_base_cmp_pascal)
            CPPUNIT_TEST(skiplist_rcu_gpb_base_less_pascal)
            CPPUNIT_TEST(skiplist_rcu_gpb_base_cmpmix_pascal)
            CPPUNIT_TEST(skiplist_rcu_gpb_base_cmp_pascal_stat)
            CPPUNIT_TEST(skiplist_rcu_gpb_base_less_pascal_stat)
            CPPUNIT_TEST(skiplist_rcu_gpb_base_cmpmix_pascal_stat)

            CPPUNIT_TEST(skiplist_rcu_gpb_member_cmp)
            CPPUNIT_TEST(skiplist_rcu_gpb_member_less)
            CPPUNIT_TEST(skiplist_rcu_gpb_member_cmpmix)
            CPPUNIT_TEST(skiplist_rcu_gpb_member_cmp_stat)
            CPPUNIT_TEST(skiplist_rcu_gpb_member_less_stat)
            CPPUNIT_TEST(skiplist_rcu_gpb_member_cmpmix_stat)
            CPPUNIT_TEST(skiplist_rcu_gpb_member_cmp_xorshift)
            CPPUNIT_TEST(skiplist_rcu_gpb_member_less_xorshift)
            CPPUNIT_TEST(skiplist_rcu_gpb_member_cmpmix_xorshift)
            CPPUNIT_TEST(skiplist_rcu_gpb_member_cmp_xorshift_stat)
            CPPUNIT_TEST(skiplist_rcu_gpb_member_less_xorshift_stat)
            CPPUNIT_TEST(skiplist_rcu_gpb_member_cmpmix_xorshift_stat)
            CPPUNIT_TEST(skiplist_rcu_gpb_member_cmp_pascal)
            CPPUNIT_TEST(skiplist_rcu_gpb_member_less_pascal)
            CPPUNIT_TEST(skiplist_rcu_gpb_member_cmpmix_pascal)
            CPPUNIT_TEST(skiplist_rcu_gpb_member_cmp_pascal_stat)
            CPPUNIT_TEST(skiplist_rcu_gpb_member_less_pascal_stat)
            CPPUNIT_TEST(skiplist_rcu_gpb_member_cmpmix_pascal_stat)

            //
            CPPUNIT_TEST(skiplist_rcu_gpt_base_cmp)
            CPPUNIT_TEST(skiplist_rcu_gpt_base_less)
            CPPUNIT_TEST(skiplist_rcu_gpt_base_cmpmix)
            CPPUNIT_TEST(skiplist_rcu_gpt_base_cmp_stat)
            CPPUNIT_TEST(skiplist_rcu_gpt_base_less_stat)
            CPPUNIT_TEST(skiplist_rcu_gpt_base_cmpmix_stat)
            CPPUNIT_TEST(skiplist_rcu_gpt_base_cmp_xorshift)
            CPPUNIT_TEST(skiplist_rcu_gpt_base_less_xorshift)
            CPPUNIT_TEST(skiplist_rcu_gpt_base_cmpmix_xorshift)
            CPPUNIT_TEST(skiplist_rcu_gpt_base_cmp_xorshift_stat)
            CPPUNIT_TEST(skiplist_rcu_gpt_base_less_xorshift_stat)
            CPPUNIT_TEST(skiplist_rcu_gpt_base_cmpmix_xorshift_stat)
            CPPUNIT_TEST(skiplist_rcu_gpt_base_cmp_pascal)
            CPPUNIT_TEST(skiplist_rcu_gpt_base_less_pascal)
            CPPUNIT_TEST(skiplist_rcu_gpt_base_cmpmix_pascal)
            CPPUNIT_TEST(skiplist_rcu_gpt_base_cmp_pascal_stat)
            CPPUNIT_TEST(skiplist_rcu_gpt_base_less_pascal_stat)
            CPPUNIT_TEST(skiplist_rcu_gpt_base_cmpmix_pascal_stat)

            CPPUNIT_TEST(skiplist_rcu_gpt_member_cmp)
            CPPUNIT_TEST(skiplist_rcu_gpt_member_less)
            CPPUNIT_TEST(skiplist_rcu_gpt_member_cmpmix)
            CPPUNIT_TEST(skiplist_rcu_gpt_member_cmp_stat)
            CPPUNIT_TEST(skiplist_rcu_gpt_member_less_stat)
            CPPUNIT_TEST(skiplist_rcu_gpt_member_cmpmix_stat)
            CPPUNIT_TEST(skiplist_rcu_gpt_member_cmp_xorshift)
            CPPUNIT_TEST(skiplist_rcu_gpt_member_less_xorshift)
            CPPUNIT_TEST(skiplist_rcu_gpt_member_cmpmix_xorshift)
            CPPUNIT_TEST(skiplist_rcu_gpt_member_cmp_xorshift_stat)
            CPPUNIT_TEST(skiplist_rcu_gpt_member_less_xorshift_stat)
            CPPUNIT_TEST(skiplist_rcu_gpt_member_cmpmix_xorshift_stat)
            CPPUNIT_TEST(skiplist_rcu_gpt_member_cmp_pascal)
            CPPUNIT_TEST(skiplist_rcu_gpt_member_less_pascal)
            CPPUNIT_TEST(skiplist_rcu_gpt_member_cmpmix_pascal)
            CPPUNIT_TEST(skiplist_rcu_gpt_member_cmp_pascal_stat)
            CPPUNIT_TEST(skiplist_rcu_gpt_member_less_pascal_stat)
            CPPUNIT_TEST(skiplist_rcu_gpt_member_cmpmix_pascal_stat)

            //
            CPPUNIT_TEST(skiplist_rcu_shb_base_cmp)
            CPPUNIT_TEST(skiplist_rcu_shb_base_less)
            CPPUNIT_TEST(skiplist_rcu_shb_base_cmpmix)
            CPPUNIT_TEST(skiplist_rcu_shb_base_cmp_stat)
            CPPUNIT_TEST(skiplist_rcu_shb_base_less_stat)
            CPPUNIT_TEST(skiplist_rcu_shb_base_cmpmix_stat)
            CPPUNIT_TEST(skiplist_rcu_shb_base_cmp_xorshift)
            CPPUNIT_TEST(skiplist_rcu_shb_base_less_xorshift)
            CPPUNIT_TEST(skiplist_rcu_shb_base_cmpmix_xorshift)
            CPPUNIT_TEST(skiplist_rcu_shb_base_cmp_xorshift_stat)
            CPPUNIT_TEST(skiplist_rcu_shb_base_less_xorshift_stat)
            CPPUNIT_TEST(skiplist_rcu_shb_base_cmpmix_xorshift_stat)
            CPPUNIT_TEST(skiplist_rcu_shb_base_cmp_pascal)
            CPPUNIT_TEST(skiplist_rcu_shb_base_less_pascal)
            CPPUNIT_TEST(skiplist_rcu_shb_base_cmpmix_pascal)
            CPPUNIT_TEST(skiplist_rcu_shb_base_cmp_pascal_stat)
            CPPUNIT_TEST(skiplist_rcu_shb_base_less_pascal_stat)
            CPPUNIT_TEST(skiplist_rcu_shb_base_cmpmix_pascal_stat)

            CPPUNIT_TEST(skiplist_rcu_shb_member_cmp)
            CPPUNIT_TEST(skiplist_rcu_shb_member_less)
            CPPUNIT_TEST(skiplist_rcu_shb_member_cmpmix)
            CPPUNIT_TEST(skiplist_rcu_shb_member_cmp_stat)
            CPPUNIT_TEST(skiplist_rcu_shb_member_less_stat)
            CPPUNIT_TEST(skiplist_rcu_shb_member_cmpmix_stat)
            CPPUNIT_TEST(skiplist_rcu_shb_member_cmp_xorshift)
            CPPUNIT_TEST(skiplist_rcu_shb_member_less_xorshift)
            CPPUNIT_TEST(skiplist_rcu_shb_member_cmpmix_xorshift)
            CPPUNIT_TEST(skiplist_rcu_shb_member_cmp_xorshift_stat)
            CPPUNIT_TEST(skiplist_rcu_shb_member_less_xorshift_stat)
            CPPUNIT_TEST(skiplist_rcu_shb_member_cmpmix_xorshift_stat)
            CPPUNIT_TEST(skiplist_rcu_shb_member_cmp_pascal)
            CPPUNIT_TEST(skiplist_rcu_shb_member_less_pascal)
            CPPUNIT_TEST(skiplist_rcu_shb_member_cmpmix_pascal)
            CPPUNIT_TEST(skiplist_rcu_shb_member_cmp_pascal_stat)
            CPPUNIT_TEST(skiplist_rcu_shb_member_less_pascal_stat)
            CPPUNIT_TEST(skiplist_rcu_shb_member_cmpmix_pascal_stat)

            //
            CPPUNIT_TEST(skiplist_rcu_sht_base_cmp)
            CPPUNIT_TEST(skiplist_rcu_sht_base_less)
            CPPUNIT_TEST(skiplist_rcu_sht_base_cmpmix)
            CPPUNIT_TEST(skiplist_rcu_sht_base_cmp_stat)
            CPPUNIT_TEST(skiplist_rcu_sht_base_less_stat)
            CPPUNIT_TEST(skiplist_rcu_sht_base_cmpmix_stat)
            CPPUNIT_TEST(skiplist_rcu_sht_base_cmp_xorshift)
            CPPUNIT_TEST(skiplist_rcu_sht_base_less_xorshift)
            CPPUNIT_TEST(skiplist_rcu_sht_base_cmpmix_xorshift)
            CPPUNIT_TEST(skiplist_rcu_sht_base_cmp_xorshift_stat)
            CPPUNIT_TEST(skiplist_rcu_sht_base_less_xorshift_stat)
            CPPUNIT_TEST(skiplist_rcu_sht_base_cmpmix_xorshift_stat)
            CPPUNIT_TEST(skiplist_rcu_sht_base_cmp_pascal)
            CPPUNIT_TEST(skiplist_rcu_sht_base_less_pascal)
            CPPUNIT_TEST(skiplist_rcu_sht_base_cmpmix_pascal)
            CPPUNIT_TEST(skiplist_rcu_sht_base_cmp_pascal_stat)
            CPPUNIT_TEST(skiplist_rcu_sht_base_less_pascal_stat)
            CPPUNIT_TEST(skiplist_rcu_sht_base_cmpmix_pascal_stat)

            CPPUNIT_TEST(skiplist_rcu_sht_member_cmp)
            CPPUNIT_TEST(skiplist_rcu_sht_member_less)
            CPPUNIT_TEST(skiplist_rcu_sht_member_cmpmix)
            CPPUNIT_TEST(skiplist_rcu_sht_member_cmp_stat)
            CPPUNIT_TEST(skiplist_rcu_sht_member_less_stat)
            CPPUNIT_TEST(skiplist_rcu_sht_member_cmpmix_stat)
            CPPUNIT_TEST(skiplist_rcu_sht_member_cmp_xorshift)
            CPPUNIT_TEST(skiplist_rcu_sht_member_less_xorshift)
            CPPUNIT_TEST(skiplist_rcu_sht_member_cmpmix_xorshift)
            CPPUNIT_TEST(skiplist_rcu_sht_member_cmp_xorshift_stat)
            CPPUNIT_TEST(skiplist_rcu_sht_member_less_xorshift_stat)
            CPPUNIT_TEST(skiplist_rcu_sht_member_cmpmix_xorshift_stat)
            CPPUNIT_TEST(skiplist_rcu_sht_member_cmp_pascal)
            CPPUNIT_TEST(skiplist_rcu_sht_member_less_pascal)
            CPPUNIT_TEST(skiplist_rcu_sht_member_cmpmix_pascal)
            CPPUNIT_TEST(skiplist_rcu_sht_member_cmp_pascal_stat)
            CPPUNIT_TEST(skiplist_rcu_sht_member_less_pascal_stat)
            CPPUNIT_TEST(skiplist_rcu_sht_member_cmpmix_pascal_stat)

        CPPUNIT_TEST_SUITE_END()
    };
} // namespace set
