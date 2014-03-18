//$$CDS-header$$

#ifndef __CDSTEST_HDR_MAP_H
#define __CDSTEST_HDR_MAP_H
#include "size_check.h"

#include "cppunit/cppunit_proxy.h"
#include <cds/os/timer.h>
#include <cds/opt/hash.h>
#include <cds/ref.h>
#include <algorithm>    // random_shuffle

namespace cds { namespace container {}}

namespace map {
    using misc::check_size ;

    namespace cc = cds::container   ;
    namespace co = cds::opt         ;

    // MichaelHashSet based on MichaelList
    class HashMapHdrTest: public CppUnitMini::TestCase
    {
    public:
        typedef int key_type    ;

        struct value_type {
            int m_val   ;

            value_type()
                : m_val(0)
            {}

            value_type( int n )
                : m_val( n )
            {}

#ifdef CDS_MOVE_SEMANTICS_SUPPORT
            value_type( value_type&& v )
                : m_val( v.m_val )
            {}

            value_type( value_type const& v )
                : m_val( v.m_val )
            {}

            value_type& operator=( value_type const& v )
            {
                m_val = v.m_val ;
                return *this ;
            }
#endif
        };

        typedef std::pair<key_type const, value_type> pair_type ;

        struct less
        {
            bool operator ()(int v1, int v2 ) const
            {
                return v1 < v2  ;
            }
        };

        struct cmp {
            int operator ()(int v1, int v2 ) const
            {
                if ( v1 < v2 )
                    return -1   ;
                return v1 > v2 ? 1 : 0  ;
            }
        };

        struct equal {
            bool operator ()(int v1, int v2 ) const
            {
                return v1 == v2 ;
            }
        };

        struct hash_int {
            size_t operator()( int i ) const
            {
                return co::v::hash<int>()( i ) ;
            }
        };

        struct simple_item_counter {
            size_t  m_nCount    ;

            simple_item_counter()
                : m_nCount(0)
            {}

            size_t operator ++()
            {
                return ++m_nCount ;
            }

            size_t operator --()
            {
                return --m_nCount   ;
            }

            void reset()
            {
                m_nCount = 0    ;
            }

            operator size_t() const
            {
                return m_nCount ;
            }
        };

        template <typename Map>
        struct insert_functor
        {
            typedef typename Map::value_type pair_type   ;

            // insert ftor
            void operator()( pair_type& item )
            {
                item.second.m_val = item.first * 3   ;
            }

            // ensure ftor
            void operator()( bool bNew, pair_type& item )
            {
                if ( bNew )
                    item.second.m_val = item.first * 2        ;
                else
                    item.second.m_val = item.first * 5       ;
            }
        };

        struct check_value {
            int     m_nExpected ;

            check_value( int nExpected )
                : m_nExpected( nExpected )
            {}

            template <typename T>
            void operator ()( T& pair )
            {
                CPPUNIT_ASSERT_CURRENT( pair.second.m_val == m_nExpected )  ;
            }
            template <typename T, typename Q>
            void operator ()( T& pair, Q )
            {
                CPPUNIT_ASSERT_CURRENT( pair.second.m_val == m_nExpected )  ;
            }
        };

        struct extract_functor
        {
            int *   m_pVal  ;
            void operator()( pair_type const& val )
            {
                *m_pVal = val.second.m_val   ;
            }
        };

        template <class Map>
        void test_int()
        {
            Map m( 52, 4 )  ;

            test_int_with(m)    ;

            // iterator test
            test_iter<Map>() ;
        }


