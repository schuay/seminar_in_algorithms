//$$CDS-header$$

#ifndef __CDS_OPT_MAKE_OPTIONS_VAR_H
#define __CDS_OPT_MAKE_OPTIONS_VAR_H

#ifndef __CDS_OPT_OPTIONS_H
#   error <cds/opt/options.h> must be included instead of <cds/opt/make_options_var.h>
#endif

#define CDS_DECL_OPTIONS1   typename... Options
#define CDS_DECL_OPTIONS2   typename... Options
#define CDS_DECL_OPTIONS3   typename... Options
#define CDS_DECL_OPTIONS4   typename... Options
#define CDS_DECL_OPTIONS5   typename... Options
#define CDS_DECL_OPTIONS6   typename... Options
#define CDS_DECL_OPTIONS7   typename... Options
#define CDS_DECL_OPTIONS8   typename... Options
#define CDS_DECL_OPTIONS9   typename... Options
#define CDS_DECL_OPTIONS10  typename... Options
#define CDS_DECL_OPTIONS11  typename... Options
#define CDS_DECL_OPTIONS12  typename... Options
#define CDS_DECL_OPTIONS13  typename... Options
#define CDS_DECL_OPTIONS14  typename... Options
#define CDS_DECL_OPTIONS15  typename... Options
#define CDS_DECL_OPTIONS16  typename... Options

#define CDS_DECL_OPTIONS    typename... Options


#define CDS_DECL_OTHER_OPTIONS1   typename... Options2
#define CDS_DECL_OTHER_OPTIONS2   typename... Options2
#define CDS_DECL_OTHER_OPTIONS3   typename... Options2
#define CDS_DECL_OTHER_OPTIONS4   typename... Options2
#define CDS_DECL_OTHER_OPTIONS5   typename... Options2
#define CDS_DECL_OTHER_OPTIONS6   typename... Options2
#define CDS_DECL_OTHER_OPTIONS7   typename... Options2
#define CDS_DECL_OTHER_OPTIONS8   typename... Options2
#define CDS_DECL_OTHER_OPTIONS9   typename... Options2
#define CDS_DECL_OTHER_OPTIONS10  typename... Options2
#define CDS_DECL_OTHER_OPTIONS11  typename... Options2
#define CDS_DECL_OTHER_OPTIONS12  typename... Options2
#define CDS_DECL_OTHER_OPTIONS13  typename... Options2
#define CDS_DECL_OTHER_OPTIONS14  typename... Options2
#define CDS_DECL_OTHER_OPTIONS15  typename... Options2
#define CDS_DECL_OTHER_OPTIONS16  typename... Options2

// for template specializations
#define CDS_SPEC_OPTIONS1   typename... Options
#define CDS_SPEC_OPTIONS2   typename... Options
#define CDS_SPEC_OPTIONS3   typename... Options
#define CDS_SPEC_OPTIONS4   typename... Options
#define CDS_SPEC_OPTIONS5   typename... Options
#define CDS_SPEC_OPTIONS6   typename... Options
#define CDS_SPEC_OPTIONS7   typename... Options
#define CDS_SPEC_OPTIONS8   typename... Options
#define CDS_SPEC_OPTIONS9   typename... Options
#define CDS_SPEC_OPTIONS10  typename... Options
#define CDS_SPEC_OPTIONS11  typename... Options
#define CDS_SPEC_OPTIONS12  typename... Options
#define CDS_SPEC_OPTIONS13  typename... Options
#define CDS_SPEC_OPTIONS14  typename... Options
#define CDS_SPEC_OPTIONS15  typename... Options
#define CDS_SPEC_OPTIONS16  typename... Options

#define CDS_SPEC_OPTIONS    typename... Options

#define CDS_OPTIONS1    Options...
#define CDS_OPTIONS2    Options...
#define CDS_OPTIONS3    Options...
#define CDS_OPTIONS4    Options...
#define CDS_OPTIONS5    Options...
#define CDS_OPTIONS6    Options...
#define CDS_OPTIONS7    Options...
#define CDS_OPTIONS8    Options...
#define CDS_OPTIONS9    Options...
#define CDS_OPTIONS10   Options...
#define CDS_OPTIONS11   Options...
#define CDS_OPTIONS12   Options...
#define CDS_OPTIONS13   Options...
#define CDS_OPTIONS14   Options...
#define CDS_OPTIONS15   Options...
#define CDS_OPTIONS16   Options...
//#define CDS_OPTIONS17   Options...
//#define CDS_OPTIONS18   Options...
//#define CDS_OPTIONS19   Options...
//#define CDS_OPTIONS20   Options...

#define CDS_OPTIONS     Options...

