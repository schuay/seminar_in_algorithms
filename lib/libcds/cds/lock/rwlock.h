//$$CDS-header$$

#ifndef __CDS_RWLOCK_H
#define __CDS_RWLOCK_H

#include <cds/atomic.h>
#include <cds/backoff_strategy.h>

#include <cds/details/noncopyable.h>

/*
    ������ Reader-Writer locks, ����������� �� CAS.
    �������� ���� � ��������� ����� �� ������:
        John M. Mellor-Crummey, Michael L. Scott "Scalable Reader-Writer Synchronization for Shared-Memory Multiprocessors", 1991

    ������������:
        - �� ���������� ������� ��������� ������ (���� ��������), - ������ ���� 32- ��� 64-������ �����.
        - �� ������� ��������� �� ������� ������������ ������� ��� ����
    ����������:
        - ���������� �������� �������� (aka spin locks) � backoff � ������ �������.
        - � ������ ����� thread ��������� ��� ���������� ��������/�������� ������ � ������� ����� ���� ������������

    ������:
        SpinWPref        writer preference, no writer ordering RW-lock
                        RW-lock � ������������� ��������, ��� ������������ ��������� �� ������� ����������� ������� �� ������
        SpinWPrefOrd    writer preference, writer ordering RW-lock
                        RW-lock � ������������� ��������, � ������������� ��������� �� ������� ����������� �������
        SpinRPref       Reader preference, no writer ordering RW-lock
                        RW-lock � ������������� ���������. �������� �� ����������� �� ������� ����������� ������� �� ������
*/

///@cond
namespace cds {
    namespace lock {

        /* RWSpinWPrefT  *******************************************************************************
            RW-lock � ������������� ��������, ��� ������������ ��������� �� ������� ����������� �������
        ***********************************************************************************************/
        template <class BACKOFF>
        class RWSpinWPrefT: cds::details::noncopyable
        {
            union Barrier {
                struct {
                    unsigned int readerCount    : 32 / 2;        // ����� �������� ���������
                    unsigned int writerCount    : 32 / 2 - 1;    // ����� �������� � ��������� ���������
                    unsigned int writerActive   : 1 ;            // 1 - �������� �������
                };
                atomic32_t         nAtomic            ;
            } ;

            volatile Barrier     m_Barrier   ;   // ����������� ���������

        public:
            RWSpinWPrefT() throw()
            {
                m_Barrier.nAtomic = 0 ;
            }
            ~RWSpinWPrefT() throw()
            {
                assert( m_Barrier.nAtomic == 0 ) ;
            }

            // ���� ��������
            // �������� �������� ������ ������ �����, ����� ��� ��������/��������� ���������
            void rlock()
            {
                BACKOFF backoff    ;
                Barrier bnew, b ;
                while (true) {
                    bnew.nAtomic = b.nAtomic = m_Barrier.nAtomic    ;
                    bnew.readerCount++      ;
                    bnew.writerActive = b.writerActive = 0  ;
                    bnew.writerCount = b.writerCount = 0    ;
                    if ( CAS( &m_Barrier.nAtomic, b.nAtomic, bnew.nAtomic ))    // OK, ���� ��� ���������
                        break   ;
                    backoff() ;
                }
            }

            // ����� ��������
            void runlock()
            {
                BACKOFF backoff    ;
                Barrier bnew, b ;
                while (true) {
                    bnew.nAtomic = b.nAtomic = m_Barrier.nAtomic    ;
                    bnew.readerCount--      ;
                    assert( b.writerActive == 0 ) ;
                    if ( CAS( &m_Barrier.nAtomic, b.nAtomic, bnew.nAtomic ))    // ��������� ���������� ���������
                        break   ;
                    backoff()    ;
                }
            }

            // ���� ��������
            // �������� ������� ��������� ������ ���������, ���������� �����, � ����� ������� ������������ �������:
            //  ��������� ������ �������� ��������� � ��������� ��������. ������ �������� ���� �� ��������� ���������.
            void wlock()
            {
                Barrier bnew, b ;
                BACKOFF backoff    ;

                // ��������� ����� ���������� ���������
                while (true) {
                    bnew.nAtomic = b.nAtomic = m_Barrier.nAtomic    ;
                    bnew.writerCount++      ;
                    if ( CAS( &m_Barrier.nAtomic, b.nAtomic, bnew.nAtomic ))
                        break   ;
                    backoff()    ;
                }

                backoff.reset() ;

                // �������, ����� �������� �������� � �������� ��������, ���� ����
                while (true) {
                    bnew.nAtomic = b.nAtomic = m_Barrier.nAtomic    ;
                    bnew.readerCount =
                        b.readerCount = 0   ;
                    b.writerActive = 0      ;
                    bnew.writerActive = 1   ;
                    if ( CAS( &m_Barrier.nAtomic, b.nAtomic, bnew.nAtomic ))
                        break   ;
                    backoff()    ;
                }
            }