        template <class Map>
        void test_int_with( Map& m )
        {
            std::pair<bool, bool> ensureResult ;

            // insert
            CPPUNIT_ASSERT( m.empty() )     ;
            CPPUNIT_ASSERT( check_size( m, 0 )) ;
            CPPUNIT_ASSERT( !m.find(25) )   ;
            CPPUNIT_ASSERT( m.insert( 25 ) )    ;   // value = 0
            CPPUNIT_ASSERT( m.find(25) )   ;
            CPPUNIT_ASSERT( !m.empty() ) ;
            CPPUNIT_ASSERT( check_size( m, 1 )) ;
            CPPUNIT_ASSERT( m.find(25) )   ;

            CPPUNIT_ASSERT( !m.insert( 25 ) )    ;
            CPPUNIT_ASSERT( !m.empty() ) ;
            CPPUNIT_ASSERT( check_size( m, 1 )) ;

            CPPUNIT_ASSERT( !m.find_with(10, less()) )   ;
            CPPUNIT_ASSERT( m.insert( 10, 10 ) )    ;
            CPPUNIT_ASSERT( !m.empty() ) ;
            CPPUNIT_ASSERT( check_size( m, 2 )) ;
            CPPUNIT_ASSERT( m.find_with(10, less()) )   ;

            CPPUNIT_ASSERT( !m.insert( 10, 20 ) )    ;
            CPPUNIT_ASSERT( !m.empty() ) ;
            CPPUNIT_ASSERT( check_size( m, 2 )) ;

            CPPUNIT_ASSERT( !m.find(30) )   ;
            CPPUNIT_ASSERT( m.insert_key( 30, insert_functor<Map>() ) )    ; // value = 90
            CPPUNIT_ASSERT( !m.empty() ) ;
            CPPUNIT_ASSERT( check_size( m, 3 )) ;
            CPPUNIT_ASSERT( m.find(30) )   ;

            CPPUNIT_ASSERT( !m.insert_key( 10, insert_functor<Map>() ) )    ;
            CPPUNIT_ASSERT( !m.insert_key( 25, insert_functor<Map>() ) )    ;
            CPPUNIT_ASSERT( !m.insert_key( 30, insert_functor<Map>() ) )    ;

            // ensure (new key)
            CPPUNIT_ASSERT( !m.find(27) )   ;
            ensureResult = m.ensure( 27, insert_functor<Map>() ) ;   // value = 54
            CPPUNIT_ASSERT( ensureResult.first )    ;
            CPPUNIT_ASSERT( ensureResult.second )    ;
            CPPUNIT_ASSERT( m.find(27) )   ;

            // find test
            check_value chk(10) ;
            CPPUNIT_ASSERT( m.find( 10, cds::ref(chk) ))   ;
            chk.m_nExpected = 0 ;
            CPPUNIT_ASSERT( m.find_with( 25, less(), boost::ref(chk) ))   ;
            chk.m_nExpected = 90 ;
            CPPUNIT_ASSERT( m.find( 30, boost::ref(chk) ))   ;
            chk.m_nExpected = 54 ;
            CPPUNIT_ASSERT( m.find( 27, boost::ref(chk) ))   ;

            ensureResult = m.ensure( 10, insert_functor<Map>() ) ;   // value = 50
            CPPUNIT_ASSERT( ensureResult.first )    ;
            CPPUNIT_ASSERT( !ensureResult.second )    ;
            chk.m_nExpected = 50 ;
            CPPUNIT_ASSERT( m.find( 10, boost::ref(chk) ))   ;

            // erase test
            CPPUNIT_ASSERT( !m.find(100) )   ;
            CPPUNIT_ASSERT( !m.erase( 100 )) ;  // not found

            CPPUNIT_ASSERT( m.find(25) )    ;
            CPPUNIT_ASSERT( check_size( m, 4 )) ;
            CPPUNIT_ASSERT( m.erase( 25 ))  ;
            CPPUNIT_ASSERT( !m.empty() ) ;
            CPPUNIT_ASSERT( check_size( m, 3 )) ;
            CPPUNIT_ASSERT( !m.find(25) )   ;
            CPPUNIT_ASSERT( !m.erase( 25 )) ;

            CPPUNIT_ASSERT( !m.find(258) )    ;
            CPPUNIT_ASSERT( m.insert(258))
            CPPUNIT_ASSERT( check_size( m, 4 )) ;
            CPPUNIT_ASSERT( m.find_with(258, less()) )    ;
            CPPUNIT_ASSERT( m.erase_with( 258, less() ))  ;
            CPPUNIT_ASSERT( !m.empty() ) ;
            CPPUNIT_ASSERT( check_size( m, 3 )) ;
            CPPUNIT_ASSERT( !m.find(258) )   ;
            CPPUNIT_ASSERT( !m.erase_with( 258, less() )) ;

            int nVal    ;
            extract_functor ext ;
            ext.m_pVal = &nVal  ;

            CPPUNIT_ASSERT( !m.find(29) )    ;
            CPPUNIT_ASSERT( m.insert(29, 290)) ;
            CPPUNIT_ASSERT( check_size( m, 4 )) ;
            CPPUNIT_ASSERT( m.erase_with( 29, less(), boost::ref(ext)))    ;
            CPPUNIT_ASSERT( !m.empty() ) ;
            CPPUNIT_ASSERT( check_size( m, 3 )) ;
            CPPUNIT_ASSERT( nVal == 290 )    ;
            nVal = -1   ;
            CPPUNIT_ASSERT( !m.erase_with( 29, less(), boost::ref(ext)))    ;
            CPPUNIT_ASSERT( nVal == -1 )    ;

            CPPUNIT_ASSERT( m.erase( 30, boost::ref(ext)))    ;
            CPPUNIT_ASSERT( !m.empty() ) ;
            CPPUNIT_ASSERT( check_size( m, 2 )) ;
            CPPUNIT_ASSERT( nVal == 90 )    ;
            nVal = -1   ;
            CPPUNIT_ASSERT( !m.erase( 30, boost::ref(ext)))    ;
            CPPUNIT_ASSERT( nVal == -1 )    ;

            m.clear()   ;
            CPPUNIT_ASSERT( m.empty() ) ;
            CPPUNIT_ASSERT( check_size( m, 0 )) ;

#       ifdef CDS_EMPLACE_SUPPORT
            // emplace test
            CPPUNIT_ASSERT( m.emplace(126) ) ; // key = 126, val = 0
            CPPUNIT_ASSERT( m.emplace(137, 731))    ;   // key = 137, val = 731
            CPPUNIT_ASSERT( m.emplace( 149, value_type(941) ))   ;   // key = 149, val = 941

            CPPUNIT_ASSERT( !m.empty() ) ;
            CPPUNIT_ASSERT( check_size( m, 3 )) ;

            chk.m_nExpected = 0 ;
            CPPUNIT_ASSERT( m.find( 126, cds::ref(chk) ))   ;
            chk.m_nExpected = 731 ;
            CPPUNIT_ASSERT( m.find_with( 137, less(), cds::ref(chk) ))   ;
            chk.m_nExpected = 941 ;
            CPPUNIT_ASSERT( m.find( 149, cds::ref(chk) ))   ;

            CPPUNIT_ASSERT( !m.emplace(126, 621)) ; // already in map
            chk.m_nExpected = 0 ;
            CPPUNIT_ASSERT( m.find( 126, cds::ref(chk) ))   ;
            CPPUNIT_ASSERT( !m.empty() ) ;
            CPPUNIT_ASSERT( check_size( m, 3 )) ;

            m.clear()   ;
            CPPUNIT_ASSERT( m.empty() ) ;
            CPPUNIT_ASSERT( check_size( m, 0 )) ;

#       endif
        }


