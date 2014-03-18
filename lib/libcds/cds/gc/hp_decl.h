//$$CDS-header$$

#ifndef __CDS_GC_HP_DECL_H
#define __CDS_GC_HP_DECL_H

#include <cds/gc/hzp/hzp.h>
#include <cds/details/marked_ptr.h>

namespace cds { namespace gc {
    /// @defgroup cds_garbage_collector Garbage collectors

    /// Hazard Pointer garbage collector
    /**  @ingroup cds_garbage_collector
        @headerfile cds/gc/hp.h

        This class realizes a wrapper for Hazard Pointer garbage collector internal implementation.

        Sources:
            - [2002] Maged M.Michael "Safe memory reclamation for dynamic lock-freeobjects using atomic reads and writes"
            - [2003] Maged M.Michael "Hazard Pointers: Safe memory reclamation for lock-free objects"
            - [2004] Andrei Alexandrescy, Maged Michael "Lock-free Data Structures with Hazard Pointers"

        See \ref cds_how_to_use "How to use" section for details of garbage collector applying.
    */
    class HP
    {
    public:
        /// Native guarded pointer type
        typedef gc::hzp::hazard_pointer guarded_pointer ;

#ifdef CDS_CXX11_TEMPLATE_ALIAS_SUPPORT
        /// Atomic reference
        /**
            @headerfile cds/gc/hp.h
        */
        template <typename T> using atomic_ref = CDS_ATOMIC::atomic<T *> ;

        /// Atomic marked pointer
        /**
            @headerfile cds/gc/hp.h
        */
        template <typename MarkedPtr> using atomic_marked_ptr = CDS_ATOMIC::atomic<MarkedPtr> ;

        /// Atomic type
        /**
            @headerfile cds/gc/hp.h
        */
        template <typename T> using atomic_type = CDS_ATOMIC::atomic<T> ;
#else
        template <typename T>
        class atomic_ref: public CDS_ATOMIC::atomic<T *>
        {
            typedef CDS_ATOMIC::atomic<T *> base_class ;
        public:
#   ifdef CDS_CXX11_EXPLICITLY_DEFAULTED_FUNCTION_SUPPORT
            atomic_ref() = default;
#   else
            atomic_ref()
                : base_class()
            {}
#   endif
            explicit CDS_CONSTEXPR atomic_ref(T * p) CDS_NOEXCEPT
                : base_class( p )
            {}
        };

        template <typename T>
        class atomic_type: public CDS_ATOMIC::atomic<T>
        {
            typedef CDS_ATOMIC::atomic<T> base_class ;
        public:
#   ifdef CDS_CXX11_EXPLICITLY_DEFAULTED_FUNCTION_SUPPORT
            atomic_type() = default;
#   else
            atomic_type() CDS_NOEXCEPT
                : base_class()
            {}
#   endif
            explicit CDS_CONSTEXPR atomic_type(T const & v) CDS_NOEXCEPT
                : base_class( v )
            {}
        };

        template <typename MarkedPtr>
        class atomic_marked_ptr: public CDS_ATOMIC::atomic<MarkedPtr>
        {
            typedef CDS_ATOMIC::atomic<MarkedPtr> base_class ;
        public:
#   ifdef CDS_CXX11_EXPLICITLY_DEFAULTED_FUNCTION_SUPPORT
            atomic_marked_ptr() CDS_NOEXCEPT_DEFAULTED_( noexcept(base_class()) ) = default;
#   else
            atomic_marked_ptr() CDS_NOEXCEPT_( noexcept(base_class()) )
                : base_class()
            {}
#   endif
            explicit CDS_CONSTEXPR atomic_marked_ptr(MarkedPtr val) CDS_NOEXCEPT_( noexcept(base_class( val )) )
                : base_class( val )
            {}
            explicit CDS_CONSTEXPR atomic_marked_ptr(typename MarkedPtr::value_type * p) CDS_NOEXCEPT_( noexcept(base_class( p )) )
                : base_class( p )
            {}
        };
#endif

        /// Thread GC implementation for internal usage
        typedef hzp::ThreadGC   thread_gc_impl  ;

        /// Wrapper for hzp::ThreadGC class
        /**
            @headerfile cds/gc/hp.h
            This class performs automatically attaching/detaching Hazard Pointer GC
            for the current thread.
        */
        class thread_gc: public thread_gc_impl
        {
            //@cond
            bool    m_bPersistent   ;
            //@endcond
        public:

            /// Constructor
            /**
                The constructor attaches the current thread to the Hazard Pointer GC
                if it is not yet attached.
                The \p bPersistent parameter specifies attachment persistence:
                - \p true - the class destructor will not detach the thread from Hazard Pointer GC.
                - \p false (default) - the class destructor will detach the thread from Hazard Pointer GC.
            */
            thread_gc(
                bool    bPersistent = false
            ) ;     //inline in hp_impl.h

            /// Destructor
            /**
                If the object has been created in persistent mode, the destructor does nothing.
                Otherwise it detaches the current thread from Hazard Pointer GC.
            */
            ~thread_gc() ;  // inline in hp_impl.h
        };

        /// Base for container node
        /**
            @headerfile cds/gc/hp.h
            This struct is empty for Hazard Pointer GC
        */
        struct container_node
        {};

        /// Hazard Pointer guard
        /**
            @headerfile cds/gc/hp.h
            This class is a wrapper for hzp::AutoHPGuard.
        */
        class Guard: public hzp::AutoHPGuard
        {
            //@cond
            typedef hzp::AutoHPGuard base_class  ;
            //@endcond

        public:
            //@cond
            Guard() ;   // inline in hp_impl.h
            //@endcond

            /// Protects a pointer of type \p atomic<T*>
            /**
                Return the value of \p toGuard

                The function tries to load \p toGuard and to store it
                to the HP slot repeatedly until the guard's value equals \p toGuard
            */
            template <typename T>
            T protect( CDS_ATOMIC::atomic<T> const& toGuard )
            {
                T pCur = toGuard.load(CDS_ATOMIC::memory_order_relaxed) ;
                T pRet ;
                do {
                    pRet = assign( pCur )    ;
                    pCur = toGuard.load(CDS_ATOMIC::memory_order_acquire)  ;
                } while ( pRet != pCur )     ;
                return pCur ;
            }

            /// Protects a converted pointer of type \p atomic<T*>
            /**
                Return the value of \p toGuard

                The function tries to load \p toGuard and to store result of \p f functor
                to the HP slot repeatedly until the guard's value equals \p toGuard.

                The function is useful for intrusive containers when \p toGuard is a node pointer
                that should be converted to a pointer to the value type before protecting.
                The parameter \p f of type Func is a functor that makes this conversion:
                \code
                    struct functor {
                        value_type * operator()( T * p ) ;
                    };
                \endcode
                Really, the result of <tt> f( toGuard.load() ) </tt> is assigned to the hazard pointer.
            */
            template <typename T, class Func>
            T protect( CDS_ATOMIC::atomic<T> const& toGuard, Func f )
            {
                T pCur = toGuard.load(CDS_ATOMIC::memory_order_relaxed) ;
                T pRet ;
                do {
                    pRet = pCur ;
                    assign( f( pCur ) ) ;
                    pCur = toGuard.load(CDS_ATOMIC::memory_order_acquire)  ;
                } while ( pRet != pCur )     ;
                return pCur ;
            }

            /// Store \p p to the guard
            /**
                The function equals to a simple assignment the value \p p to guard, no loop is performed.
                Can be used for a pointer that cannot be changed concurrently
            */
            template <typename T>
            T * assign( T * p )
            {
                return base_class::operator =(p) ;
            }

            /// Copy from \p src guard to \p this guard
            void copy( Guard const& src )
            {
                assign( src.get_native() ) ;
            }

            /// Store marked pointer \p p to the guard
            /**
                The function equals to a simple assignment of <tt>p.ptr()</tt>, no loop is performed.
                Can be used for a marked pointer that cannot be changed concurrently.
            */
            template <typename T, int BITMASK>
            T * assign( cds::details::marked_ptr<T, BITMASK> p )
            {
                return base_class::operator =( p.ptr() ) ;
            }

            /// Clear value of the guard
            void clear()
            {
                assign( reinterpret_cast<void *>(NULL) )    ;
            }

            /// Get the value currently protected
            template <typename T>
            T * get() const
            {
                return reinterpret_cast<T *>( get_native() )   ;
            }

            /// Get native hazard pointer stored
            guarded_pointer get_native() const
            {
                return base_class::get() ;
            }
        };

        /// Array of Hazard Pointer guards
        /**
            @headerfile cds/gc/hp.h
            This class is a wrapper for hzp::AutoHPArray template.
            Template parameter \p Count defines the size of HP array.
        */
        template <size_t Count>
        class GuardArray: public hzp::AutoHPArray<Count>
        {
            //@cond
            typedef hzp::AutoHPArray<Count> base_class   ;
            //@endcond
        public:
            /// Rebind array for other size \p Count2
            template <size_t Count2>
            struct rebind {
                typedef GuardArray<Count2>  other   ;   ///< rebinding result
            };

