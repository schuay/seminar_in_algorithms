//$$CDS-header$$

#ifndef __CDSUNIT_STD_PQUEUE_H
#define __CDSUNIT_STD_PQUEUE_H

#include <queue>

namespace pqueue {

    template <typename T, typename Container, typename Lock, typename Less=std::less<typename Container::value_type> >
    class StdPQueue
    {
        typedef T value_type ;
        typedef std::priority_queue<value_type, Container, Less> pqueue_type ;

        pqueue_type     m_PQueue    ;
        mutable Lock    m_Lock      ;

        struct scoped_lock
        {
            Lock&   m_lock ;

            scoped_lock( Lock& l )
                : m_lock(l)
            {
                l.lock();
            }

            ~scoped_lock()
            {
                m_lock.unlock();
            }
        };

    public:
        bool push( value_type const& val )
        {
            scoped_lock l( m_Lock );
            m_PQueue.push( val );
            return true;
        }

        bool pop( value_type& dest )
        {
            scoped_lock l( m_Lock ) ;
            if ( !m_PQueue.empty() ) {
                dest = m_PQueue.top()   ;
                m_PQueue.pop()  ;
                return true ;
            }
            return false ;
        }

        template <typename Q, typename MoveFunc>
        bool pop_with( Q& dest, MoveFunc f )
        {
            scoped_lock l( m_Lock ) ;
            if ( !m_PQueue.empty() ) {
                f( dest, m_PQueue.top())    ;
                m_PQueue.pop()  ;
                return true ;
            }
            return false ;
        }

        void clear()
        {
            scoped_lock l( m_Lock ) ;
            while ( !m_PQueue.empty() )
                m_PQueue.pop()  ;
        }

        template <typename Func>
        void clear_with( Func f )
        {
            scoped_lock l( m_Lock ) ;
            while ( !m_PQueue.empty() ) {
                f( m_PQueue.top() );
                m_PQueue.pop()  ;
            }
        }

        bool empty() const
        {
            return m_PQueue.empty() ;
        }

        size_t size() const
        {
            return m_PQueue.size() ;
        }
    };

} // namespace pqueue

#endif // #ifndef __CDSUNIT_STD_PQUEUE_H