        template <class Map>
        void test_int_nogc()
        {
            typedef typename Map::iterator          iterator    ;
            typedef typename Map::const_iterator    const_iterator  ;

            {
                Map m( 52, 4 )  ;

                CPPUNIT_ASSERT( m.empty() ) ;
                CPPUNIT_ASSERT( check_size( m, 0 )) ;

                CPPUNIT_ASSERT( m.find(10) == m.end() ) ;
                iterator it = m.insert( 10 )   ;
                CPPUNIT_ASSERT( it != m.end() )     ;
                CPPUNIT_ASSERT( !m.empty() )        ;
                CPPUNIT_ASSERT( check_size( m, 1 )) ;
                CPPUNIT_ASSERT( m.find(10) == it )  ;
                CPPUNIT_ASSERT( it->first == 10 )    ;
                CPPUNIT_ASSERT( it->second.m_val == 0 )     ;

                CPPUNIT_ASSERT( m.find(100) == m.end() ) ;
                it = m.insert( 100, 200 )   ;
                CPPUNIT_ASSERT( it != m.end() )     ;
                CPPUNIT_ASSERT( !m.empty() )        ;
                CPPUNIT_ASSERT( check_size( m, 2 )) ;
                CPPUNIT_ASSERT( m.find_with(100, less()) == it ) ;
                CPPUNIT_ASSERT( it->first == 100 )   ;
                CPPUNIT_ASSERT( it->second.m_val == 200 )   ;

                CPPUNIT_ASSERT( m.find(55) == m.end() ) ;
                it = m.insert_key( 55, insert_functor<Map>() )   ;
                CPPUNIT_ASSERT( it != m.end() )     ;
                CPPUNIT_ASSERT( !m.empty() )        ;
                CPPUNIT_ASSERT( check_size( m, 3 )) ;
                CPPUNIT_ASSERT( m.find(55) == it )  ;
                CPPUNIT_ASSERT( it->first == 55 )    ;
                CPPUNIT_ASSERT( it->second.m_val == 55 * 3 )    ;

                CPPUNIT_ASSERT( m.insert( 55 ) == m.end() ) ;
                CPPUNIT_ASSERT( m.insert( 55, 10 ) == m.end() ) ;
                CPPUNIT_ASSERT( m.insert_key( 55, insert_functor<Map>()) == m.end() )   ;

                CPPUNIT_ASSERT( m.find(10) != m.end() ) ;
                std::pair<iterator, bool> ensureResult = m.ensure( 10 )   ;
                CPPUNIT_ASSERT( ensureResult.first != m.end() )     ;
                CPPUNIT_ASSERT( !ensureResult.second  )     ;
                CPPUNIT_ASSERT( !m.empty() )        ;
                ensureResult.first->second.m_val = ensureResult.first->first * 5 ;
                CPPUNIT_ASSERT( check_size( m, 3 )) ;
                CPPUNIT_ASSERT( m.find(10) == ensureResult.first ) ;
                it = m.find(10) ;
                CPPUNIT_ASSERT( it != m.end() ) ;
                CPPUNIT_ASSERT( it->second.m_val == 50 )    ;

                CPPUNIT_ASSERT( m.find(120) == m.end() ) ;
                ensureResult = m.ensure( 120 )   ;
                CPPUNIT_ASSERT( ensureResult.first != m.end() )     ;
                CPPUNIT_ASSERT( ensureResult.second  )     ;
                CPPUNIT_ASSERT( !m.empty() )        ;
                CPPUNIT_ASSERT( check_size( m, 4 )) ;
                ensureResult.first->second.m_val = ensureResult.first->first * 5 ;
                CPPUNIT_ASSERT( m.find_with(120, less()) == ensureResult.first ) ;
                it = m.find_with(120, less())    ;
                CPPUNIT_ASSERT( it != m.end() ) ;
                CPPUNIT_ASSERT( it->second.m_val == 120 * 5 )    ;
                CPPUNIT_ASSERT( m.find_with(120, less()) == m.find(120) ) ;

#           ifdef CDS_EMPLACE_SUPPORT
                // emplace test
                it = m.emplace( 151 ) ;  // key = 151,  val = 0
                CPPUNIT_ASSERT( it != m.end() ) ;
                CPPUNIT_ASSERT( it->first == 151 ) ;
                CPPUNIT_ASSERT( it->second.m_val == 0 ) ;

                it = m.emplace( 174, 471 ) ; // key == 174, val = 471
                CPPUNIT_ASSERT( it != m.end() ) ;
                CPPUNIT_ASSERT( it->first == 174 ) ;
                CPPUNIT_ASSERT( it->second.m_val == 471 ) ;

                it = m.emplace( 190, value_type(91)) ; // key == 190, val = 19
                CPPUNIT_ASSERT( it != m.end() ) ;
                CPPUNIT_ASSERT( it->first == 190 ) ;
                CPPUNIT_ASSERT( it->second.m_val == 91 ) ;

                it = m.emplace( 151, 1051 ) ;
                CPPUNIT_ASSERT( it == m.end()) ;

                it = m.find( 174 )  ;
                CPPUNIT_ASSERT( it != m.end() ) ;
                CPPUNIT_ASSERT( it->first == 174 ) ;
                CPPUNIT_ASSERT( it->second.m_val == 471 ) ;

                it = m.find( 190 )  ;
                CPPUNIT_ASSERT( it != m.end() ) ;
                CPPUNIT_ASSERT( it->first == 190 ) ;
                CPPUNIT_ASSERT( it->second.m_val == 91 ) ;

                it = m.find( 151 )  ;
                CPPUNIT_ASSERT( it != m.end() ) ;
                CPPUNIT_ASSERT( it->first == 151 ) ;
                CPPUNIT_ASSERT( it->second.m_val == 0 ) ;
#           endif
            }

            // iterator test

            {
                Map m( 52, 4 )  ;

                for ( int i = 0; i < 500; ++i ) {
                    CPPUNIT_ASSERT( m.insert( i, i * 2 ) != m.end() )   ;
                }
                CPPUNIT_ASSERT( check_size( m, 500 )) ;

                for ( iterator it = m.begin(), itEnd = m.end(); it != itEnd; ++it ) {
                    CPPUNIT_ASSERT( it->first * 2 == (*it).second.m_val )    ;
                    it->second = it->first ;
                }

                Map const& refMap = m   ;
                for ( const_iterator it = refMap.begin(), itEnd = refMap.end(); it != itEnd; ++it ) {
                    CPPUNIT_ASSERT( it->first == it->second.m_val )    ;
                    CPPUNIT_ASSERT( (*it).first == (*it).second.m_val )    ;
                }
            }
        }

