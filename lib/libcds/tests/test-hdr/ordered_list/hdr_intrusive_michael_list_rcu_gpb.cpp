//$$CDS-header$$

#include "ordered_list/hdr_intrusive_michael.h"
#include <cds/urcu/general_buffered.h>
#include <cds/intrusive/michael_list_rcu.h>

namespace ordlist {
    namespace {
        typedef cds::urcu::gc< cds::urcu::general_buffered<> >    RCU ;
    }

    void IntrusiveMichaelListHeaderTest::RCU_GPB_base_cmp()
    {
        typedef base_int_item< RCU > item ;
        typedef ci::MichaelList< RCU
            ,item
            ,ci::michael_list::make_traits<
                ci::opt::hook< ci::michael_list::base_hook< co::gc<RCU> > >
                ,co::compare< cmp<item> >
                ,ci::opt::disposer< faked_disposer >
            >::type
        >    list    ;
        test_rcu_int<list>()    ;
    }
    void IntrusiveMichaelListHeaderTest::RCU_GPB_base_less()
    {
        typedef base_int_item< RCU > item ;
        typedef ci::MichaelList< RCU
            ,item
            ,ci::michael_list::make_traits<
                ci::opt::hook< ci::michael_list::base_hook< co::gc<RCU> > >
                ,co::less< less<item> >
                ,ci::opt::disposer< faked_disposer >
            >::type
        >    list    ;
        test_rcu_int<list>()    ;
    }
    void IntrusiveMichaelListHeaderTest::RCU_GPB_base_cmpmix()
    {
        typedef base_int_item< RCU > item ;
        typedef ci::MichaelList< RCU
            ,item
            ,ci::michael_list::make_traits<
                ci::opt::hook< ci::michael_list::base_hook< co::gc<RCU> > >
                ,co::less< less<item> >
                ,co::compare< cmp<item> >
                ,ci::opt::disposer< faked_disposer >
            >::type
        >    list    ;
        test_rcu_int<list>()    ;
    }
    void IntrusiveMichaelListHeaderTest::RCU_GPB_base_ic()
    {
        typedef base_int_item< RCU > item ;
        typedef ci::MichaelList< RCU
            ,item
            ,ci::michael_list::make_traits<
                ci::opt::hook< ci::michael_list::base_hook< co::gc<RCU> > >
                ,co::less< less<item> >
                ,co::compare< cmp<item> >
                ,ci::opt::disposer< faked_disposer >
                ,co::item_counter< cds::atomicity::item_counter >
            >::type
        >    list    ;
        test_rcu_int<list>()    ;
    }
    void IntrusiveMichaelListHeaderTest::RCU_GPB_member_cmp()
    {
        typedef member_int_item< RCU > item ;
        typedef ci::MichaelList< RCU
            ,item
            ,ci::michael_list::make_traits<
                ci::opt::hook< ci::michael_list::member_hook<
                    offsetof( item, hMember ),
                    co::gc<RCU>
                > >
                ,co::compare< cmp<item> >
                ,ci::opt::disposer< faked_disposer >
            >::type
        >    list    ;
        test_rcu_int<list>()    ;
    }
    void IntrusiveMichaelListHeaderTest::RCU_GPB_member_less()
    {
        typedef member_int_item< RCU > item ;
        typedef ci::MichaelList< RCU
            ,item
            ,ci::michael_list::make_traits<
                ci::opt::hook< ci::michael_list::member_hook<
                    offsetof( item, hMember ),
                    co::gc<RCU>
                > >
                ,co::less< less<item> >
                ,ci::opt::disposer< faked_disposer >
            >::type
        >    list    ;
        test_rcu_int<list>()    ;
    }
    void IntrusiveMichaelListHeaderTest::RCU_GPB_member_cmpmix()
    {
        typedef member_int_item< RCU > item ;
        typedef ci::MichaelList< RCU
            ,item
            ,ci::michael_list::make_traits<
                ci::opt::hook< ci::michael_list::member_hook<
                    offsetof( item, hMember ),
                    co::gc<RCU>
                > >
                ,co::less< less<item> >
                ,co::compare< cmp<item> >
                ,ci::opt::disposer< faked_disposer >
            >::type
        >    list    ;
        test_rcu_int<list>()    ;
    }
    void IntrusiveMichaelListHeaderTest::RCU_GPB_member_ic()
    {
        typedef member_int_item< RCU > item ;
        typedef ci::MichaelList< RCU
            ,item
            ,ci::michael_list::make_traits<
                ci::opt::hook< ci::michael_list::member_hook<
                    offsetof( item, hMember ),
                    co::gc<RCU>
                > >
                ,co::compare< cmp<item> >
                ,ci::opt::disposer< faked_disposer >
                ,co::item_counter< cds::atomicity::item_counter >
            >::type
        >    list    ;
        test_rcu_int<list>()    ;
    }

}
