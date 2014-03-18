//$$CDS-header$$

#ifndef __CDS_INTRUSIVE_STRIPED_SET_ADAPTER_H
#define __CDS_INTRUSIVE_STRIPED_SET_ADAPTER_H

#include <cds/opt/options.h>
#include <cds/intrusive/striped_set/resizing_policy.h>
#include <cds/opt/hash.h>
#include <cds/opt/compare.h>    // cds::opt::details::make_comparator - for some adapt specializations
#include <cds/ref.h>

namespace cds { namespace intrusive {

    /// StripedSet related definitions
    namespace striped_set {

        /// Default adapter for intrusive striped/refinable hash set
        /**
            By default, the metafunction does not make any transformation for container type \p Container.
            \p Container should provide interface suitable for the hash set.

            The \p SetOptions template argument contains option pack
            that has been passed to cds::intrusive::StripedSet.

        <b>Bucket interface</b>

            The result of metafunction is a container (a bucket) that should support the following interface:

            Public typedefs that the bucket should provide:
                - \p value_type - the type of the item in the bucket
                - \p iterator - bucket's item iterator
                - \p const_iterator - bucket's item constant iterator
                - \p default_resizing_policy - default resizing policy preferable for the container.
                    By default, the library defines cds::container::striped_set::load_factor_resizing<4> for sequential containers like
                    boost::intrusive::list,  and cds::container::striped_set::no_resizing for ordered container like boost::intrusive::set.

            <b>Insert value \p val of type \p Q</b>
            \code template <typename Func> bool insert( value_type& val, Func f ) ; \endcode
                Inserts \p val into the container and, if inserting is successful, calls functor \p f
                with \p val.

                The functor signature is:
                \code
                struct functor {
                    void operator()( value_type& item ) ;
                };
                \endcode
                where \p item is the item inserted.

                The user-defined functor \p f is called only if the inserting is success. It can be passed by reference
                using <tt>boost::ref</tt>
                <hr>

            <b>Ensures that the \p item exists in the container</b>
            \code template <typename Func> std::pair<bool, bool> ensure( value_type& val, Func f ) \endcode
                The operation performs inserting or changing data.

                If the \p val key not found in the container, then \p val is inserted.
                Otherwise, the functor \p f is called with the item found.

                The \p Func functor has the following interface:
                \code
                    void func( bool bNew, value_type& item, value_type& val ) ;
                \endcode
                or like a functor:
                \code
                    struct functor {
                        void operator()( bool bNew, value_type& item, value_type& val ) ;
                    };
                \endcode

                where arguments are:
                - \p bNew - \p true if the item has been inserted, \p false otherwise
                - \p item - container's item
                - \p val - argument \p val passed into the \p ensure function

                If \p val has been inserted (i.e. <tt>bNew == true</tt>) then \p item and \p val
                are the same element: <tt>&item == &val</tt>. Otherwise, they are different.

                The functor can change non-key fields of the \p item.

                You can pass \p f argument by reference using <tt>boost::ref</tt>.

                Returns <tt> std::pair<bool, bool> </tt> where \p first is true if operation is successfull,
                \p second is true if new item has been added or \p false if the item with \p val key
                already exists.
                <hr>

            <b>Unlink an item</b>
            \code bool unlink( value_type& val ) \endcode
                Unlink \p val from the container if \p val belongs to it.
                <hr>

            <b>Erase \p key</b>
            \code template <typename Q, typename Func> bool erase( Q const& key, Func f ) \endcode
                The function searches an item with key \p key, calls \p f functor
                and erases the item. If \p key is not found, the functor is not called.

                The functor \p Func interface is:
                \code
                struct functor {
                    void operator()(value_type& val) ;
                };
                \endcode
                The functor can be passed by reference using <tt>boost:ref</tt>

                The type \p Q can differ from \ref value_type of items storing in the container.
                Therefore, the \p value_type should be comparable with type \p Q.

                Return \p true if key is found and deleted, \p false otherwise
                <hr>


            <b>Find the key \p val </b>
            \code
            template <typename Q, typename Func> bool find( Q& val, Func f )
            template <typename Q, typename Compare, typename Func> bool find( Q& val, Compare cmp, Func f )
            \endcode
                The function searches the item with key equal to \p val and calls the functor \p f for item found.
                The interface of \p Func functor is:
                \code
                struct functor {
                    void operator()( value_type& item, Q& val ) ;
                };
                \endcode
                where \p item is the item found, \p val is the <tt>find</tt> function argument.

                You can pass \p f argument by reference using <tt>boost::ref</tt> or cds::ref.

                The functor can change non-key fields of \p item.
                The \p val argument may be non-const since it can be used as \p f functor destination i.e., the functor
                can modify both arguments.

                The type \p Q can differ from \ref value_type of items storing in the container.
                Therefore, the \p value_type should be comparable with type \p Q.

                The first form uses default \p compare function used for key ordering.
                The second form allows to point specific \p Compare functor \p cmp
                that can compare \p value_typwe and \p Q type. The interface of \p Compare is the same as \p std::less.

                The function returns \p true if \p val is found, \p false otherwise.
                <hr>

            <b>Clears the container</b>
            \code
            void clear()
            template <typename Disposer> void clear( Disposer disposer )
            \endcode
            Second form calls \p disposer for each item in the container before clearing.
            <hr>

            <b>Get size of bucket</b>
            \code size_t size() const \endcode
            This function may be required by some resizing policy
            <hr>

            <b>Iterators</b>
            \code
            iterator begin();
            const_iterator begin() const;
            iterator end();
            const_iterator end() const;
            \endcode
            <hr>

            <b>Move item when resizing</b>
            \code void move_item( adapted_container& from, iterator it ) \endcode
                This helper function is invented for the set resizing when the item
                pointed by \p it iterator is copied from old bucket \p from to a new bucket
                pointed by \p this.
            <hr>

        */
        template < typename Container, CDS_DECL_OPTIONS >
        class adapt
        {
        public:
            typedef Container   type            ;   ///< adapted container type
            typedef typename type::value_type value_type  ;   ///< value type stored in the container
        };

