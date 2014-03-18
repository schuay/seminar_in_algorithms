//$$CDS-header$$

#ifndef __CDS_INTRUSIVE_MICHAEL_SET_BASE_H
#define __CDS_INTRUSIVE_MICHAEL_SET_BASE_H

#include <cds/intrusive/base.h>
#include <cds/opt/compare.h>
#include <cds/opt/hash.h>
#include <cds/bitop.h>
#include <cds/cxx11_atomic.h>
#include <cds/ref.h>

namespace cds { namespace intrusive {

    /// MichaelHashSet related definitions
    /** @ingroup cds_intrusive_helper
    */
    namespace michael_set {

        /// Type traits for MichaelHashSet class
        struct type_traits {
            /// Hash function
            /**
                Hash function converts the key fields of struct \p T stored in the hash-set
                into value of type \p size_t called hash value that is an index of hash table.

                This is mandatory type and has no predefined one.
            */
            typedef opt::none       hash    ;

            /// Item counter
            /**
                The item counting is an important part of MichaelHashSet algorithm:
                the <tt>empty()</tt> member function depends on correct item counting.
                Therefore, atomicity::empty_item_counter is not allowed as a type of the option.

                Default is atomicity::item_counter.
            */
            typedef atomicity::item_counter     item_counter;

            /// Bucket table allocator
            /**
                Allocator for bucket table. Default is \ref CDS_DEFAULT_ALLOCATOR
                The allocator uses only in ctor (for allocating bucket table)
                and in dtor (for destroying bucket table)
            */
            typedef CDS_DEFAULT_ALLOCATOR   allocator   ;
        };

        /// Metafunction converting option list to traits struct
        /**
            This is a wrapper for <tt> cds::opt::make_options< type_traits, Options...> </tt>

            Available \p Options:
            - opt::hash - mandatory option, specifies hash functor.
            - opt::item_counter - optional, specifies item counting policy. See type_traits::item_counter
                for default type.
            - opt::allocator - optional, bucket table allocator. Default is \ref CDS_DEFAULT_ALLOCATOR.

            See \ref MichaelHashSet, \ref type_traits.
        */
        template <CDS_DECL_OPTIONS3>
        struct make_traits {
            typedef typename cds::opt::make_options< type_traits, CDS_OPTIONS3>::type type  ;   ///< Result of metafunction
        };

        //@cond
        namespace details {
            static inline size_t init_hash_bitmask( size_t nMaxItemCount, size_t nLoadFactor )
            {
                if ( nLoadFactor == 0 )
                    nLoadFactor = 1     ;
                if ( nMaxItemCount == 0 )
                    nMaxItemCount = 4   ;
                const size_t nBucketCount = (size_t)( nMaxItemCount / nLoadFactor )   ;
                const size_t nLog2 = cds::bitop::MSB( nBucketCount )   ;

                return (( size_t( 1 << nLog2 ) < nBucketCount ? size_t( 1 << (nLog2 + 1) ) : size_t( 1 << nLog2 ))) - 1 ;
            }

            template <typename OrderedList, bool IsConst>
            struct list_iterator_selector   ;

            template <typename OrderedList>
            struct list_iterator_selector< OrderedList, false>
            {
                typedef OrderedList * bucket_ptr               ;
                typedef typename OrderedList::iterator  type    ;
            };

            template <typename OrderedList>
            struct list_iterator_selector< OrderedList, true>
            {
                typedef OrderedList const * bucket_ptr              ;
                typedef typename OrderedList::const_iterator  type  ;
            };

            template <typename OrderedList, bool IsConst>
            class iterator
            {
            protected:
                typedef OrderedList bucket_type ;
                typedef typename list_iterator_selector< bucket_type, IsConst>::bucket_ptr bucket_ptr ;
                typedef typename list_iterator_selector< bucket_type, IsConst>::type list_iterator   ;

                bucket_ptr      m_pCurBucket    ;
                list_iterator   m_itList        ;
                bucket_ptr      m_pEndBucket    ;

                void next()
                {
                    if ( m_pCurBucket < m_pEndBucket ) {
                        if ( ++m_itList != m_pCurBucket->end() )
                            return  ;
                        while ( ++m_pCurBucket < m_pEndBucket ) {
                            m_itList = m_pCurBucket->begin()    ;
                            if ( m_itList != m_pCurBucket->end() )
                                return  ;
                        }
                    }
                    m_pCurBucket = m_pEndBucket - 1 ;
                    m_itList = m_pCurBucket->end()  ;
                }

            public:
                typedef typename list_iterator::value_ptr   value_ptr   ;
                typedef typename list_iterator::value_ref   value_ref   ;

            public:
                iterator()
                    : m_pCurBucket( null_ptr<bucket_ptr>() )
                    , m_itList()
                    , m_pEndBucket( null_ptr<bucket_ptr>() )
                {}

                iterator( list_iterator const& it, bucket_ptr pFirst, bucket_ptr pLast )
                    : m_pCurBucket( pFirst )
                    , m_itList( it )
                    , m_pEndBucket( pLast )
                {
                    if ( it == pFirst->end() )
                        next()  ;
                }

                iterator( iterator const& src )
                    : m_pCurBucket( src.m_pCurBucket )
                    , m_itList( src.m_itList )
                    , m_pEndBucket( src.m_pEndBucket )
                {}

                value_ptr operator ->() const
                {
                    assert( m_pCurBucket != null_ptr<bucket_ptr>() )  ;
                    return m_itList.operator ->()   ;
                }

                value_ref operator *() const
                {
                    assert( m_pCurBucket != null_ptr<bucket_ptr>() )  ;
                    return m_itList.operator *()    ;
                }

                /// Pre-increment
                iterator& operator ++()
                {
                    next()  ;
                    return *this;
                }

                iterator& operator = (const iterator& src)
                {
                    m_pCurBucket = src.m_pCurBucket ;
                    m_pEndBucket = src.m_pEndBucket ;
                    m_itList = src.m_itList    ;
                    return *this    ;
                }

                bucket_ptr bucket() const
                {
                    return m_pCurBucket != m_pEndBucket ? m_pCurBucket : null_ptr<bucket_ptr>()   ;
                }

                template <bool C>
                bool operator ==(iterator<OrderedList, C> const& i ) const
                {
                    return m_pCurBucket == i.m_pCurBucket && m_itList == i.m_itList ;
                }
                template <bool C>
                bool operator !=(iterator<OrderedList, C> const& i ) const
                {
                    return !( *this == i )  ;
                }

            };
        }
        //@endcond
    }

    //@cond
    // Forward declarations
    template <class GC, class OrderedList, class Traits = michael_set::type_traits>
    class MichaelHashSet    ;
    //@endcond

}} // namespace cds::intrusive

#endif // #ifndef __CDS_INTRUSIVE_MICHAEL_SET_BASE_H
