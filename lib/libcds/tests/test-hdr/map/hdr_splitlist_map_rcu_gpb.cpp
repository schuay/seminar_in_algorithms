//$$CDS-header$$

#include "map/hdr_map.h"
#include <cds/urcu/general_buffered.h>
#include <cds/container/michael_list_rcu.h>
#include <cds/container/split_list_map_rcu.h>

namespace map {

    namespace {
        typedef cds::urcu::gc< cds::urcu::general_buffered<> >  rcu_type ;

        struct RCU_GPB_cmp_traits: public cc::split_list::type_traits
        {
            typedef cc::michael_list_tag                ordered_list    ;
            typedef HashMapHdrTest::hash_int            hash            ;
            typedef HashMapHdrTest::simple_item_counter item_counter    ;
            typedef cc::opt::v::relaxed_ordering        memory_model    ;
            enum { dynamic_bucket_table = false };

            struct ordered_list_traits: public cc::michael_list::type_traits
            {
                typedef HashMapHdrTest::cmp   compare ;
            } ;
        };

        struct RCU_GPB_less_traits: public cc::split_list::type_traits
        {
            typedef cc::michael_list_tag                ordered_list    ;
            typedef HashMapHdrTest::hash_int            hash            ;
            typedef HashMapHdrTest::simple_item_counter item_counter    ;
            typedef cc::opt::v::sequential_consistent                      memory_model    ;
            enum { dynamic_bucket_table = true };

            struct ordered_list_traits: public cc::michael_list::type_traits
            {
                typedef HashMapHdrTest::less   less ;
            };
        };

        struct RCU_GPB_cmpmix_traits: public cc::split_list::type_traits
        {
            typedef cc::michael_list_tag                ordered_list    ;
            typedef HashMapHdrTest::hash_int            hash            ;
            typedef HashMapHdrTest::simple_item_counter item_counter    ;

            struct ordered_list_traits: public cc::michael_list::type_traits
            {
                typedef HashMapHdrTest::cmp   compare ;
                typedef std::less<HashMapHdrTest::key_type>     less ;
            };
        };
    }

    void HashMapHdrTest::Split_RCU_GPB_cmp()
    {
        // traits-based version
        typedef cc::SplitListMap< rcu_type, key_type, value_type, RCU_GPB_cmp_traits > map_type     ;
        test_int< map_type >()  ;

        // option-based version
        typedef cc::SplitListMap< rcu_type,
            key_type,
            value_type,
            cc::split_list::make_traits<
                cc::split_list::ordered_list<cc::michael_list_tag>
                ,cc::opt::hash< hash_int >
                ,cc::opt::item_counter< simple_item_counter >
                ,cc::opt::memory_model< cc::opt::v::relaxed_ordering >
                ,cc::split_list::dynamic_bucket_table< true >
                ,cc::split_list::ordered_list_traits<
                    cc::michael_list::make_traits<
                        cc::opt::compare< cmp >
                    >::type
                >
            >::type
        > opt_map   ;
        test_int< opt_map >()  ;
    }

    void HashMapHdrTest::Split_RCU_GPB_less()
    {
        // traits-based version
        typedef cc::SplitListMap< rcu_type, key_type, value_type, RCU_GPB_less_traits > map_type     ;
        test_int< map_type >()  ;

        // option-based version
        typedef cc::SplitListMap< rcu_type,
            key_type,
            value_type,
            cc::split_list::make_traits<
                cc::split_list::ordered_list<cc::michael_list_tag>
                ,cc::opt::hash< hash_int >
                ,cc::opt::item_counter< simple_item_counter >
                ,cc::opt::memory_model< cc::opt::v::relaxed_ordering >
                ,cc::split_list::dynamic_bucket_table< false >
                ,cc::split_list::ordered_list_traits<
                    cc::michael_list::make_traits<
                        cc::opt::less< less >
                    >::type
                >
            >::type
        > opt_map   ;
        test_int< opt_map >()  ;
    }

    void HashMapHdrTest::Split_RCU_GPB_cmpmix()
    {
        // traits-based version
        typedef cc::SplitListMap< rcu_type, key_type, value_type, RCU_GPB_cmpmix_traits > map_type   ;
        test_int< map_type >()  ;

        // option-based version
        typedef cc::SplitListMap< rcu_type,
            key_type,
            value_type,
            cc::split_list::make_traits<
                cc::split_list::ordered_list<cc::michael_list_tag>
                ,cc::opt::hash< hash_int >
                ,cc::opt::item_counter< simple_item_counter >
                ,cc::split_list::ordered_list_traits<
                    cc::michael_list::make_traits<
                    cc::opt::less< std::less<key_type> >
                        ,cc::opt::compare< cmp >
                    >::type
                >
            >::type
        > opt_map   ;
        test_int< opt_map >()  ;
    }


} // namespace map

