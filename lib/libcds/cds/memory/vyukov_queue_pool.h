//$$CDS-header$$

#ifndef __CDS_MEMORY_VYUKOV_QUEUE_ALLOCATOR_H
#define __CDS_MEMORY_VYUKOV_QUEUE_ALLOCATOR_H

#include <cds/details/allocator.h>
#include <cds/intrusive/vyukov_mpmc_cycle_queue.h>

namespace cds { namespace memory {

    /// Free-list based on bounded lock-free queue cds::intrusive::VyukovMPMCCycleQueue
    /**
        Template parameters:
        - \p T - the type of object maintaining by free-list
        - \p Options - the options of cds::intrusive::VyukovMPMCCycleQueue class plus
            cds::opt::allocator option.

        \b Internals

        This free-list is very simple.
        At construction time, the free-list allocates the array of N items
        and stores them into queue, where N is the queue capacity.
        When allocating the free-list tries to pop an object from
        internal queue i.e. from preallocated pool. If success the popped object is returned.
        Otherwise a new one is allocated. When deallocating, the free-list checks whether
        the object is from preallocated pool. If so, the object is pushed into queue, otherwise
        it is deallocated by using the allocator provided.
        The pool can contain more than \p N items.

        \b Usage

        \p %vyukov_queue_pool should be used together with \ref pool_allocator.
        You should declare an static object of type \p %vyukov_queue_pool, provide
        an accessor functor to this object and use \p pool_allocator as an allocator:
        \code
        #include <cds/memory/vyukov_queue_pool.h>
        #include <cds/memory/pool_allocator.h>

        // Pool of Foo object of size 1024.
        typedef cds::memory::vyukov_queue_pool< Foo, cds::opt::buffer< cds::opt::v::static_buffer< Foo, 1024 > > pool_type ;
        static pool_type thePool    ;

        struct pool_accessor {
            typedef typename pool_type::value_type  value_type  ;

            pool_type& operator()() const
            {
                return thePool  ;
            }
        };

        // Declare pool allocator
        typedef cds::memory::pool_allocator< Foo, pool_accessor >   pool_allocator ;

        // Use pool_allocator
        // Allocate an object
        Foo * p = pool_allocator().allocate( 1 ) ;

        // construct object
        new(p) Foo ;

        //...

        // Destruct object
        p->~Foo()   ;

        // Deallocate object
        pool_allocator().deallocate( p , 1 ) ;
        \endcode
    */
    template <typename T, CDS_DECL_OPTIONS6>
    class vyukov_queue_pool
    {
    public:
        typedef cds::intrusive::VyukovMPMCCycleQueue< T, CDS_OPTIONS6 > queue_type  ;   ///< Queue type

    protected:
        //@cond
        struct default_options: public queue_type::options
        {
            typedef CDS_DEFAULT_ALLOCATOR   allocator   ;
        };
        typedef typename opt::make_options< default_options, CDS_OPTIONS6 >::type   options ;
        //@endcond

    public:
        typedef T  value_type ; ///< Value type
        typedef typename options::allocator::template rebind<value_type>::other allocator_type  ;   ///< allocator type

    protected:
        //@cond
        queue_type      m_Queue ;
        value_type *    m_pFirst;
        value_type *    m_pLast ;
        //@endcond

    protected:
        //@cond
        void preallocate_pool()
        {
            m_pFirst = allocator_type().allocate( m_Queue.capacity() ) ;
            m_pLast = m_pFirst + m_Queue.capacity() ;

            for ( value_type * p = m_pFirst; p < m_pLast; ++p )
                CDS_VERIFY( m_Queue.push( *p )) ;   // must be true
        }

        bool from_pool( value_type * p ) const
        {
            return m_pFirst <= p && p < m_pLast ;
        }
        //@endcond

    public:
        /// Preallocates the pool of object
        /**
            \p nCapacity argument is the queue capacity. It should be passed
            if the queue is based on dynamically-allocated buffer.
            See cds::intrusive::VyukovMPMCCycleQueue for explanation.
        */
        vyukov_queue_pool( size_t nCapacity = 0 )
            : m_Queue( nCapacity )
        {
            preallocate_pool()  ;
        }

        /// Deallocates the pool.
        ~vyukov_queue_pool()
        {
            m_Queue.clear() ;
            allocator_type().deallocate( m_pFirst, m_Queue.capacity() ) ;
        }

        /// Allocates an object from pool
        /**
            The pool supports allocation only single object (\p n = 1).
            If \p n > 1 the behaviour is undefined.

            If the queue is not empty, the popped value is returned.
            Otherwise, a new value allocated.
        */
        value_type * allocate( size_t n )
        {
            assert( n == 1 )    ;

            value_type * p = m_Queue.pop() ;
            if ( p ) {
                assert( from_pool(p) ) ;
                return p    ;
            }

            return allocator_type().allocate( n ) ;
        }