        template <class Map>
        void test_iter()
        {
            typedef typename Map::iterator          iterator ;
            typedef typename Map::const_iterator    const_iterator ;

            Map s( 100, 4 )     ;

            const int nMaxCount = 500   ;
            for ( int i = 0; i < nMaxCount; ++i ) {
                CPPUNIT_ASSERT( s.insert( i, i * 2 ))   ;
            }

            int nCount = 0  ;
            for ( iterator it = s.begin(), itEnd = s.end(); it != itEnd; ++it ) {
                CPPUNIT_ASSERT( it->first * 2 == it->second.m_val )    ;
                CPPUNIT_ASSERT( (*it).first * 2 == (*it).second.m_val )    ;
                it->second.m_val = it->first   ;
                ++nCount    ;
            }
            CPPUNIT_ASSERT( nCount == nMaxCount ) ;

            Map const& refSet = s;
            nCount = 0  ;
            for ( const_iterator it = refSet.begin(), itEnd = refSet.end(); it != itEnd; ++it ) {
                CPPUNIT_ASSERT( it->first == it->second.m_val )    ;
                CPPUNIT_ASSERT( (*it).first == (*it).second.m_val )    ;
                ++nCount    ;
            }
            CPPUNIT_ASSERT( nCount == nMaxCount ) ;
        }


