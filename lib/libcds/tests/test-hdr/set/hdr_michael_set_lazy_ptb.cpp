//$$CDS-header$$

#include "set/hdr_set.h"
#include <cds/container/lazy_list_ptb.h>
#include <cds/container/michael_set.h>

namespace set {

    namespace {
        struct set_traits: public cc::michael_set::type_traits
        {
            typedef HashSetHdrTest::hash_int            hash            ;
            typedef HashSetHdrTest::simple_item_counter item_counter    ;
        };

        struct PTB_cmp_traits: public cc::lazy_list::type_traits
        {
            typedef HashSetHdrTest::cmp<HashSetHdrTest::item>   compare ;
        };

        struct PTB_less_traits: public cc::lazy_list::type_traits
        {
            typedef HashSetHdrTest::less<HashSetHdrTest::item>   less ;
        };

        struct PTB_cmpmix_traits: public cc::lazy_list::type_traits
        {
            typedef HashSetHdrTest::cmp<HashSetHdrTest::item>   compare ;
            typedef HashSetHdrTest::less<HashSetHdrTest::item>   less ;
        };
    }

    void HashSetHdrTest::Lazy_PTB_cmp()
    {
        typedef cc::LazyList< cds::gc::PTB, item, PTB_cmp_traits > list   ;

        // traits-based version
        typedef cc::MichaelHashSet< cds::gc::PTB, list, set_traits > set     ;
        test_int< set >()  ;

        // option-based version
        typedef cc::MichaelHashSet< cds::gc::PTB, list,
            cc::michael_set::make_traits<
                cc::opt::hash< hash_int >
                ,cc::opt::item_counter< simple_item_counter >
            >::type
        > opt_set   ;
        test_int< opt_set >()  ;
    }

    void HashSetHdrTest::Lazy_PTB_less()
    {
        typedef cc::LazyList< cds::gc::PTB, item, PTB_less_traits > list   ;

        // traits-based version
        typedef cc::MichaelHashSet< cds::gc::PTB, list, set_traits > set     ;
        test_int< set >()  ;

        // option-based version
        typedef cc::MichaelHashSet< cds::gc::PTB, list,
            cc::michael_set::make_traits<
                cc::opt::hash< hash_int >
                ,cc::opt::item_counter< simple_item_counter >
            >::type
        > opt_set   ;
        test_int< opt_set >()  ;
    }

    void HashSetHdrTest::Lazy_PTB_cmpmix()
    {
        typedef cc::LazyList< cds::gc::PTB, item, PTB_cmpmix_traits > list   ;

        // traits-based version
        typedef cc::MichaelHashSet< cds::gc::PTB, list, set_traits > set     ;
        test_int< set >()  ;

        // option-based version
        typedef cc::MichaelHashSet< cds::gc::PTB, list,
            cc::michael_set::make_traits<
                cc::opt::hash< hash_int >
                ,cc::opt::item_counter< simple_item_counter >
            >::type
        > opt_set   ;
        test_int< opt_set >()  ;
    }


} // namespace set