        //@cond
        struct adapted_sequential_container
        {
            typedef striped_set::load_factor_resizing<4>   default_resizing_policy ;
        };

        struct adapted_container
        {
            typedef striped_set::no_resizing   default_resizing_policy ;
        };
        //@endcond

        //@cond
        namespace details {
            template <typename Set>
            class boost_intrusive_set_adapter: public cds::intrusive::striped_set::adapted_container
            {
            public:
                typedef Set container_type ;

                typedef typename container_type::value_type     value_type      ;   ///< value type stored in the container
                typedef typename container_type::iterator       iterator        ;   ///< container iterator
                typedef typename container_type::const_iterator const_iterator  ;   ///< container const iterator

                typedef typename container_type::value_compare  key_comparator ;

            private:
#       ifndef CDS_CXX11_LAMBDA_SUPPORT
                struct empty_insert_functor {
                    void operator()( value_type& )
                    {}
                };
#       endif

                container_type  m_Set  ;

            public:
                boost_intrusive_set_adapter()
                {}

                container_type& base_container()
                {
                    return m_Set ;
                }

                template <typename Func>
                bool insert( value_type& val, Func f )
                {
                    std::pair<iterator, bool> res = m_Set.insert( val ) ;
                    if ( res.second )
                        cds::unref(f)( val )  ;
                    return res.second   ;
                }

                template <typename Func>
                std::pair<bool, bool> ensure( value_type& val, Func f )
                {
                    std::pair<iterator, bool> res = m_Set.insert( val ) ;
                    cds::unref(f)( res.second, *res.first, val )     ;
                    return std::make_pair( true, res.second )   ;
                }

                bool unlink( value_type& val )
                {
                    iterator it = m_Set.find( value_type(val) ) ;
                    if ( it == m_Set.end() || &(*it) != &val )
                        return false ;
                    m_Set.erase( it )       ;
                    return true ;
                }

                template <typename Q, typename Func>
                value_type * erase( Q const& key, Func f )
                {
                    iterator it = m_Set.find( key, key_comparator() ) ;
                    if ( it == m_Set.end() )
                        return null_ptr<value_type *>() ;
                    value_type& val = *it   ;
                    cds::unref(f)( val )    ;
                    m_Set.erase( it )       ;
                    return &val ;
                }

                template <typename Q, typename Less, typename Func>
                value_type * erase( Q const& key, Less pred, Func f )
                {
                    iterator it = m_Set.find( key, pred ) ;
                    if ( it == m_Set.end() )
                        return null_ptr<value_type *>() ;
                    value_type& val = *it   ;
                    cds::unref(f)( val )    ;
                    m_Set.erase( it )       ;
                    return &val ;
                }

                template <typename Q, typename Func>
                bool find( Q& key, Func f )
                {
                    return find( key, key_comparator(), f ) ;
                }

                template <typename Q, typename Compare, typename Func>
                bool find( Q& key, Compare cmp, Func f )
                {
                    iterator it = m_Set.find( key, cmp ) ;
                    if ( it == m_Set.end() )
                        return false ;
                    cds::unref(f)( *it, key ) ;
                    return true ;
                }

                void clear()
                {
                    m_Set.clear()    ;
                }

                template <typename Disposer>
                void clear( Disposer disposer )
                {
                    m_Set.clear_and_dispose( disposer )    ;
                }

                iterator begin()                { return m_Set.begin(); }
                const_iterator begin() const    { return m_Set.begin(); }
                iterator end()                  { return m_Set.end(); }
                const_iterator end() const      { return m_Set.end(); }

                size_t size() const
                {
                    return (size_t) m_Set.size() ;
                }