        void Michael_HP_cmp()          ;
        void Michael_HP_less()         ;
        void Michael_HP_cmpmix()       ;

        void Michael_PTB_cmp()          ;
        void Michael_PTB_less()         ;
        void Michael_PTB_cmpmix()       ;

        void Michael_HRC_cmp()          ;
        void Michael_HRC_less()         ;
        void Michael_HRC_cmpmix()       ;

        void Michael_RCU_GPI_cmp()          ;
        void Michael_RCU_GPI_less()         ;
        void Michael_RCU_GPI_cmpmix()       ;

        void Michael_RCU_GPB_cmp()          ;
        void Michael_RCU_GPB_less()         ;
        void Michael_RCU_GPB_cmpmix()       ;

        void Michael_RCU_GPT_cmp()          ;
        void Michael_RCU_GPT_less()         ;
        void Michael_RCU_GPT_cmpmix()       ;

        void Michael_RCU_SHB_cmp()          ;
        void Michael_RCU_SHB_less()         ;
        void Michael_RCU_SHB_cmpmix()       ;

        void Michael_RCU_SHT_cmp()          ;
        void Michael_RCU_SHT_less()         ;
        void Michael_RCU_SHT_cmpmix()       ;

        void Michael_nogc_cmp()          ;
        void Michael_nogc_less()         ;
        void Michael_nogc_cmpmix()       ;

