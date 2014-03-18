//$$CDS-header$$

#include "ordered_list/hdr_michael_kv.h"
#include <cds/container/michael_kvlist_ptb.h>

namespace ordlist {
    namespace {
        struct PTB_cmp_traits: public cc::michael_list::type_traits
        {
            typedef MichaelKVListTestHeader::cmp<MichaelKVListTestHeader::key_type>   compare ;
        };
    }
    void MichaelKVListTestHeader::PTB_cmp()
    {
        // traits-based version
        typedef cc::MichaelKVList< cds::gc::PTB, key_type, value_type, PTB_cmp_traits > list   ;
        test< list >()  ;

        // option-based version

        typedef cc::MichaelKVList< cds::gc::PTB, key_type, value_type,
            cc::michael_list::make_traits<
                cc::opt::compare< cmp<key_type> >
            >::type
        > opt_list   ;
        test< opt_list >()  ;
    }

    namespace {
        struct PTB_less_traits: public cc::michael_list::type_traits
        {
            typedef MichaelKVListTestHeader::lt<MichaelKVListTestHeader::key_type>   less ;
        };
    }
    void MichaelKVListTestHeader::PTB_less()
    {
        // traits-based version
        typedef cc::MichaelKVList< cds::gc::PTB, key_type, value_type, PTB_less_traits > list   ;
        test< list >()  ;

        // option-based version

        typedef cc::MichaelKVList< cds::gc::PTB, key_type, value_type,
            cc::michael_list::make_traits<
                cc::opt::less< lt<key_type> >
            >::type
        > opt_list   ;
        test< opt_list >()  ;
    }

    namespace {
        struct PTB_cmpmix_traits: public cc::michael_list::type_traits
        {
            typedef MichaelKVListTestHeader::cmp<MichaelKVListTestHeader::key_type>   compare ;
            typedef MichaelKVListTestHeader::lt<MichaelKVListTestHeader::key_type>  less    ;
        };
    }
    void MichaelKVListTestHeader::PTB_cmpmix()
    {
        // traits-based version
        typedef cc::MichaelKVList< cds::gc::PTB, key_type, value_type, PTB_cmpmix_traits > list   ;
        test< list >()  ;

        // option-based version

        typedef cc::MichaelKVList< cds::gc::PTB, key_type, value_type,
            cc::michael_list::make_traits<
                cc::opt::compare< cmp<key_type> >
                ,cc::opt::less< lt<key_type> >
            >::type
        > opt_list   ;
        test< opt_list >()  ;
    }

    namespace {
        struct PTB_ic_traits: public cc::michael_list::type_traits
        {
            typedef MichaelKVListTestHeader::lt<MichaelKVListTestHeader::key_type>   less ;
            typedef cds::atomicity::item_counter item_counter   ;
        };
    }
    void MichaelKVListTestHeader::PTB_ic()
    {
        // traits-based version
        typedef cc::MichaelKVList< cds::gc::PTB, key_type, value_type, PTB_ic_traits > list   ;
        test< list >()  ;

        // option-based version

        typedef cc::MichaelKVList< cds::gc::PTB, key_type, value_type,
            cc::michael_list::make_traits<
                cc::opt::less< lt<key_type> >
                ,cc::opt::item_counter< cds::atomicity::item_counter >
            >::type
        > opt_list   ;
        test< opt_list >()  ;
    }

}   // namespace ordlist