        /// Deallocated the object \p p
        /**
            The pool supports allocation only single object (\p n = 1).
            If \p n > 1 the behaviour is undefined.

            If \p p is from preallocated pool, it pushes into the queue.
            Otherwise, \p p is deallocated by allocator provided.
        */
        void deallocate( value_type * p, size_t n )
        {
            assert( n == 1 )    ;

            if ( p ) {
                if ( from_pool( p ) )
                    m_Queue.push( *p )  ;
                else
                    allocator_type().deallocate( p, n ) ;
            }
        }
    };

    /// Lazy free-list based on bounded lock-free queue cds::intrusive::VyukovMPMCCycleQueue
    /**
        Template parameters:
        - \p T - the type of object maintaining by free-list
        - \p Options - the options of cds::intrusive::VyukovMPMCCycleQueue class plus
            cds::opt::allocator option.

        \b Internals

        This free-list is very simple.
        At construction time the pool is empty.
        When allocating the free-list tries to pop an object from
        internal queue. If success the popped object is returned.
        Otherwise a new one is allocated.
        When deallocating, the free-list tries to push the object into the pool.
        If internal queue is full, the object is deallocated by using the allocator provided.
        The pool can contain more than \p N items.

        \b Usage

        \p %lazy_vyukov_queue_pool should be used together with \ref pool_allocator.
        You should declare an static object of type \p %lazy_vyukov_queue_pool, provide
        an accessor functor to this object and use \p pool_allocator as an allocator:
        \code
        #include <cds/memory/vyukov_queue_pool.h>
        #include <cds/memory/pool_allocator.h>

        // Pool of Foo object of size 1024.
        typedef cds::memory::lazy_vyukov_queue_pool< Foo, cds::opt::buffer< cds::opt::v::dynamic_buffer< Foo > > pool_type ;
        static pool_type thePool( 1024 ) ;

        struct pool_accessor {
            typedef typename pool_type::value_type  value_type  ;

            pool_type& operator()() const
            {
                return thePool  ;
            }
        };

        // Declare pool allocator
        typedef cds::memory::pool_allocator< Foo, pool_accessor >   pool_allocator ;

        // Use pool_allocator
        // Allocate an object
        Foo * p = pool_allocator().allocate( 1 ) ;

        // construct object
        new(p) Foo ;

        //...

        // Destruct object
        p->~Foo()   ;

        // Deallocate object
        pool_allocator().deallocate( p , 1 ) ;
        \endcode

    */
    template <typename T, CDS_DECL_OPTIONS6>
    class lazy_vyukov_queue_pool
    {
    public:
        typedef cds::intrusive::VyukovMPMCCycleQueue< T, CDS_OPTIONS6 > queue_type  ;   ///< Queue type

    protected:
        //@cond
        struct default_options: public queue_type::options
        {
            typedef CDS_DEFAULT_ALLOCATOR   allocator   ;
        };
        typedef typename opt::make_options< default_options, CDS_OPTIONS6 >::type   options ;
        //@endcond

    public:
        typedef T  value_type ; ///< Value type
        typedef typename options::allocator::template rebind<value_type>::other allocator_type  ;   ///< allocator type

    protected:
        //@cond
        queue_type      m_Queue ;
        //@endcond

    public:
        /// Constructs empty pool
        lazy_vyukov_queue_pool( size_t nCapacity = 0 )
            : m_Queue( nCapacity )
        {}

        /// Deallocates all objects from the pool
        ~lazy_vyukov_queue_pool()
        {
            allocator_type a ;
            while ( !m_Queue.empty() ) {
                value_type * p = m_Queue.pop() ;
                a.deallocate( p, 1 )    ;
            }
        }

        /// Allocates an object from pool
        /**
            The pool supports allocation only single object (\p n = 1).
            If \p n > 1 the behaviour is undefined.

            If the queue is not empty, the popped value is returned.
            Otherwise, a new value allocated.
        */
        value_type * allocate( size_t n )
        {
            assert( n == 1 )    ;

            value_type * p = m_Queue.pop() ;
            if ( p )
                return p    ;

            return allocator_type().allocate( n ) ;
        }

        /// Deallocated the object \p p
        /**
            The pool supports allocation only single object (\p n = 1).
            If \p n > 1 the behaviour is undefined.

            If the queue is not full, \p p is pushed into the queue.
            Otherwise, \p p is deallocated by allocator provided.
        */
        void deallocate( value_type * p, size_t n )
        {
            assert( n == 1 )    ;

            if ( p ) {
                if ( !m_Queue.push( *p ))
                    allocator_type().deallocate( p, n ) ;
            }
        }

    };

