//$$CDS-header$$

#include "ordered_list/hdr_michael.h"
#include <cds/urcu/general_buffered.h>
#include <cds/container/michael_list_rcu.h>

namespace ordlist {
    namespace {
        typedef cds::urcu::gc< cds::urcu::general_buffered<> >    rcu_type ;

        struct RCU_GPB_cmp_traits: public cc::michael_list::type_traits
        {
            typedef MichaelListTestHeader::cmp<MichaelListTestHeader::item>   compare ;
        };
    }

    void MichaelListTestHeader::RCU_GPB_cmp()
    {
        // traits-based version
        typedef cc::MichaelList< rcu_type, item, RCU_GPB_cmp_traits > list   ;
        test_rcu< list >()  ;

        // option-based version

        typedef cc::MichaelList< rcu_type, item,
            cc::michael_list::make_traits<
                cc::opt::compare< cmp<item> >
            >::type
        > opt_list   ;
        test_rcu< opt_list >()  ;
    }

    namespace {
        struct RCU_GPB_less_traits: public cc::michael_list::type_traits
        {
            typedef MichaelListTestHeader::lt<MichaelListTestHeader::item>   less ;
        };
    }
    void MichaelListTestHeader::RCU_GPB_less()
    {
        // traits-based version
        typedef cc::MichaelList< rcu_type, item, RCU_GPB_less_traits > list   ;
        test_rcu< list >()  ;

        // option-based version

        typedef cc::MichaelList< rcu_type, item,
            cc::michael_list::make_traits<
                cc::opt::less< lt<item> >
            >::type
        > opt_list   ;
        test_rcu< opt_list >()  ;
    }

    namespace {
        struct RCU_GPB_cmpmix_traits: public cc::michael_list::type_traits
        {
            typedef MichaelListTestHeader::cmp<MichaelListTestHeader::item>   compare ;
            typedef MichaelListTestHeader::lt<MichaelListTestHeader::item>  less    ;
        };
    }
    void MichaelListTestHeader::RCU_GPB_cmpmix()
    {
        // traits-based version
        typedef cc::MichaelList< rcu_type, item, RCU_GPB_cmpmix_traits > list   ;
        test_rcu< list >()  ;

        // option-based version

        typedef cc::MichaelList< rcu_type, item,
            cc::michael_list::make_traits<
                cc::opt::compare< cmp<item> >
                ,cc::opt::less< lt<item> >
            >::type
        > opt_list   ;
        test_rcu< opt_list >()  ;
    }

    namespace {
        struct RCU_GPB_ic_traits: public cc::michael_list::type_traits
        {
            typedef MichaelListTestHeader::lt<MichaelListTestHeader::item>   less ;
            typedef cds::atomicity::item_counter item_counter   ;
        };
    }
    void MichaelListTestHeader::RCU_GPB_ic()
    {
        // traits-based version
        typedef cc::MichaelList< rcu_type, item, RCU_GPB_ic_traits > list   ;
        test_rcu< list >()  ;

        // option-based version

        typedef cc::MichaelList< rcu_type, item,
            cc::michael_list::make_traits<
                cc::opt::less< lt<item> >
                ,cc::opt::item_counter< cds::atomicity::item_counter >
            >::type
        > opt_list   ;
        test_rcu< opt_list >()  ;
    }

}   // namespace ordlist

