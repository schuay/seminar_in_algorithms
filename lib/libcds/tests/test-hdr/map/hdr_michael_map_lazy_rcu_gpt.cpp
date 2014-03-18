//$$CDS-header$$

#include "map/hdr_map.h"
#include <cds/urcu/general_threaded.h>
#include <cds/container/lazy_kvlist_rcu.h>
#include <cds/container/michael_map.h>

namespace map {
    namespace {
        struct map_traits: public cc::michael_map::type_traits
        {
            typedef HashMapHdrTest::hash_int            hash            ;
            typedef HashMapHdrTest::simple_item_counter item_counter    ;
        };
        typedef cds::urcu::gc< cds::urcu::general_threaded<> > rcu_type ;

        struct RCU_GPT_cmp_traits: public cc::lazy_list::type_traits
        {
            typedef HashMapHdrTest::cmp   compare ;
        };

        struct RCU_GPT_less_traits: public cc::lazy_list::type_traits
        {
            typedef HashMapHdrTest::less  less ;
        };

        struct RCU_GPT_cmpmix_traits: public cc::lazy_list::type_traits
        {
            typedef HashMapHdrTest::cmp   compare ;
            typedef HashMapHdrTest::less  less ;
        };
    }

    void HashMapHdrTest::Lazy_RCU_GPT_cmp()
    {
        typedef cc::LazyKVList< rcu_type, int, HashMapHdrTest::value_type, RCU_GPT_cmp_traits > list   ;

        // traits-based version
        typedef cc::MichaelHashMap< rcu_type, list, map_traits > map     ;
        test_int< map >()  ;

        // option-based version
        typedef cc::MichaelHashMap< rcu_type, list,
            cc::michael_map::make_traits<
                cc::opt::hash< hash_int >
                ,cc::opt::item_counter< simple_item_counter >
            >::type
        > opt_map   ;
        test_int< opt_map >()  ;
    }

    void HashMapHdrTest::Lazy_RCU_GPT_less()
    {
        typedef cc::LazyKVList< rcu_type, int, HashMapHdrTest::value_type, RCU_GPT_less_traits > list   ;

        // traits-based version
        typedef cc::MichaelHashMap< rcu_type, list, map_traits > map     ;
        test_int< map >()  ;

        // option-based version
        typedef cc::MichaelHashMap< rcu_type, list,
            cc::michael_map::make_traits<
                cc::opt::hash< hash_int >
                ,cc::opt::item_counter< simple_item_counter >
            >::type
        > opt_map   ;
        test_int< opt_map >()  ;
    }

    void HashMapHdrTest::Lazy_RCU_GPT_cmpmix()
    {
        typedef cc::LazyKVList< rcu_type, int, HashMapHdrTest::value_type, RCU_GPT_cmpmix_traits > list   ;

        // traits-based version
        typedef cc::MichaelHashMap< rcu_type, list, map_traits > map     ;
        test_int< map >()  ;

        // option-based version
        typedef cc::MichaelHashMap< rcu_type, list,
            cc::michael_map::make_traits<
                cc::opt::hash< hash_int >
                ,cc::opt::item_counter< simple_item_counter >
            >::type
        > opt_map   ;
        test_int< opt_map >()  ;
    }


} // namespace map