            // ����� ��������
            void wunlock()
            {
                Barrier bnew, b ;
                BACKOFF backoff    ;

                while (true) {
                    bnew.nAtomic = b.nAtomic = m_Barrier.nAtomic    ;
                    assert( b.writerActive == 1 ) ;
                    assert( b.readerCount == 0 )  ;
                    bnew.writerActive = 0   ;
                    --bnew.writerCount      ;
                    if ( CAS( &m_Barrier.nAtomic, b.nAtomic, bnew.nAtomic ))
                        break   ;
                    backoff()    ;
                }
            }

            bool isWriting() const
            {
                return m_Barrier.writerActive == 1 ;
            }

            bool isReading() const
            {
                return m_Barrier.readerCount != 0    ;
            }
        };
        typedef RWSpinWPrefT< backoff::LockDefault >    RWSpinWPref ;

        /* RWSpinWPrefOrdT  ******************************************************************************
        RW-lock � ������������� ��������, � ������������� ��������� �� ������� ����������� �������
            ��� ����� �������� ��������� ������ ���������, �������� �����, ����������� ������������ �������
            ���������, � ����, ����� �������� ��� ������� �������� ����������� ������.
        ***********************************************************************************************/
        template <class BACKOFF>
        class RWSpinWPrefOrdT: cds::details::noncopyable
        {
            union Barrier {
                struct {
                    unsigned short int  workerCount ;     // ����� �������� ������������� (���������/���������)
                    unsigned short int  writerCount ;     // ����� ��������� ��� �������� ���������
                    unsigned short int  doneCount   ;     // ����� ��������� (�����������) �������������
                    unsigned short int  writerActive;     // =1 - �������� �������
                };
                atomic64_t  nAtomic   ;
            };
            volatile Barrier     m_Barrier   ;

        public:
            RWSpinWPrefOrdT() throw()
            {
                static_assert( sizeof( m_Barrier ) == sizeof( m_Barrier.nAtomic ), "Structure size error") ;
                m_Barrier.nAtomic = 0 ;
            }
            ~RWSpinWPrefOrdT() throw()
            {
                assert( m_Barrier.writerCount == 0 ) ;
            }

            // ���� ��������
            // �������� �������� ������ ������ �����, ����� ��� ���������/��������� ���������
            void rlock()
            {
                Barrier b, bnew ;
                BACKOFF backoff    ;

                while (true) {
                    b.nAtomic = m_Barrier.nAtomic    ;
                    b.writerCount = 0            ;
                    bnew.nAtomic = b.nAtomic    ;
                    ++bnew.workerCount            ;
                    if ( CAS( &m_Barrier.nAtomic, b.nAtomic, bnew.nAtomic ))
                        break   ;
                    backoff()    ;
                }
            }

            // ����� ��������
            void runlock()
            {
                Barrier bnew, b ;
                BACKOFF backoff    ;

                while (true) {
                    bnew.nAtomic = b.nAtomic = m_Barrier.nAtomic    ;
                    ++bnew.doneCount        ;
                    if ( CAS( &m_Barrier.nAtomic, b.nAtomic, bnew.nAtomic ))
                        break   ;
                    backoff()    ;
                }
            }

            // ���� ��������
            // �������� ������� ��������� ������ ���������, ���������� �����, � �������� ����� �� ����
            //  (����� ���������� ������� ���������). ����� �������� ������� ������ ������.
            void wlock()
            {
                Barrier bnew, b ;
                unsigned int nTicket    ;
                BACKOFF backoff    ;

                // ��������� ����� ����������� ��������� � �������� ����� (ticket) �� ����
                while (true) {
                    bnew.nAtomic = b.nAtomic = m_Barrier.nAtomic    ;
                    ++bnew.writerCount              ;
                    nTicket = bnew.workerCount      ;
                    ++bnew.workerCount              ;
                    if ( CAS( &m_Barrier.nAtomic, b.nAtomic, bnew.nAtomic ))
                        break   ;
                    backoff()    ;
                }

                backoff.reset() ;

                // ������� ����� ������� (�� ��������� ������)
                while (true) {
                    b.nAtomic = m_Barrier.nAtomic    ;
                    b.doneCount = nTicket   ;
                    bnew.nAtomic = b.nAtomic;
                    b.writerActive = 0      ;
                    bnew.writerActive = 1   ;
                    if ( CAS( &m_Barrier.nAtomic, b.nAtomic, bnew.nAtomic ))
                        break   ;
                    backoff()    ;
                }
            }