#define CDS_OTHER_OPTIONS1    Options2...
#define CDS_OTHER_OPTIONS2    Options2...
#define CDS_OTHER_OPTIONS3    Options2...
#define CDS_OTHER_OPTIONS4    Options2...
#define CDS_OTHER_OPTIONS5    Options2...
#define CDS_OTHER_OPTIONS6    Options2...
#define CDS_OTHER_OPTIONS7    Options2...
#define CDS_OTHER_OPTIONS8    Options2...
#define CDS_OTHER_OPTIONS9    Options2...
#define CDS_OTHER_OPTIONS10   Options2...
#define CDS_OTHER_OPTIONS11   Options2...
#define CDS_OTHER_OPTIONS12   Options2...
#define CDS_OTHER_OPTIONS13   Options2...
#define CDS_OTHER_OPTIONS14   Options2...
#define CDS_OTHER_OPTIONS15   Options2...
#define CDS_OTHER_OPTIONS16   Options2...

namespace cds { namespace opt {

    //@cond
    namespace details {
        template <typename OptionList, typename Option>
        struct do_pack
        {
            // Use "pack" member template to pack options
            typedef typename Option::template pack<OptionList> type;
        };

        template <typename ...T> class typelist ;

        template <typename Typelist> struct typelist_head ;
        template <typename Head, typename ...Tail>
        struct typelist_head< typelist<Head, Tail...> > {
            typedef Head type   ;
        };
        template <typename Head>
        struct typelist_head< typelist<Head> > {
            typedef Head type   ;
        };

        template <typename Typelist> struct typelist_tail ;
        template <typename Head, typename ...Tail>
        struct typelist_tail< typelist<Head, Tail...> > {
            typedef typelist<Tail...> type   ;
        };
        template <typename Head>
        struct typelist_tail< typelist<Head> > {
            typedef typelist<> type   ;
        };

        template <typename OptionList, typename Typelist>
        struct make_options_impl {
            typedef typename make_options_impl<
                typename do_pack<
                    OptionList,
                    typename typelist_head< Typelist >::type
                >::type,
                typename typelist_tail<Typelist>::type
            >::type type ;
        };

        template <typename OptionList>
        struct make_options_impl<OptionList, typelist<> > {
            typedef OptionList type ;
        };
    }   // namespace details
    //@endcond

    /// make_options metafunction
    /** @headerfile cds/opt/options.h

        The metafunction converts option list \p Options to traits structure.
        The result of metafunction is \p type.

        Template parameter \p OptionList is default option set (default traits).
        \p Options is option list.
    */
    template <typename OptionList, typename... Options>
    struct make_options {
#ifdef CDS_DOXYGEN_INVOKED
        typedef implementation_defined type ;   ///< Result of the metafunction
#else
        typedef typename details::make_options_impl< OptionList, details::typelist<Options...> >::type type  ;
#endif
    };


    // *****************************************************************
    // find_type_traits metafunction
    // *****************************************************************

    //@cond
    namespace details {
        template <typename... Options>
        struct find_type_traits_option  ;

        template <>
        struct find_type_traits_option<> {
            typedef cds::opt::none  type ;
        };

        template <typename Any>
        struct find_type_traits_option< Any > {
            typedef cds::opt::none type ;
        };

        template <typename Any>
        struct find_type_traits_option< cds::opt::type_traits< Any > > {
            typedef Any type ;
        };

        template <typename Any, typename... Options>
        struct find_type_traits_option< cds::opt::type_traits< Any >, Options... > {
            typedef Any type ;
        };

        template <typename Any, typename... Options>
        struct find_type_traits_option< Any, Options... > {
            typedef typename find_type_traits_option< Options... >::type type ;
        };
    } // namespace details
    //@endcond

    /// Metafunction to find opt::type_traits option in \p Options list
    /** @headerfile cds/opt/options.h

        If \p Options contains opt::type_traits option then it is the metafunction result.
        Otherwise the result is \p DefaultOptons.
    */
    template <typename DefaultOptions, typename... Options>
    struct find_type_traits {
        typedef typename select_default< typename details::find_type_traits_option<Options...>::type, DefaultOptions>::type type ;  ///< Metafunction result
    };


    // *****************************************************************
    // find_option metafunction
    // *****************************************************************

    //@cond
    namespace details {
        template <typename What, typename... Options>
        struct find_option  ;

        struct compare_ok   ;
        struct compare_fail ;

        template <typename A, typename B>
        struct compare_option
        {
            typedef compare_fail type ;
        };

        template <template <typename> class Opt, typename A, typename B>
        struct compare_option< Opt<A>, Opt<B> >
        {
            typedef compare_ok   type    ;
        };

        // Specializations for integral type of option
#define _CDS_FIND_OPTION_INTEGRAL_SPECIALIZATION( _type ) template <template <_type> class What, _type A, _type B> \
        struct compare_option< What<A>, What<B> > { typedef compare_ok type ; };