        void Lazy_HP_cmp()          ;
        void Lazy_HP_less()         ;
        void Lazy_HP_cmpmix()       ;

        void Lazy_PTB_cmp()          ;
        void Lazy_PTB_less()         ;
        void Lazy_PTB_cmpmix()       ;

        void Lazy_HRC_cmp()          ;
        void Lazy_HRC_less()         ;
        void Lazy_HRC_cmpmix()       ;

        void Lazy_RCU_GPI_cmp()          ;
        void Lazy_RCU_GPI_less()         ;
        void Lazy_RCU_GPI_cmpmix()       ;

        void Lazy_RCU_GPB_cmp()          ;
        void Lazy_RCU_GPB_less()         ;
        void Lazy_RCU_GPB_cmpmix()       ;

        void Lazy_RCU_GPT_cmp()          ;
        void Lazy_RCU_GPT_less()         ;
        void Lazy_RCU_GPT_cmpmix()       ;

        void Lazy_RCU_SHB_cmp()          ;
        void Lazy_RCU_SHB_less()         ;
        void Lazy_RCU_SHB_cmpmix()       ;

        void Lazy_RCU_SHT_cmp()          ;
        void Lazy_RCU_SHT_less()         ;
        void Lazy_RCU_SHT_cmpmix()       ;

        void Lazy_nogc_cmp()          ;
        void Lazy_nogc_less()         ;
        void Lazy_nogc_cmpmix()       ;

        void Split_HP_cmp()          ;
        void Split_HP_less()         ;
        void Split_HP_cmpmix()       ;

        void Split_PTB_cmp()          ;
        void Split_PTB_less()         ;
        void Split_PTB_cmpmix()       ;

        void Split_HRC_cmp()          ;
        void Split_HRC_less()         ;
        void Split_HRC_cmpmix()       ;

        void Split_RCU_GPI_cmp()          ;
        void Split_RCU_GPI_less()         ;
        void Split_RCU_GPI_cmpmix()       ;

        void Split_RCU_GPB_cmp()          ;
        void Split_RCU_GPB_less()         ;
        void Split_RCU_GPB_cmpmix()       ;

        void Split_RCU_GPT_cmp()          ;
        void Split_RCU_GPT_less()         ;
        void Split_RCU_GPT_cmpmix()       ;

        void Split_RCU_SHB_cmp()          ;
        void Split_RCU_SHB_less()         ;
        void Split_RCU_SHB_cmpmix()       ;

        void Split_RCU_SHT_cmp()          ;
        void Split_RCU_SHT_less()         ;
        void Split_RCU_SHT_cmpmix()       ;

        void Split_nogc_cmp()          ;
        void Split_nogc_less()         ;
        void Split_nogc_cmpmix()       ;

        void Split_Lazy_HP_cmp()          ;
        void Split_Lazy_HP_less()         ;
        void Split_Lazy_HP_cmpmix()       ;

        void Split_Lazy_PTB_cmp()          ;
        void Split_Lazy_PTB_less()         ;
        void Split_Lazy_PTB_cmpmix()       ;

        void Split_Lazy_HRC_cmp()          ;
        void Split_Lazy_HRC_less()         ;
        void Split_Lazy_HRC_cmpmix()       ;

        void Split_Lazy_RCU_GPI_cmp()          ;
        void Split_Lazy_RCU_GPI_less()         ;
        void Split_Lazy_RCU_GPI_cmpmix()       ;

        void Split_Lazy_RCU_GPB_cmp()          ;
        void Split_Lazy_RCU_GPB_less()         ;
        void Split_Lazy_RCU_GPB_cmpmix()       ;

        void Split_Lazy_RCU_GPT_cmp()          ;
        void Split_Lazy_RCU_GPT_less()         ;
        void Split_Lazy_RCU_GPT_cmpmix()       ;

        void Split_Lazy_RCU_SHB_cmp()          ;
        void Split_Lazy_RCU_SHB_less()         ;
        void Split_Lazy_RCU_SHB_cmpmix()       ;

        void Split_Lazy_RCU_SHT_cmp()          ;
        void Split_Lazy_RCU_SHT_less()         ;
        void Split_Lazy_RCU_SHT_cmpmix()       ;

