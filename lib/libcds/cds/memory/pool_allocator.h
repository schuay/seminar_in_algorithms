//$$CDS-header$$

#ifndef __CDS_MEMORY_POOL_ALLOCATOR_H
#define __CDS_MEMORY_POOL_ALLOCATOR_H

#include <cds/details/defs.h>
#include <utility>

namespace cds { namespace memory {

    /// Pool allocator adapter
    /**
        This class is an adapter for an object pool. It gives \p std::allocator interface
        for the pool.

        Template arguments:
        - \p T - value type
        - \p Accessor - a functor to access to pool object. The pool has the following interface:
            \code
            template <typename T>
            class pool {
                typedef T value_type    ;   // Object type maintained by pool
                T * allocate( size_t n )            ;   // Allocate an array of object of type T
                void deallocate( T * p, size_t n )  ;   // Deallocate the array p of size n
            };
            \endcode

        <b>Usage</b>

            Suppose, we have got a pool with interface above. Usually, the pool is a static object:
            \code
                static pool<Foo>     thePool ;
            \endcode

            The \p %pool_allocator gives \p std::allocator interface for the pool.
            It is needed to declare an <i>accessor</i> functor to access to \p thePool:
            \code
                struct pool_accessor {
                    typedef typename pool::value_type   value_type ;

                    pool& operator()() const
                    {
                        return thePool  ;
                    }
                };
            \endcode

            Now, <tt>cds::memory::pool_allocator< T, pool_accessor > </tt> can be used instead of \p std::allocator.
    */
    template <typename T, typename Accessor>
    class pool_allocator
    {
    //@cond
    public:
        typedef Accessor    accessor_type   ;

        typedef size_t      size_type       ;
        typedef ptrdiff_t   difference_type ;
        typedef T*          pointer         ;
        typedef const T*    const_pointer   ;
        typedef T&          reference       ;
        typedef const T&    const_reference ;
        typedef T           value_type      ;

        template <class U> struct rebind {
            typedef pool_allocator<U, accessor_type> other;
        };

    public:
        pool_allocator() CDS_NOEXCEPT
        {}

        pool_allocator(const pool_allocator&) CDS_NOEXCEPT
        {}
        template <class U> pool_allocator(const pool_allocator<U, accessor_type>&) CDS_NOEXCEPT
        {}
        ~pool_allocator()
        {}

        pointer address(reference x) const CDS_NOEXCEPT
        {
            return &x ;
        }
        const_pointer address(const_reference x) const CDS_NOEXCEPT
        {
            return &x ;
        }
        pointer allocate( size_type n, void const * hint = 0)
        {
            static_assert( sizeof(value_type) <= sizeof(typename accessor_type::value_type), "Incompatible type" ) ;

            return reinterpret_cast<pointer>( accessor_type()().allocate( n )) ;
        }
        void deallocate(pointer p, size_type n) CDS_NOEXCEPT
        {
            accessor_type()().deallocate( reinterpret_cast<typename accessor_type::value_type *>( p ), n ) ;
        }
        size_type max_size() const CDS_NOEXCEPT
        {
            return size_t(-1) / sizeof(value_type) ;
        }

#if defined(CDS_MOVE_SEMANTICS_SUPPORT) && defined(CDS_CXX11_VARIADIC_TEMPLATE_SUPPORT)
        template <class U, class... Args>
        void construct(U* p, Args&&... args)
        {
            new((void *)p) U( std::forward<Args>(args)...) ;
        }
#else
        template <typename Arg>
        void construct(pointer p, Arg const& val )
        {
            new((void *) p) value_type(val) ;
        }
#endif
        template <class U>
        void destroy(U* p)
        {
            p->~U() ;
        }
    //@endcond
    };

}} // namespace cds::memory


#endif // #ifndef __CDS_MEMORY_POOL_ALLOCATOR_H