        // For user-defined enum types
#define CDS_DECLARE_FIND_OPTION_INTEGRAL_SPECIALIZATION( _type ) namespace cds { namespace opt { namespace details { _CDS_FIND_OPTION_INTEGRAL_SPECIALIZATION(_type ) }}}

        _CDS_FIND_OPTION_INTEGRAL_SPECIALIZATION(bool)
        _CDS_FIND_OPTION_INTEGRAL_SPECIALIZATION(char)
        _CDS_FIND_OPTION_INTEGRAL_SPECIALIZATION(unsigned char)
        _CDS_FIND_OPTION_INTEGRAL_SPECIALIZATION(signed char)
        _CDS_FIND_OPTION_INTEGRAL_SPECIALIZATION(short int)
        _CDS_FIND_OPTION_INTEGRAL_SPECIALIZATION(unsigned short int)
        _CDS_FIND_OPTION_INTEGRAL_SPECIALIZATION(int)
        _CDS_FIND_OPTION_INTEGRAL_SPECIALIZATION(unsigned int)
        _CDS_FIND_OPTION_INTEGRAL_SPECIALIZATION(long)
        _CDS_FIND_OPTION_INTEGRAL_SPECIALIZATION(unsigned long)
        _CDS_FIND_OPTION_INTEGRAL_SPECIALIZATION(long long)
        _CDS_FIND_OPTION_INTEGRAL_SPECIALIZATION(unsigned long long)


        template <typename CompResult, typename Ok, typename Fail>
        struct select_option
        {
            typedef Fail    type ;
        };

        template <typename Ok, typename Fail>
        struct select_option< compare_ok, Ok, Fail >
        {
            typedef Ok      type ;
        };

        template <typename What>
        struct find_option< What > {
            typedef What    type ;
        };

        template <typename What, typename Opt>
        struct find_option< What, Opt > {
            typedef typename select_option<
                typename compare_option< What, Opt >::type
                ,Opt
                ,What
            >::type type    ;
        };

        template <typename What, typename Opt, typename... Options>
        struct find_option< What, Opt, Options... > {
            typedef typename select_option<
                typename compare_option< What, Opt >::type
                ,Opt
                ,typename find_option< What, Options... >::type
            >::type type    ;
        };
    } // namespace details
    //@endcond

    /// Metafunction to find \p What option in \p Options list
    /** @headerfile cds/opt/options.h

        If \p Options contains \p What< Val > option for any \p Val then the result is \p What< Val >
        Otherwise the result is \p What.

        Example:
        \code
        #include <cds/opt/options.h>
        namespace co = cds::opt ;

        struct default_tag;
        struct tag_a    ;
        struct tag_b    ;

        // Find option co::tag.

        // res1 is co::tag< tag_a >
        typedef co::find_option< co::tag< default_tag >, co::gc< cds::gc::HP >, co::tag< tag_a > >::type res1    ;

        // res2 is default co::tag< default_tag >
        typedef co::find_option< co::tag< default_tag >, co::less< x >, co::hash< H > >::type res2    ;

        // Multiple option co::tag. The first option is selected
        // res3 is default co::tag< tag_a >
        typedef co::find_option< co::tag< default_tag >, co::tag< tag_a >, co::tag< tag_b > >::type res3    ;

        \endcode
    */
    template <typename What, typename... Options>
    struct find_option {
        typedef typename details::find_option<What, Options...>::type   type ;  ///< Metafunction result
    };


    // *****************************************************************
    // select metafunction
    // *****************************************************************

    //@cond
    namespace details {

        template <typename What, typename... Pairs>
        struct select ;

        template <typename What, typename Value>
        struct select< What, What, Value>
        {
            typedef Value   type    ;
        };

        template <typename What, typename Tag, typename Value>
        struct select<What, Tag, Value>
        {
            typedef What    type    ;
        };

        template <typename What, typename Value, typename... Pairs>
        struct select< What, What, Value, Pairs...>
        {
            typedef Value   type    ;
        };

        template <typename What, typename Tag, typename Value, typename... Pairs>
        struct select< What, Tag, Value, Pairs...>
        {
            typedef typename select<What, Pairs...>::type   type    ;
        };
    }   // namespace details
    //@endcond

    /// Select option metafunction
    /** @headerfile cds/opt/options.h

        Pseudocode:
        \code
        select <What, T1, R1, T2, R2, ... Tn, Rn> ::=
            if What == T1 then return R1
            if What == T2 then return R2
            ...
            if What == Tn then return Rn
            else return What
        \endcode
    */
    template <typename What, typename... Pairs>
    struct select {
        typedef typename details::select< What, Pairs...>::type  type    ;   ///< Metafunction result
    };

}}  // namespace cds::opt

#endif // #ifndef __CDS_OPT_MAKE_OPTIONS_STD_H