                void move_item( boost_intrusive_set_adapter& from, iterator itWhat )
                {
                    value_type& val = *itWhat ;
                    from.base_container().erase( itWhat )    ;
#           ifdef CDS_CXX11_LAMBDA_SUPPORT
                    insert( val, []( value_type& ) {} )    ;
#           else
                    insert( val, empty_insert_functor() )    ;
#           endif
                }
            };
        }   // namespace details
        //@endcond

    } // namespace striped_set
}} // namespace cds::intrusive

//@cond
#if defined(CDS_CXX11_VARIADIC_TEMPLATE_SUPPORT) && defined(BOOST_INTRUSIVE_VARIADIC_TEMPLATES)
#   define CDS_BOOST_INTRUSIVE_DECL_OPTIONS3    typename... BIOptions
#   define CDS_BOOST_INTRUSIVE_DECL_OPTIONS4    typename... BIOptions
#   define CDS_BOOST_INTRUSIVE_DECL_OPTIONS5    typename... BIOptions
#   define CDS_BOOST_INTRUSIVE_DECL_OPTIONS6    typename... BIOptions
#   define CDS_BOOST_INTRUSIVE_DECL_OPTIONS7    typename... BIOptions
#   define CDS_BOOST_INTRUSIVE_DECL_OPTIONS8    typename... BIOptions
#   define CDS_BOOST_INTRUSIVE_DECL_OPTIONS9    typename... BIOptions
#   define CDS_BOOST_INTRUSIVE_DECL_OPTIONS10    typename... BIOptions

#   define CDS_BOOST_INTRUSIVE_OPTIONS3    BIOptions...
#   define CDS_BOOST_INTRUSIVE_OPTIONS4    BIOptions...
#   define CDS_BOOST_INTRUSIVE_OPTIONS5    BIOptions...
#   define CDS_BOOST_INTRUSIVE_OPTIONS6    BIOptions...
#   define CDS_BOOST_INTRUSIVE_OPTIONS7    BIOptions...
#   define CDS_BOOST_INTRUSIVE_OPTIONS8    BIOptions...
#   define CDS_BOOST_INTRUSIVE_OPTIONS9    BIOptions...
#   define CDS_BOOST_INTRUSIVE_OPTIONS10    BIOptions...
#else
#   define CDS_BOOST_INTRUSIVE_DECL_OPTIONS3    typename BIO1, typename BIO2, typename BIO3
#   define CDS_BOOST_INTRUSIVE_DECL_OPTIONS4    CDS_BOOST_INTRUSIVE_DECL_OPTIONS3, typename BIO4
#   define CDS_BOOST_INTRUSIVE_DECL_OPTIONS5    CDS_BOOST_INTRUSIVE_DECL_OPTIONS4, typename BIO5
#   define CDS_BOOST_INTRUSIVE_DECL_OPTIONS6    CDS_BOOST_INTRUSIVE_DECL_OPTIONS5, typename BIO6
#   define CDS_BOOST_INTRUSIVE_DECL_OPTIONS7    CDS_BOOST_INTRUSIVE_DECL_OPTIONS6, typename BIO7
#   define CDS_BOOST_INTRUSIVE_DECL_OPTIONS8    CDS_BOOST_INTRUSIVE_DECL_OPTIONS7, typename BIO8
#   define CDS_BOOST_INTRUSIVE_DECL_OPTIONS9    CDS_BOOST_INTRUSIVE_DECL_OPTIONS8, typename BIO9
#   define CDS_BOOST_INTRUSIVE_DECL_OPTIONS10   CDS_BOOST_INTRUSIVE_DECL_OPTIONS9, typename BIO10

#   define CDS_BOOST_INTRUSIVE_OPTIONS3    BIO1,BIO2,BIO3
#   define CDS_BOOST_INTRUSIVE_OPTIONS4    CDS_BOOST_INTRUSIVE_OPTIONS3, BIO4
#   define CDS_BOOST_INTRUSIVE_OPTIONS5    CDS_BOOST_INTRUSIVE_OPTIONS4, BIO5
#   define CDS_BOOST_INTRUSIVE_OPTIONS6    CDS_BOOST_INTRUSIVE_OPTIONS5, BIO6
#   define CDS_BOOST_INTRUSIVE_OPTIONS7    CDS_BOOST_INTRUSIVE_OPTIONS6, BIO7
#   define CDS_BOOST_INTRUSIVE_OPTIONS8    CDS_BOOST_INTRUSIVE_OPTIONS7, BIO8
#   define CDS_BOOST_INTRUSIVE_OPTIONS9    CDS_BOOST_INTRUSIVE_OPTIONS8, BIO9
#   define CDS_BOOST_INTRUSIVE_OPTIONS10   CDS_BOOST_INTRUSIVE_OPTIONS9, BIO10
#endif
//@endcond

#endif // #ifndef __CDS_INTRUSIVE_STRIPED_SET_ADAPTER_H