        void Split_Lazy_nogc_cmp()          ;
        void Split_Lazy_nogc_less()         ;
        void Split_Lazy_nogc_cmpmix()       ;

        CPPUNIT_TEST_SUITE(HashMapHdrTest)
            CPPUNIT_TEST(Michael_HP_cmp)
            CPPUNIT_TEST(Michael_HP_less)
            CPPUNIT_TEST(Michael_HP_cmpmix)

            CPPUNIT_TEST(Michael_PTB_cmp)
            CPPUNIT_TEST(Michael_PTB_less)
            CPPUNIT_TEST(Michael_PTB_cmpmix)

            CPPUNIT_TEST(Michael_HRC_cmp)
            CPPUNIT_TEST(Michael_HRC_less)
            CPPUNIT_TEST(Michael_HRC_cmpmix)

            CPPUNIT_TEST(Michael_RCU_GPI_cmp)
            CPPUNIT_TEST(Michael_RCU_GPI_less)
            CPPUNIT_TEST(Michael_RCU_GPI_cmpmix)

            CPPUNIT_TEST(Michael_RCU_GPB_cmp)
            CPPUNIT_TEST(Michael_RCU_GPB_less)
            CPPUNIT_TEST(Michael_RCU_GPB_cmpmix)

            CPPUNIT_TEST(Michael_RCU_SHT_cmp)
            CPPUNIT_TEST(Michael_RCU_SHT_less)
            CPPUNIT_TEST(Michael_RCU_SHT_cmpmix)

            CPPUNIT_TEST(Michael_RCU_SHB_cmp)
            CPPUNIT_TEST(Michael_RCU_SHB_less)
            CPPUNIT_TEST(Michael_RCU_SHB_cmpmix)

            CPPUNIT_TEST(Michael_RCU_GPT_cmp)
            CPPUNIT_TEST(Michael_RCU_GPT_less)
            CPPUNIT_TEST(Michael_RCU_GPT_cmpmix)

            CPPUNIT_TEST(Michael_nogc_cmp)
            CPPUNIT_TEST(Michael_nogc_less)
            CPPUNIT_TEST(Michael_nogc_cmpmix)

            CPPUNIT_TEST(Lazy_HP_cmp)
            CPPUNIT_TEST(Lazy_HP_less)
            CPPUNIT_TEST(Lazy_HP_cmpmix)

            CPPUNIT_TEST(Lazy_PTB_cmp)
            CPPUNIT_TEST(Lazy_PTB_less)
            CPPUNIT_TEST(Lazy_PTB_cmpmix)

            CPPUNIT_TEST(Lazy_HRC_cmp)
            CPPUNIT_TEST(Lazy_HRC_less)
            CPPUNIT_TEST(Lazy_HRC_cmpmix)

            CPPUNIT_TEST(Lazy_RCU_GPI_cmp)
            CPPUNIT_TEST(Lazy_RCU_GPI_less)
            CPPUNIT_TEST(Lazy_RCU_GPI_cmpmix)

            CPPUNIT_TEST(Lazy_RCU_GPB_cmp)
            CPPUNIT_TEST(Lazy_RCU_GPB_less)
            CPPUNIT_TEST(Lazy_RCU_GPB_cmpmix)

            CPPUNIT_TEST(Lazy_RCU_GPT_cmp)
            CPPUNIT_TEST(Lazy_RCU_GPT_less)
            CPPUNIT_TEST(Lazy_RCU_GPT_cmpmix)

            CPPUNIT_TEST(Lazy_RCU_SHB_cmp)
            CPPUNIT_TEST(Lazy_RCU_SHB_less)
            CPPUNIT_TEST(Lazy_RCU_SHB_cmpmix)

            CPPUNIT_TEST(Lazy_RCU_SHT_cmp)
            CPPUNIT_TEST(Lazy_RCU_SHT_less)
            CPPUNIT_TEST(Lazy_RCU_SHT_cmpmix)

            CPPUNIT_TEST(Lazy_nogc_cmp)
            CPPUNIT_TEST(Lazy_nogc_less)
            CPPUNIT_TEST(Lazy_nogc_cmpmix)

