//$$CDS-header$$

#ifndef __CDS_TESTHDR_MAP_PRINT_SKIPLIST_STAT_H
#define __CDS_TESTHDR_MAP_PRINT_SKIPLIST_STAT_H

#include "unit/print_skip_list_stat.h"

namespace misc {

    template <typename Stat>
    struct print_skiplist_stat  ;

    template <>
    struct print_skiplist_stat< cds::intrusive::skip_list::stat >
    {
        template <class Set>
        std::string operator()( Set const& s, const char * pszHdr )
        {
            std::stringstream st    ;
            st << "\t\t" << pszHdr << "\n"
                << s.statistics()
                ;
            return st.str() ;
        }
    };

    template<>
    struct print_skiplist_stat< cds::intrusive::skip_list::empty_stat >
    {
        template <class Set>
        std::string operator()( Set const& /*s*/, const char * /*pszHdr*/ )
        {
            return std::string() ;
        }
    };

}   // namespace misc

#endif // #ifndef __CDS_TESTHDR_MAP_PRINT_SKIPLIST_STAT_H