    /// Bounded free-list based on bounded lock-free queue cds::intrusive::VyukovMPMCCycleQueue
    /**
        Template parameters:
        - \p T - the type of object maintaining by free-list
        - \p Options - the options of cds::intrusive::VyukovMPMCCycleQueue class plus
            cds::opt::allocator option.

        \b Internals

        At construction time, the free-list allocates the array of N items
        and stores them into queue, where N is the queue capacity.
        When allocating the free-list tries to pop an object from
        internal queue i.e. from preallocated pool. If success the popped object is returned.
        Otherwise a \p std::bad_alloc exception is raised.
        So, the pool can contain up to \p N items.
        When deallocating, the object is pushed into queue.
        In debug mode the \ref deallocate member function asserts
        that the pointer is from preallocated pool.

        \b Usage

        \p %bounded_vyukov_queue_pool should be used together with \ref pool_allocator.
        You should declare an static object of type \p %bounded_vyukov_queue_pool, provide
        an accessor functor to this object and use \p pool_allocator as an allocator:
        \code
        #include <cds/memory/vyukov_queue_pool.h>
        #include <cds/memory/pool_allocator.h>

        // Pool of Foo object of size 1024.
        typedef cds::memory::bounded_vyukov_queue_pool< Foo, cds::opt::buffer< cds::opt::v::static_buffer< Foo, 1024 > > pool_type ;
        static pool_type thePool    ;

        struct pool_accessor {
            typedef typename pool_type::value_type  value_type  ;

            pool_type& operator()() const
            {
                return thePool  ;
            }
        };

        // Declare pool allocator
        typedef cds::memory::pool_allocator< Foo, pool_accessor >   pool_allocator ;

        // Use pool_allocator
        // Allocate an object
        Foo * p = pool_allocator().allocate( 1 ) ;

        // construct object
        new(p) Foo ;

        //...

        // Destruct object
        p->~Foo()   ;

        // Deallocate object
        pool_allocator().deallocate( p , 1 ) ;
        \endcode
    */
    template <typename T, CDS_DECL_OPTIONS6>
    class bounded_vyukov_queue_pool
    {
    public:
        typedef cds::intrusive::VyukovMPMCCycleQueue< T, CDS_OPTIONS6 > queue_type  ;   ///< Queue type

    protected:
        //@cond
        struct default_options: public queue_type::options
        {
            typedef CDS_DEFAULT_ALLOCATOR   allocator   ;
        };
        typedef typename opt::make_options< default_options, CDS_OPTIONS6 >::type   options ;
        //@endcond

    public:
        typedef T  value_type ; ///< Value type
        typedef typename options::allocator::template rebind<value_type>::other allocator_type  ;   ///< allocator type

    protected:
        //@cond
        queue_type      m_Queue ;
        value_type *    m_pFirst;
        value_type *    m_pLast ;
        //@endcond

    protected:
        //@cond
        void preallocate_pool()
        {
            m_pFirst = allocator_type().allocate( m_Queue.capacity() ) ;
            m_pLast = m_pFirst + m_Queue.capacity() ;

            for ( value_type * p = m_pFirst; p < m_pLast; ++p )
                CDS_VERIFY( m_Queue.push( *p )) ;   // must be true
        }

        bool from_pool( value_type * p ) const
        {
            return m_pFirst <= p && p < m_pLast ;
        }
        //@endcond

    public:
        /// Preallocates the pool of object
        /**
            \p nCapacity argument is the queue capacity. It should be passed
            if the queue is based on dynamically-allocated buffer.
            See cds::intrusive::VyukovMPMCCycleQueue for explanation.
        */
        bounded_vyukov_queue_pool( size_t nCapacity = 0 )
            : m_Queue( nCapacity )
        {
            preallocate_pool()  ;
        }

        /// Deallocates the pool.
        ~bounded_vyukov_queue_pool()
        {
            m_Queue.clear() ;
            allocator_type().deallocate( m_pFirst, m_Queue.capacity() ) ;
        }

        /// Allocates an object from pool
        /**
            The pool supports allocation only single object (\p n = 1).
            If \p n > 1 the behaviour is undefined.

            If the queue is not empty, the popped value is returned.
            Otherwise, a \p std::bad_alloc exception is raised.
        */
        value_type * allocate( size_t n )
        {
            assert( n == 1 )    ;

            value_type * p = m_Queue.pop() ;
            if ( p ) {
                assert( from_pool(p) ) ;
                return p    ;
            }

            throw std::bad_alloc() ;
        }

        /// Deallocated the object \p p
        /**
            The pool supports allocation only single object (\p n = 1).
            If \p n > 1 the behaviour is undefined.

            \p should be from preallocated pool.
        */
        void deallocate( value_type * p, size_t n )
        {
            assert( n == 1 )    ;

            if ( p ) {
                assert( from_pool( p )) ;
                m_Queue.push( *p )  ;
            }
        }
    };


}}  // namespace cds::memory


#endif // #ifndef __CDS_MEMORY_VYUKOV_QUEUE_ALLOCATOR_H
