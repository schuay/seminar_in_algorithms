//$$CDS-header$$

#include "ordered_list/hdr_lazy_kv.h"
#include <cds/urcu/signal_threaded.h>
#include <cds/container/lazy_kvlist_rcu.h>

namespace ordlist {
#ifdef CDS_URCU_SIGNAL_HANDLING_ENABLED
    namespace {
        typedef cds::urcu::gc< cds::urcu::signal_threaded<> > rcu_type ;

        struct RCU_SHT_cmp_traits: public cc::lazy_list::type_traits
        {
            typedef LazyKVListTestHeader::cmp<LazyKVListTestHeader::key_type>   compare ;
        };
    }
#endif
    void LazyKVListTestHeader::RCU_SHT_cmp()
    {
#ifdef CDS_URCU_SIGNAL_HANDLING_ENABLED
        // traits-based version
        typedef cc::LazyKVList< rcu_type, key_type, value_type, RCU_SHT_cmp_traits > list   ;
        test_rcu< list >()  ;

        // option-based version

        typedef cc::LazyKVList< rcu_type, key_type, value_type,
            cc::lazy_list::make_traits<
                cc::opt::compare< cmp<key_type> >
            >::type
        > opt_list   ;
        test_rcu< opt_list >()  ;
#endif
    }

#ifdef CDS_URCU_SIGNAL_HANDLING_ENABLED
    namespace {
        struct RCU_SHT_less_traits: public cc::lazy_list::type_traits
        {
            typedef LazyKVListTestHeader::lt<LazyKVListTestHeader::key_type>   less ;
        };
    }
#endif
    void LazyKVListTestHeader::RCU_SHT_less()
    {
#ifdef CDS_URCU_SIGNAL_HANDLING_ENABLED
        // traits-based version
        typedef cc::LazyKVList< rcu_type, key_type, value_type, RCU_SHT_less_traits > list   ;
        test_rcu< list >()  ;

        // option-based version

        typedef cc::LazyKVList< rcu_type, key_type, value_type,
            cc::lazy_list::make_traits<
                cc::opt::less< lt<key_type> >
            >::type
        > opt_list   ;
        test_rcu< opt_list >()  ;
#endif
    }

#ifdef CDS_URCU_SIGNAL_HANDLING_ENABLED
    namespace {
        struct RCU_SHT_cmpmix_traits: public cc::lazy_list::type_traits
        {
            typedef LazyKVListTestHeader::cmp<LazyKVListTestHeader::key_type>   compare ;
            typedef LazyKVListTestHeader::lt<LazyKVListTestHeader::key_type>  less    ;
        };
    }
#endif
    void LazyKVListTestHeader::RCU_SHT_cmpmix()
    {
#ifdef CDS_URCU_SIGNAL_HANDLING_ENABLED
        // traits-based version
        typedef cc::LazyKVList< rcu_type, key_type, value_type, RCU_SHT_cmpmix_traits > list   ;
        test_rcu< list >()  ;

        // option-based version

        typedef cc::LazyKVList< rcu_type, key_type, value_type,
            cc::lazy_list::make_traits<
                cc::opt::compare< cmp<key_type> >
                ,cc::opt::less< lt<key_type> >
            >::type
        > opt_list   ;
        test_rcu< opt_list >()  ;
#endif
    }

#ifdef CDS_URCU_SIGNAL_HANDLING_ENABLED
    namespace {
        struct RCU_SHT_ic_traits: public cc::lazy_list::type_traits
        {
            typedef LazyKVListTestHeader::lt<LazyKVListTestHeader::key_type>   less ;
            typedef cds::atomicity::item_counter item_counter   ;
        };
    }
#endif
    void LazyKVListTestHeader::RCU_SHT_ic()
    {
#ifdef CDS_URCU_SIGNAL_HANDLING_ENABLED
        // traits-based version
        typedef cc::LazyKVList< rcu_type, key_type, value_type, RCU_SHT_ic_traits > list   ;
        test_rcu< list >()  ;

        // option-based version

        typedef cc::LazyKVList< rcu_type, key_type, value_type,
            cc::lazy_list::make_traits<
                cc::opt::less< lt<key_type> >
                ,cc::opt::item_counter< cds::atomicity::item_counter >
            >::type
        > opt_list   ;
        test_rcu< opt_list >()  ;
#endif
    }

}   // namespace ordlist

