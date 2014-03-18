//$$CDS-header$$

#ifndef __CDS_CONTAINER_LAZY_LIST_NOGC_H
#define __CDS_CONTAINER_LAZY_LIST_NOGC_H

#include <cds/container/lazy_list_base.h>
#include <cds/intrusive/lazy_list_nogc.h>
#include <cds/container/details/make_lazy_list.h>
#include <cds/details/std/memory.h>

namespace cds { namespace container {

    //@cond
    namespace details {

        template <typename T, class Traits>
        struct make_lazy_list_nogc: public make_lazy_list<gc::nogc, T, Traits>
        {
            typedef make_lazy_list<cds::gc::nogc, T, Traits>  base_maker  ;
            typedef typename base_maker::node_type node_type    ;

            struct type_traits: public base_maker::type_traits
            {
                typedef typename base_maker::node_deallocator    disposer    ;
            };

            typedef intrusive::LazyList<cds::gc::nogc, node_type, type_traits>  type    ;
        };

    }   // namespace details
    //@endcond

    /// Lazy ordered single-linked list (template specialization for gc::nogc)
    /** @ingroup cds_nonintrusive_list
        \anchor cds_nonintrusive_LazyList_nogc

        This specialization is intended for so-called persistent usage when no item
        reclamation may be performed. The class does not support deleting of list item.

        Usually, ordered single-linked list is used as a building block for the hash table implementation.
        The complexity of searching is <tt>O(N)</tt>.

        See \ref cds_nonintrusive_LazyList_gc "LazyList" for description of template parameters.

        The interface of the specialization is a little different.
    */
    template <typename T, typename Traits>
    class LazyList<gc::nogc, T, Traits>:
#ifdef CDS_DOXYGEN_INVOKED
        protected intrusive::LazyList< gc::nogc, T, Traits >
#else
        protected details::make_lazy_list_nogc< T, Traits >::type
#endif
    {
        //@cond
        typedef details::make_lazy_list_nogc< T, Traits > options ;
        typedef typename options::type  base_class   ;
        //@endcond

    public:
        typedef T                                   value_type      ;   ///< Type of value stored in the list
        typedef typename base_class::gc             gc              ;   ///< Garbage collector used
        typedef typename base_class::back_off       back_off        ;   ///< Back-off strategy used
        typedef typename options::allocator_type    allocator_type  ;   ///< Allocator type used for allocate/deallocate the nodes
        typedef typename base_class::item_counter   item_counter    ;   ///< Item counting policy used
        typedef typename options::key_comparator    key_comparator  ;   ///< key comparison functor
        typedef typename base_class::memory_model   memory_model    ;   ///< Memory ordering. See cds::opt::memory_model option

    protected:
        //@cond
        typedef typename base_class::value_type     node_type       ;
        typedef typename options::cxx_allocator     cxx_allocator   ;
        typedef typename options::node_deallocator  node_deallocator;
        typedef typename options::type_traits::compare  intrusive_key_comparator    ;

        typedef typename base_class::node_type      head_type       ;
        //@endcond

    protected:
        //@cond
#   ifndef CDS_CXX11_LAMBDA_SUPPORT
        struct ensure_functor
        {
            node_type * m_pItemFound    ;

            ensure_functor()
                : m_pItemFound( null_ptr<node_type *>() )
            {}

            void operator ()(bool, node_type& item, node_type& )
            {
                m_pItemFound = &item    ;
            }
        };
#   endif
        //@endcond

    protected:
        //@cond
        static node_type * alloc_node()
        {
            return cxx_allocator().New()    ;
        }

        static node_type * alloc_node( value_type const& v )
        {
            return cxx_allocator().New( v ) ;
        }

#   ifdef CDS_EMPLACE_SUPPORT
        template <typename... Args>
        static node_type * alloc_node( Args&&... args )
        {
            return cxx_allocator().MoveNew( std::forward<Args>(args)... ) ;
        }
#   endif

        static void free_node( node_type * pNode )
        {
            cxx_allocator().Delete( pNode ) ;
        }

        struct node_disposer {
            void operator()( node_type * pNode )
            {
                free_node( pNode )  ;
            }
        };
        typedef std::unique_ptr< node_type, node_disposer >     scoped_node_ptr ;

        head_type& head()
        {
            return base_class::m_Head  ;
        }

        head_type const& head() const
        {
            return base_class::m_Head  ;
        }

        head_type& tail()
        {
            return base_class::m_Tail  ;
        }

        head_type const& tail() const
        {
            return base_class::m_Tail  ;
        }
        //@endcond

    protected:
        //@cond
        template <bool IsConst>
        class iterator_type: protected base_class::template iterator_type<IsConst>
        {
            typedef typename base_class::template iterator_type<IsConst>    iterator_base ;

            iterator_type( head_type const& pNode )
                : iterator_base( const_cast<head_type *>(&pNode) )
            {}

            explicit iterator_type( const iterator_base& it )
                : iterator_base( it )
            {}

            friend class LazyList ;

        protected:
            explicit iterator_type( node_type& pNode )
                : iterator_base( &pNode )
            {}

        public:
            typedef typename cds::details::make_const_type<value_type, IsConst>::pointer   value_ptr;
            typedef typename cds::details::make_const_type<value_type, IsConst>::reference value_ref;

            iterator_type()
            {}

            iterator_type( const iterator_type& src )
                : iterator_base( src )
            {}

            value_ptr operator ->() const
            {
                typename iterator_base::value_ptr p = iterator_base::operator ->()      ;
                return p ? &(p->m_Value) : null_ptr<value_ptr>()    ;
            }

            value_ref operator *() const
            {
                return (iterator_base::operator *()).m_Value  ;
            }

            /// Pre-increment
            iterator_type& operator ++()
            {
                iterator_base::operator ++()    ;
                return *this;
            }

            /// Post-increment
            iterator_type operator ++(int)
            {
                return iterator_base::operator ++(0) ;
            }

            template <bool C>
            bool operator ==(iterator_type<C> const& i ) const
            {
                return iterator_base::operator ==(i)    ;
            }
            template <bool C>
            bool operator !=(iterator_type<C> const& i ) const
            {
                return iterator_base::operator !=(i)    ;
            }
        };
        //@endcond

    public:
        /// Returns a forward iterator addressing the first element in a list
        /**
            For empty list \code begin() == end() \endcode
        */
        typedef iterator_type<false>    iterator        ;

        /// Const forward iterator
        /**
            For iterator's features and requirements see \ref iterator
        */
        typedef iterator_type<true>     const_iterator  ;

        /// Returns a forward iterator addressing the first element in a list
        /**
            For empty list \code begin() == end() \endcode
        */
        iterator begin()
        {
            iterator it( head() )    ;
            ++it    ;   // skip dummy head node
            return it   ;
        }

        /// Returns an iterator that addresses the location succeeding the last element in a list
        /**
            Do not use the value returned by <tt>end</tt> function to access any item.

            The returned value can be used only to control reaching the end of the list.
            For empty list \code begin() == end() \endcode
        */
        iterator end()
        {
            return iterator( tail())   ;
        }

        /// Returns a forward const iterator addressing the first element in a list
        //@{
        const_iterator begin() const
        {
            const_iterator it( head() )    ;
            ++it    ;   // skip dummy head node
            return it   ;
        }
        const_iterator cbegin()
        {
            const_iterator it( head() )    ;
            ++it    ;   // skip dummy head node
            return it   ;
        }
        //@}

        /// Returns an const iterator that addresses the location succeeding the last element in a list
        //@{
        const_iterator end() const
        {
            return const_iterator( tail())   ;
        }
        const_iterator cend()
        {
            return const_iterator( tail())   ;
        }
        //@}

    protected:
        //@cond
        iterator node_to_iterator( node_type * pNode )
        {
            if ( pNode )
                return iterator( *pNode )    ;
            return end()    ;
        }
        //@endcond

    public:
        /// Default constructor
        /**
            Initialize empty list
        */
        LazyList()
        {}

        /// List desctructor
        /**
            Clears the list
        */
        ~LazyList()
        {
            clear() ;
        }

        /// Inserts new node
        /**
            The function inserts \p val in the list if the list does not contain
            an item with key equal to \p val.

            Return an iterator pointing to inserted item if success \ref end() otherwise
        */
        template <typename Q>
        iterator insert( Q const& val )
        {
            return node_to_iterator( insert_at( head(), val ) ) ;
        }

#   ifdef CDS_EMPLACE_SUPPORT
        /// Inserts data of type \ref value_type constructed with <tt>std::forward<Args>(args)...</tt>
        /**
            Return an iterator pointing to inserted item if success \ref end() otherwise

            This function is available only for compiler that supports
            variadic template and move semantics
        */
        template <typename... Args>
        iterator emplace( Args&&... args )
        {
            return node_to_iterator( emplace_at( head(), std::forward<Args>(args)... )) ;
        }
#   endif

        /// Ensures that the item \p val exists in the list
        /**
            The operation inserts new item if the key \p val is not found in the list.
            Otherwise, the function returns an iterator that points to item found.

            Returns <tt> std::pair<iterator, bool>  </tt> where \p first is an iterator pointing to
            item found or inserted, \p second is true if new item has been added or \p false if the item
            already is in the list.
        */
        template <typename Q>
        std::pair<iterator, bool> ensure( Q const& val )
        {
            std::pair< node_type *, bool > ret = ensure_at( head(), val )  ;
            return std::make_pair( node_to_iterator( ret.first ), ret.second ) ;
        }

        /// Find the key \p val
        /** \anchor cds_nonintrusive_LazyList_nogc_find
            The function searches the item with key equal to \p val
            and returns an iterator pointed to item found if the key is found,
            and \ref end() otherwise
        */
        template <typename Q>
        iterator find( Q const& key )
        {
            return node_to_iterator( find_at( head(), key, intrusive_key_comparator() ))    ;
        }

        /// Finds the key \p val using \p pred predicate for searching
        /**
            The function is an analog of \ref cds_nonintrusive_LazyList_nogc_find "find(Q const&)"
            but \p pred is used for key comparing.
            \p Less functor has the interface like \p std::less.
            \p pred must imply the same element order as the comparator used for building the list.
        */
        template <typename Q, typename Less>
        iterator find_with( Q const& key, Less pred )
        {
            return node_to_iterator( find_at( head(), key, typename options::template less_wrapper<Less>::type() ))    ;
        }

        /// Check if the list is empty
        bool empty() const
        {
            return base_class::empty()  ;
        }

        /// Returns list's item count
        /**
            The value returned depends on opt::item_counter option. For atomicity::empty_item_counter,
            this function always returns 0.

            <b>Warning</b>: even if you use real item counter and it returns 0, this fact is not mean that the list
            is empty. To check list emptyness use \ref empty() method.
        */
        size_t size() const
        {
            return base_class::size()   ;
        }

        /// Clears the list
        /**
            Post-condition: the list is empty
        */
        void clear()
        {
            base_class::clear() ;
        }

    protected:
        //@cond
        node_type * insert_node_at( head_type& refHead, node_type * pNode )
        {
            assert( pNode != null_ptr<node_type *>() ) ;
            scoped_node_ptr p( pNode ) ;
            if ( base_class::insert_at( &refHead, *p ))
                return p.release() ;

            return null_ptr<node_type *>() ;
        }

        template <typename Q>
        node_type * insert_at( head_type& refHead, Q const& val )
        {
            return insert_node_at( refHead, alloc_node( val )) ;
        }

#   ifdef CDS_EMPLACE_SUPPORT
        template <typename... Args>
        node_type * emplace_at( head_type& refHead, Args&&... args )
        {
            return insert_node_at( refHead, alloc_node( std::forward<Args>(args)... )) ;
        }
#   endif
        template <typename Q>
        std::pair< node_type *, bool > ensure_at( head_type& refHead, Q const& val )
        {
            scoped_node_ptr pNode( alloc_node( val ))   ;
            node_type * pItemFound = null_ptr<node_type *>() ;

#   ifdef CDS_CXX11_LAMBDA_SUPPORT
            std::pair<bool, bool> ret = base_class::ensure_at( &refHead, *pNode,
                [&pItemFound](bool, node_type& item, node_type&){ pItemFound = &item; }) ;
#   else
            ensure_functor func     ;
            std::pair<bool, bool> ret = base_class::ensure_at( &refHead, *pNode, boost::ref(func) ) ;
            pItemFound = func.m_pItemFound  ;
#   endif
            assert( pItemFound != null_ptr<node_type *>() ) ;

            if ( ret.first && ret.second )
                pNode.release() ;

            return std::make_pair( pItemFound, ret.second )  ;
        }

        template <typename Q, typename Compare>
        node_type * find_at( head_type& refHead, Q const& key, Compare cmp )
        {
            return base_class::find_at( &refHead, key, cmp ) ;
        }

        //@endcond
    };
}} // namespace cds::container

#endif // #ifndef __CDS_CONTAINER_LAZY_LIST_NOGC_H