            CPPUNIT_TEST(Split_HP_cmp)
            CPPUNIT_TEST(Split_HP_less)
            CPPUNIT_TEST(Split_HP_cmpmix)

            CPPUNIT_TEST(Split_PTB_cmp)
            CPPUNIT_TEST(Split_PTB_less)
            CPPUNIT_TEST(Split_PTB_cmpmix)

            CPPUNIT_TEST(Split_HRC_cmp)
            CPPUNIT_TEST(Split_HRC_less)
            CPPUNIT_TEST(Split_HRC_cmpmix)

            CPPUNIT_TEST(Split_RCU_GPI_cmp)
            CPPUNIT_TEST(Split_RCU_GPI_less)
            CPPUNIT_TEST(Split_RCU_GPI_cmpmix)

            CPPUNIT_TEST(Split_RCU_GPB_cmp)
            CPPUNIT_TEST(Split_RCU_GPB_less)
            CPPUNIT_TEST(Split_RCU_GPB_cmpmix)

            CPPUNIT_TEST(Split_RCU_GPT_cmp)
            CPPUNIT_TEST(Split_RCU_GPT_less)
            CPPUNIT_TEST(Split_RCU_GPT_cmpmix)

            CPPUNIT_TEST(Split_RCU_SHB_cmp)
            CPPUNIT_TEST(Split_RCU_SHB_less)
            CPPUNIT_TEST(Split_RCU_SHB_cmpmix)

            CPPUNIT_TEST(Split_RCU_SHT_cmp)
            CPPUNIT_TEST(Split_RCU_SHT_less)
            CPPUNIT_TEST(Split_RCU_SHT_cmpmix)

            CPPUNIT_TEST(Split_nogc_cmp)
            CPPUNIT_TEST(Split_nogc_less)
            CPPUNIT_TEST(Split_nogc_cmpmix)

            CPPUNIT_TEST(Split_Lazy_HP_cmp)
            CPPUNIT_TEST(Split_Lazy_HP_less)
            CPPUNIT_TEST(Split_Lazy_HP_cmpmix)

            CPPUNIT_TEST(Split_Lazy_PTB_cmp)
            CPPUNIT_TEST(Split_Lazy_PTB_less)
            CPPUNIT_TEST(Split_Lazy_PTB_cmpmix)

            CPPUNIT_TEST(Split_Lazy_HRC_cmp)
            CPPUNIT_TEST(Split_Lazy_HRC_less)
            CPPUNIT_TEST(Split_Lazy_HRC_cmpmix)

            CPPUNIT_TEST(Split_Lazy_RCU_GPI_cmp)
            CPPUNIT_TEST(Split_Lazy_RCU_GPI_less)
            CPPUNIT_TEST(Split_Lazy_RCU_GPI_cmpmix)

            CPPUNIT_TEST(Split_Lazy_RCU_GPB_cmp)
            CPPUNIT_TEST(Split_Lazy_RCU_GPB_less)
            CPPUNIT_TEST(Split_Lazy_RCU_GPB_cmpmix)

            CPPUNIT_TEST(Split_Lazy_RCU_GPT_cmp)
            CPPUNIT_TEST(Split_Lazy_RCU_GPT_less)
            CPPUNIT_TEST(Split_Lazy_RCU_GPT_cmpmix)

            CPPUNIT_TEST(Split_Lazy_RCU_SHB_cmp)
            CPPUNIT_TEST(Split_Lazy_RCU_SHB_less)
            CPPUNIT_TEST(Split_Lazy_RCU_SHB_cmpmix)

            CPPUNIT_TEST(Split_Lazy_RCU_SHT_cmp)
            CPPUNIT_TEST(Split_Lazy_RCU_SHT_less)
            CPPUNIT_TEST(Split_Lazy_RCU_SHT_cmpmix)

            CPPUNIT_TEST(Split_Lazy_nogc_cmp)
            CPPUNIT_TEST(Split_Lazy_nogc_less)
            CPPUNIT_TEST(Split_Lazy_nogc_cmpmix)
        CPPUNIT_TEST_SUITE_END()

    };
}   // namespace map

#endif // #ifndef __CDSTEST_HDR_MAP_H