        public:
            //@cond
            GuardArray()    ;   // inline in hp_impl.h
            //@endcond
            /// Protects a pointer of type \p atomic<T*>
            /**
                Return the value of \p toGuard

                The function tries to load \p toGuard and to store it
                to the slot \p nIndex repeatedly until the guard's value equals \p toGuard
            */
            template <typename T>
            T protect(size_t nIndex, CDS_ATOMIC::atomic<T> const& toGuard )
            {
                T pRet    ;
                do {
                    pRet = assign( nIndex, toGuard.load(CDS_ATOMIC::memory_order_acquire) )    ;
                } while ( pRet != toGuard.load(CDS_ATOMIC::memory_order_relaxed))    ;

                return pRet ;
            }

            /// Protects a pointer of type \p atomic<T*>
            /**
                Return the value of \p toGuard

                The function tries to load \p toGuard and to store it
                to the slot \p nIndex repeatedly until the guard's value equals \p toGuard

                The function is useful for intrusive containers when \p toGuard is a node pointer
                that should be converted to a pointer to the value type before guarding.
                The parameter \p f of type Func is a functor that makes this conversion:
                \code
                    struct functor {
                        value_type * operator()( T * p ) ;
                    };
                \endcode
                Really, the result of <tt> f( toGuard.load() ) </tt> is assigned to the hazard pointer.
            */
            template <typename T, class Func>
            T protect(size_t nIndex, CDS_ATOMIC::atomic<T> const& toGuard, Func f )
            {
                T pRet    ;
                do {
                    assign( nIndex, f( pRet = toGuard.load(CDS_ATOMIC::memory_order_acquire) ))    ;
                } while ( pRet != toGuard.load(CDS_ATOMIC::memory_order_relaxed))    ;

                return pRet ;
            }

            /// Store \p to the slot \p nIndex
            /**
                The function equals to a simple assignment, no loop is performed.
            */
            template <typename T>
            T * assign( size_t nIndex, T * p )
            {
                base_class::set(nIndex, p) ;
                return p    ;
            }

            /// Store marked pointer \p p to the guard
            /**
                The function equals to a simple assignment of <tt>p.ptr()</tt>, no loop is performed.
                Can be used for a marked pointer that cannot be changed concurrently.
            */
            template <typename T, int BITMASK>
            T * assign( size_t nIndex, cds::details::marked_ptr<T, BITMASK> p )
            {
                return assign( nIndex, p.ptr() ) ;
            }

            /// Copy guarded value from \p src guard to slot at index \p nIndex
            void copy( size_t nIndex, Guard const& src )
            {
                assign( nIndex, src.get_native() ) ;
            }

            /// Copy guarded value from slot \p nSrcIndex to slot at index \p nDestIndex
            void copy( size_t nDestIndex, size_t nSrcIndex )
            {
                assign( nDestIndex, get_native( nSrcIndex )) ;
            }

            /// Clear value of the slot \p nIndex
            void clear( size_t nIndex)
            {
                base_class::clear( nIndex );
            }

            /// Get current value of slot \p nIndex
            template <typename T>
            T * get( size_t nIndex) const
            {
                return reinterpret_cast<T *>( get_native( nIndex ) )   ;
            }

            /// Get native hazard pointer stored
            guarded_pointer get_native( size_t nIndex ) const
            {
                return base_class::operator[](nIndex).get() ;
            }

            /// Capacity of the guard array
            static CDS_CONSTEXPR size_t capacity()
            {
                return Count ;
            }
        };

    public:
        /// Initializes hzp::GarbageCollector singleton
        /**
            The constructor initializes GC singleton with passed parameters.
            If GC instance is not exist then the function creates the instance.
            Otherwise it does nothing.

            The Michael's HP reclamation schema depends of three parameters:
            - \p nHazardPtrCount - hazard pointer count per thread. Usually it is small number (up to 10) depending from
                the data structure algorithms. By default, if \p nHazardPtrCount = 0, the function
                uses maximum of the hazard pointer count for CDS library.
            - \p nMaxThreadCount - max count of thread with using Hazard Pointer GC in your application. Default is 100.
            - \p nMaxRetiredPtrCount - capacity of array of retired pointers for each thread. Must be greater than
                <tt> nHazardPtrCount * nMaxThreadCount </tt>. Default is <tt>2 * nHazardPtrCount * nMaxThreadCount </tt>.
        */
        HP(
            size_t nHazardPtrCount = 0,     ///< Hazard pointer count per thread
            size_t nMaxThreadCount = 0,     ///< Max count of simultaneous working thread in your application
            size_t nMaxRetiredPtrCount = 0, ///< Capacity of the array of retired objects for the thread
            hzp::scan_type nScanType = hzp::inplace   ///< Scan type (see \ref hzp::scan_type enum)
        )
        {
            hzp::GarbageCollector::Construct(
                nHazardPtrCount,
                nMaxThreadCount,
                nMaxRetiredPtrCount,
                nScanType
            )   ;
        }

