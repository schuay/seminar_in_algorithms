//$$CDS-header$$

#include "set/hdr_intrusive_striped_set.h"
#include <cds/intrusive/striped_set/boost_list.h>
#include <cds/intrusive/striped_set.h>

#include <cds/details/std/type_traits.h>    // std::is_same

namespace set {
    namespace bi = boost::intrusive ;

    namespace {
        typedef IntrusiveStripedSetHdrTest::base_item< bi::list_base_hook<> > base_item_type ;
        typedef IntrusiveStripedSetHdrTest::member_item< bi::list_member_hook<> > member_item_type ;
    }

    void IntrusiveStripedSetHdrTest::Striped_list_basehook_cmp()
    {
        typedef ci::StripedSet<
            bi::list<base_item_type>
            ,co::mutex_policy< ci::striped_set::striping<> >
            ,co::hash< IntrusiveStripedSetHdrTest::hash_int >
            ,co::compare< IntrusiveStripedSetHdrTest::cmp<base_item_type> >
        > set_type ;

        static_assert( (std::is_same<
            IntrusiveStripedSetHdrTest::cmp<base_item_type>
            ,set_type::bucket_type::key_comparator
        >::value), "Key compare function selection error" ) ;

        test<set_type>() ;
    }

    void IntrusiveStripedSetHdrTest::Striped_list_basehook_less()
    {
        typedef ci::StripedSet<
            bi::list<base_item_type>
            ,co::hash< IntrusiveStripedSetHdrTest::hash_int >
            ,co::less< IntrusiveStripedSetHdrTest::less<base_item_type> >
        > set_type ;

        test<set_type>() ;
    }

    void IntrusiveStripedSetHdrTest::Striped_list_basehook_cmpmix()
    {
        typedef ci::StripedSet<
            bi::list<base_item_type>
            ,co::hash< IntrusiveStripedSetHdrTest::hash_int >
            ,co::less< IntrusiveStripedSetHdrTest::less<base_item_type> >
            ,co::compare< IntrusiveStripedSetHdrTest::cmp<base_item_type> >
        > set_type ;

        static_assert( (std::is_same<
            IntrusiveStripedSetHdrTest::cmp<base_item_type>
            ,set_type::bucket_type::key_comparator
        >::value), "Key compare function selection error" ) ;

        test<set_type>() ;
    }

    void IntrusiveStripedSetHdrTest::Striped_list_basehook_bucket_threshold()
    {
        typedef ci::StripedSet<
            bi::list<base_item_type>
            ,co::hash< IntrusiveStripedSetHdrTest::hash_int >
            ,co::less< IntrusiveStripedSetHdrTest::less<base_item_type> >
            ,co::compare< IntrusiveStripedSetHdrTest::cmp<base_item_type> >
            ,co::resizing_policy< ci::striped_set::single_bucket_size_threshold<8> >
        > set_type ;

        static_assert( (std::is_same<
            IntrusiveStripedSetHdrTest::cmp<base_item_type>
            ,set_type::bucket_type::key_comparator
        >::value), "Key compare function selection error" ) ;

        test<set_type>() ;
    }

    void IntrusiveStripedSetHdrTest::Striped_list_basehook_bucket_threshold_rt()
    {
        typedef ci::StripedSet<
            bi::list<base_item_type>
            ,co::hash< IntrusiveStripedSetHdrTest::hash_int >
            ,co::less< IntrusiveStripedSetHdrTest::less<base_item_type> >
            ,co::compare< IntrusiveStripedSetHdrTest::cmp<base_item_type> >
            ,co::resizing_policy< ci::striped_set::single_bucket_size_threshold<0> >
        > set_type ;

        static_assert( (std::is_same<
            IntrusiveStripedSetHdrTest::cmp<base_item_type>
            ,set_type::bucket_type::key_comparator
        >::value), "Key compare function selection error" ) ;

        set_type s( 128, ci::striped_set::single_bucket_size_threshold<0>(4) ) ;
        test_with( s ) ;
    }

    void IntrusiveStripedSetHdrTest::Striped_list_memberhook_cmp()
    {
        typedef ci::StripedSet<
            bi::list<
                member_item_type
                , bi::member_hook< member_item_type, bi::list_member_hook<>, &member_item_type::hMember>
            >
            ,co::hash< IntrusiveStripedSetHdrTest::hash_int >
            ,co::compare< IntrusiveStripedSetHdrTest::cmp<member_item_type> >
        > set_type ;

        test<set_type>() ;
    }

    void IntrusiveStripedSetHdrTest::Striped_list_memberhook_less()
    {
        typedef ci::StripedSet<
            bi::list<
                member_item_type
                , bi::member_hook< member_item_type, bi::list_member_hook<>, &member_item_type::hMember>
            >
            ,co::hash< IntrusiveStripedSetHdrTest::hash_int >
            ,co::less< IntrusiveStripedSetHdrTest::less<member_item_type> >
        > set_type ;

        test<set_type>() ;
    }

    void IntrusiveStripedSetHdrTest::Striped_list_memberhook_cmpmix()
    {
        typedef ci::StripedSet<
            bi::list<
                member_item_type
                , bi::member_hook< member_item_type, bi::list_member_hook<>, &member_item_type::hMember>
            >
            ,co::hash< IntrusiveStripedSetHdrTest::hash_int >
            ,co::less< IntrusiveStripedSetHdrTest::less<member_item_type> >
            ,co::compare< IntrusiveStripedSetHdrTest::cmp<member_item_type> >
        > set_type ;

        test<set_type>() ;
    }

    void IntrusiveStripedSetHdrTest::Striped_list_memberhook_bucket_threshold()
    {
        typedef ci::StripedSet<
            bi::list<
                member_item_type
                , bi::member_hook< member_item_type, bi::list_member_hook<>, &member_item_type::hMember>
            >
            ,co::hash< IntrusiveStripedSetHdrTest::hash_int >
            ,co::less< IntrusiveStripedSetHdrTest::less<member_item_type> >
            ,co::compare< IntrusiveStripedSetHdrTest::cmp<member_item_type> >
            ,co::resizing_policy< ci::striped_set::single_bucket_size_threshold<8> >
        > set_type ;

        static_assert( (std::is_same<
            IntrusiveStripedSetHdrTest::cmp<member_item_type>
            ,set_type::bucket_type::key_comparator
        >::value), "Key compare function selection error" ) ;

        test<set_type>() ;
    }

    void IntrusiveStripedSetHdrTest::Striped_list_memberhook_bucket_threshold_rt()
    {
        typedef ci::StripedSet<
            bi::list<
            member_item_type
            , bi::member_hook< member_item_type, bi::list_member_hook<>, &member_item_type::hMember>
            >
            ,co::hash< IntrusiveStripedSetHdrTest::hash_int >
            ,co::less< IntrusiveStripedSetHdrTest::less<member_item_type> >
            ,co::compare< IntrusiveStripedSetHdrTest::cmp<member_item_type> >
            ,co::resizing_policy< ci::striped_set::single_bucket_size_threshold<0> >
        > set_type ;

        static_assert( (std::is_same<
            IntrusiveStripedSetHdrTest::cmp<member_item_type>
            ,set_type::bucket_type::key_comparator
        >::value), "Key compare function selection error" ) ;

        set_type s( 128, ci::striped_set::single_bucket_size_threshold<0>(4) ) ;
        test_with( s ) ;
    }

} // namespace set


