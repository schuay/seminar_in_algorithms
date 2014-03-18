//$$CDS-header$$

#include "set/hdr_set.h"
#include <cds/urcu/signal_threaded.h>
#include <cds/container/lazy_list_rcu.h>
#include <cds/container/michael_set_rcu.h>

namespace set {

#ifdef CDS_URCU_SIGNAL_HANDLING_ENABLED
    namespace {
        typedef cds::urcu::gc< cds::urcu::signal_threaded<> > rcu_type ;

        struct set_traits: public cc::michael_set::type_traits
        {
            typedef HashSetHdrTest::hash_int            hash            ;
            typedef HashSetHdrTest::simple_item_counter item_counter    ;
        };

        struct RCU_SHT_cmp_traits: public cc::lazy_list::type_traits
        {
            typedef HashSetHdrTest::cmp<HashSetHdrTest::item>   compare ;
        };

        struct RCU_SHT_less_traits: public cc::lazy_list::type_traits
        {
            typedef HashSetHdrTest::less<HashSetHdrTest::item>   less ;
        };

        struct RCU_SHT_cmpmix_traits: public cc::lazy_list::type_traits
        {
            typedef HashSetHdrTest::cmp<HashSetHdrTest::item>   compare ;
            typedef HashSetHdrTest::less<HashSetHdrTest::item>   less ;
        };
    }
#endif

    void HashSetHdrTest::Lazy_RCU_SHT_cmp()
    {
#ifdef CDS_URCU_SIGNAL_HANDLING_ENABLED
        typedef cc::LazyList< rcu_type, item, RCU_SHT_cmp_traits > list   ;

        // traits-based version
        typedef cc::MichaelHashSet< rcu_type, list, set_traits > set     ;
        test_int_rcu< set >()  ;

        // option-based version
        typedef cc::MichaelHashSet< rcu_type, list,
            cc::michael_set::make_traits<
                cc::opt::hash< hash_int >
                ,cc::opt::item_counter< simple_item_counter >
            >::type
        > opt_set   ;
        test_int_rcu< opt_set >()  ;
#endif
    }

    void HashSetHdrTest::Lazy_RCU_SHT_less()
    {
#ifdef CDS_URCU_SIGNAL_HANDLING_ENABLED
        typedef cc::LazyList< rcu_type, item, RCU_SHT_less_traits > list   ;

        // traits-based version
        typedef cc::MichaelHashSet< rcu_type, list, set_traits > set     ;
        test_int_rcu< set >()  ;

        // option-based version
        typedef cc::MichaelHashSet< rcu_type, list,
            cc::michael_set::make_traits<
                cc::opt::hash< hash_int >
                ,cc::opt::item_counter< simple_item_counter >
            >::type
        > opt_set   ;
        test_int_rcu< opt_set >()  ;
#endif
    }

    void HashSetHdrTest::Lazy_RCU_SHT_cmpmix()
    {
#ifdef CDS_URCU_SIGNAL_HANDLING_ENABLED
        typedef cc::LazyList< rcu_type, item, RCU_SHT_cmpmix_traits > list   ;

        // traits-based version
        typedef cc::MichaelHashSet< rcu_type, list, set_traits > set     ;
        test_int_rcu< set >()  ;

        // option-based version
        typedef cc::MichaelHashSet< rcu_type, list,
            cc::michael_set::make_traits<
                cc::opt::hash< hash_int >
                ,cc::opt::item_counter< simple_item_counter >
            >::type
        > opt_set   ;
        test_int_rcu< opt_set >()  ;
#endif
    }

} // namespace set