        /// Terminates GC singleton
        /**
            The destructor calls \code hzp::GarbageCollector::Destruct( true ) \endcode
        */
        ~HP()
        {
            hzp::GarbageCollector::Destruct( true )  ;
        }

        /// Checks if count of hazard pointer is no less than \p nCountNeeded
        /**
            If \p bRaiseException is \p true (that is the default), the function raises an exception gc::too_few_hazard_pointers
            if \p nCountNeeded is more than the count of hazard pointer per thread.
        */
        static bool check_available_guards( size_t nCountNeeded, bool bRaiseException = true )
        {
            if ( hzp::GarbageCollector::instance().getHazardPointerCount() < nCountNeeded ) {
                if ( bRaiseException )
                    throw cds::gc::too_few_hazard_pointers() ;
                return false ;
            }
            return true ;
        }

        /// Retire pointer \p p with function \p pFunc
        /**
            The function places pointer \p p to array of pointers ready for removing.
            (so called retired pointer array). The pointer can be safely removed when no hazard pointer points to it.
            Deleting the pointer is the function \p pFunc call.
        */
        template <typename T>
        static void retire( T * p, void (* pFunc)(T *) )    ;   // inline in hp_impl.h

        /// Retire pointer \p p with functor of type \p Disposer
        /**
            The function places pointer \p p to array of pointers ready for removing.
            (so called retired pointer array). The pointer can be safely removed when no hazard pointer points to it.

            Deleting the pointer is an invocation of some object of type \p Disposer; the interface of \p Disposer is:
            \code
            template <typename T>
            struct disposer {
                void operator()( T * p )    ;   // disposing operator
            };
            \endcode
            Since the functor call can happen at any time after \p retire call, additional restrictions are imposed to \p Disposer type:
            - it should be stateless functor
            - it should be default-constructible
            - the result of functor call with argument \p p should not depend on where the functor will be called.

            \par Examples:
            Operator \p delete functor:
            \code
            template <typename T>
            struct disposer {
                void operator ()( T * p ) {
                    delete p    ;
                }
            };

            // How to call GC::retire method
            int * p = new int   ;

            // ... use p in lock-free manner

            cds::gc::HP::retire<disposer>( p ) ;   // place p to retired pointer array of HP GC
            \endcode

            Functor based on \p std::allocator :
            \code
            template <typename ALLOC = std::allocator<int> >
            struct disposer {
                template <typename T>
                void operator()( T * p ) {
                    typedef typename ALLOC::templare rebind<T>::other   alloc_t ;
                    alloc_t a   ;
                    a.destroy( p )  ;
                    a.deallocate( p, 1 )    ;
                }
            };
            \endcode
        */
        template <class Disposer, typename T>
        static void retire( T * p ) ;   // inline in hp_impl.h

        /// Get current scan strategy
        /**@anchor hrc_gc_HP_getScanType
            See hzp::GarbageCollector::Scan for scan algo description
        */
        hzp::scan_type getScanType() const
        {
            return hzp::GarbageCollector::instance().getScanType()  ;
        }

        /// Set current scan strategy
        /**
            Scan strategy changing is allowed on the fly.

            About scan strategy see \ref hrc_gc_HP_getScanType "getScanType"
        */
        void setScanType(
            hzp::scan_type nScanType     ///< new scan strategy
        )
        {
            hzp::GarbageCollector::instance().setScanType( nScanType ) ;
        }

        /// Checks if Hazard Pointer GC is constructed and may be used
        static bool isUsed()
        {
            return hzp::GarbageCollector::isUsed() ;
        }


        /// Forced GC cycle call for current thread
        /**
            Usually, this function should not be called directly.
        */
        //@{
        static void scan()  ;   // inline in hp_impl.h
        static void force_dispose()
        {
            scan() ;
        }
        //@}
    };
}}  // namespace cds::gc

#endif  // #ifndef __CDS_GC_HP_DECL_H