            // ����� ��������
            void wunlock()
            {
                Barrier bnew, b ;
                BACKOFF backoff    ;

                while (true) {
                    bnew.nAtomic = b.nAtomic = m_Barrier.nAtomic    ;
                    --bnew.writerCount      ;
                    ++bnew.doneCount        ;
                    bnew.writerActive = 0   ;
                    if ( CAS( &m_Barrier.nAtomic, b.nAtomic, bnew.nAtomic ))
                        break   ;
                    backoff()    ;
                }
            }

            bool isWriting() const
            {
                return m_Barrier.writerActive != 0    ;
            }

            bool isReading() const
            {
                return m_Barrier.writerCount == 0 && m_Barrier.workerCount != 0    ;
            }
        } ;
        typedef RWSpinWPrefOrdT< backoff::LockDefault > RWSpinWPrefOrd ;


        /* SpinRPrefT  *********************************************************************************
            RW-lock � ������������� ���������, ��� ������������ ��������� �� ������� ����������� �������
        ***********************************************************************************************/
        template <class BACKOFF>
        class RWSpinRPrefT: cds::details::noncopyable
        {
            typedef    atomic32u_t    TAtomic    ;
            union Barrier {
                struct {
                    TAtomic        readerCount:  32 - 1 ;  // ����� �������� ���������
                    TAtomic        writerActive: 1 ;        // 1 - ������� ��������
                };
                TAtomic        nAtomic ;
            };
            volatile Barrier     m_Barrier ;
        public:
            RWSpinRPrefT() throw()
            {
                m_Barrier.nAtomic = 0   ;
            }
            ~RWSpinRPrefT() throw()
            {
                assert( m_Barrier.nAtomic == 0 ) ;
            }

            void rlock()
            {
                Barrier b, bnew ;
                BACKOFF backoff    ;

                while ( true ) {
                    bnew.nAtomic = b.nAtomic = m_Barrier.nAtomic    ;
                    ++bnew.readerCount      ;
                    bnew.writerActive = b.writerActive = 0      ;
                    if ( CAS( &m_Barrier.nAtomic, b.nAtomic, bnew.nAtomic ) )
                        break   ;
                    backoff()    ;
                }
            }

            void runlock()
            {
                Barrier b, bnew ;
                BACKOFF backoff    ;

                while ( true ) {
                    bnew.nAtomic = b.nAtomic = m_Barrier.nAtomic    ;
                    assert( b.writerActive == 0 ) ;
                    --bnew.readerCount      ;
                    if ( CAS( &m_Barrier.nAtomic, b.nAtomic, bnew.nAtomic ) ) {
                        break   ;
                    }
                    backoff()    ;
                }
            }

            void wlock()
            {
                Barrier b ;
                BACKOFF backoff    ;

                b.writerActive = 1  ;
                b.readerCount = 0   ;
                while ( true ) {
                    if ( CAS( &m_Barrier.nAtomic, (TAtomic) 0, b.nAtomic ))
                        break   ;
                    backoff()    ;
                }
            }

            void wunlock()
            {
                BACKOFF backoff    ;

                while ( true ) {
                    Barrier b, bnew         ;
                    bnew.nAtomic = b.nAtomic = m_Barrier.nAtomic    ;
                    bnew.writerActive = 0   ;
                    if ( CAS( &m_Barrier.nAtomic, b.nAtomic, bnew.nAtomic ))
                        break   ;
                    backoff()    ;
                }
            }

            bool isWriting() const
            {
                return m_Barrier.writerActive == 1 ;
            }

            bool isReading() const
            {
                return m_Barrier.readerCount != 0    ;
            }
        };
        typedef RWSpinRPrefT<backoff::LockDefault>    RWSpinRPref   ;

        /* AutoR/W ***********************************************************************
            �������������� ������� ��� readers/writers
        **********************************************************************************/
        template <class RWLOCK>
        class AutoR {
            RWLOCK& m_lock  ;
        public:
            AutoR( RWLOCK& lock )
                : m_lock( lock )
            {
                m_lock.rlock()  ;
            }
            ~AutoR()
            {
                m_lock.runlock() ;
            }
        };

        template <class RWLOCK>
        class AutoW {
            RWLOCK& m_lock  ;
        public:
            AutoW( RWLOCK& lock )
                : m_lock( lock )
            {
                m_lock.wlock() ;
            }
            ~AutoW()
            {
                m_lock.wunlock() ;
            }
        };
    }   // namespace lock

    // �������� ����� ���������������� �������
    //

    typedef lock::RWSpinWPref        RWSpinLock    ;

} // namespace cds

///@endcond


#endif // #ifndef __CDS_RWLOCK_H
