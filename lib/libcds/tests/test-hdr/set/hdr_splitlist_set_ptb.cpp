//$$CDS-header$$

#include "set/hdr_set.h"
#include <cds/container/michael_list_ptb.h>
#include <cds/container/split_list_set.h>

namespace set {

    namespace {
        struct PTB_cmp_traits: public cc::split_list::type_traits
        {
            typedef cc::michael_list_tag                ordered_list    ;
            typedef HashSetHdrTest::hash_int            hash            ;
            typedef HashSetHdrTest::simple_item_counter item_counter    ;
            typedef cc::opt::v::relaxed_ordering        memory_model    ;
            enum { dynamic_bucket_table = false };

            struct ordered_list_traits: public cc::michael_list::type_traits
            {
                typedef HashSetHdrTest::cmp<HashSetHdrTest::item>   compare ;
            } ;
        };

        struct PTB_less_traits: public cc::split_list::type_traits
        {
            typedef cc::michael_list_tag                ordered_list    ;
            typedef HashSetHdrTest::hash_int            hash            ;
            typedef HashSetHdrTest::simple_item_counter item_counter    ;
            typedef cc::opt::v::sequential_consistent   memory_model    ;
            enum { dynamic_bucket_table = false };

            struct ordered_list_traits: public cc::michael_list::type_traits
            {
                typedef HashSetHdrTest::less<HashSetHdrTest::item>   less ;
            };
        };

        struct PTB_cmpmix_traits: public cc::split_list::type_traits
        {
            typedef cc::michael_list_tag                ordered_list    ;
            typedef HashSetHdrTest::hash_int            hash            ;
            typedef HashSetHdrTest::simple_item_counter item_counter    ;

            struct ordered_list_traits: public cc::michael_list::type_traits
            {
                typedef HashSetHdrTest::cmp<HashSetHdrTest::item>   compare ;
                typedef HashSetHdrTest::less<HashSetHdrTest::item>   less   ;
            };
        };
    }

    void HashSetHdrTest::Split_PTB_cmp()
    {
        // traits-based version
        typedef cc::SplitListSet< cds::gc::PTB, item, PTB_cmp_traits > set   ;

        test_int< set >()  ;

        // option-based version
        typedef cc::SplitListSet< cds::gc::PTB, item,
            cc::split_list::make_traits<
                cc::split_list::ordered_list<cc::michael_list_tag>
                ,cc::opt::hash< hash_int >
                ,cc::opt::item_counter< simple_item_counter >
                ,cc::opt::memory_model< cc::opt::v::relaxed_ordering >
                ,cc::split_list::dynamic_bucket_table< true >
                ,cc::split_list::ordered_list_traits<
                    cc::michael_list::make_traits<
                        cc::opt::compare< cmp<item> >
                    >::type
                >
            >::type
        > opt_set   ;
        test_int< opt_set >()  ;
    }

    void HashSetHdrTest::Split_PTB_less()
    {
        // traits-based version
        typedef cc::SplitListSet< cds::gc::PTB, item, PTB_less_traits > set   ;

        test_int< set >()  ;

        // option-based version
        typedef cc::SplitListSet< cds::gc::PTB, item,
            cc::split_list::make_traits<
                cc::split_list::ordered_list<cc::michael_list_tag>
                ,cc::opt::hash< hash_int >
                ,cc::opt::item_counter< simple_item_counter >
                ,cc::opt::memory_model< cc::opt::v::sequential_consistent >
                ,cc::split_list::dynamic_bucket_table< false >
                ,cc::split_list::ordered_list_traits<
                    cc::michael_list::make_traits<
                        cc::opt::less< less<item> >
                    >::type
                >
            >::type
        > opt_set   ;
        test_int< opt_set >()  ;
    }

    void HashSetHdrTest::Split_PTB_cmpmix()
    {
        // traits-based version
        typedef cc::SplitListSet< cds::gc::PTB, item, PTB_cmpmix_traits > set   ;
        test_int< set >()  ;

        // option-based version
        typedef cc::SplitListSet< cds::gc::PTB, item,
            cc::split_list::make_traits<
                cc::split_list::ordered_list<cc::michael_list_tag>
                ,cc::opt::hash< hash_int >
                ,cc::opt::item_counter< simple_item_counter >
                ,cc::split_list::ordered_list_traits<
                    cc::michael_list::make_traits<
                        cc::opt::less< less<item> >
                        ,cc::opt::compare< cmp<item> >
                    >::type
                >
            >::type
        > opt_set   ;
        test_int< opt_set >()  ;
    }


} // namespace set


