CDS_TESTHDR_MAP := \
    tests/test-hdr/map/hdr_michael_map_hp.cpp \
    tests/test-hdr/map/hdr_michael_map_hrc.cpp \
    tests/test-hdr/map/hdr_michael_map_ptb.cpp \
    tests/test-hdr/map/hdr_michael_map_rcu_gpi.cpp \
    tests/test-hdr/map/hdr_michael_map_rcu_gpb.cpp \
    tests/test-hdr/map/hdr_michael_map_rcu_gpt.cpp \
    tests/test-hdr/map/hdr_michael_map_rcu_shb.cpp \
    tests/test-hdr/map/hdr_michael_map_rcu_sht.cpp \
    tests/test-hdr/map/hdr_michael_map_nogc.cpp \
    tests/test-hdr/map/hdr_michael_map_lazy_hp.cpp \
    tests/test-hdr/map/hdr_michael_map_lazy_hrc.cpp \
    tests/test-hdr/map/hdr_michael_map_lazy_ptb.cpp \
    tests/test-hdr/map/hdr_michael_map_lazy_rcu_gpi.cpp \
    tests/test-hdr/map/hdr_michael_map_lazy_rcu_gpb.cpp \
    tests/test-hdr/map/hdr_michael_map_lazy_rcu_gpt.cpp \
    tests/test-hdr/map/hdr_michael_map_lazy_rcu_shb.cpp \
    tests/test-hdr/map/hdr_michael_map_lazy_rcu_sht.cpp \
    tests/test-hdr/map/hdr_michael_map_lazy_nogc.cpp \
    tests/test-hdr/map/hdr_refinable_hashmap_hashmap_std.cpp \
    tests/test-hdr/map/hdr_refinable_hashmap_boost_list.cpp \
    tests/test-hdr/map/hdr_refinable_hashmap_list.cpp \
    tests/test-hdr/map/hdr_refinable_hashmap_map.cpp \
    tests/test-hdr/map/hdr_refinable_hashmap_boost_map.cpp \
    tests/test-hdr/map/hdr_refinable_hashmap_boost_flat_map.cpp \
    tests/test-hdr/map/hdr_refinable_hashmap_boost_unordered_map.cpp \
    tests/test-hdr/map/hdr_refinable_hashmap_slist.cpp \
    tests/test-hdr/map/hdr_skiplist_map_hp.cpp \
    tests/test-hdr/map/hdr_skiplist_map_hrc.cpp \
    tests/test-hdr/map/hdr_skiplist_map_ptb.cpp \
    tests/test-hdr/map/hdr_skiplist_map_rcu_gpi.cpp \
    tests/test-hdr/map/hdr_skiplist_map_rcu_gpb.cpp \
    tests/test-hdr/map/hdr_skiplist_map_rcu_gpt.cpp \
    tests/test-hdr/map/hdr_skiplist_map_rcu_shb.cpp \
    tests/test-hdr/map/hdr_skiplist_map_rcu_sht.cpp \
    tests/test-hdr/map/hdr_skiplist_map_nogc.cpp \
    tests/test-hdr/map/hdr_splitlist_map_hp.cpp \
    tests/test-hdr/map/hdr_splitlist_map_hrc.cpp \
    tests/test-hdr/map/hdr_splitlist_map_ptb.cpp \
    tests/test-hdr/map/hdr_splitlist_map_nogc.cpp \
    tests/test-hdr/map/hdr_splitlist_map_rcu_gpi.cpp \
    tests/test-hdr/map/hdr_splitlist_map_rcu_gpb.cpp \
    tests/test-hdr/map/hdr_splitlist_map_rcu_gpt.cpp \
    tests/test-hdr/map/hdr_splitlist_map_rcu_shb.cpp \
    tests/test-hdr/map/hdr_splitlist_map_rcu_sht.cpp \
    tests/test-hdr/map/hdr_splitlist_map_lazy_hp.cpp \
    tests/test-hdr/map/hdr_splitlist_map_lazy_hrc.cpp \
    tests/test-hdr/map/hdr_splitlist_map_lazy_ptb.cpp \
    tests/test-hdr/map/hdr_splitlist_map_lazy_nogc.cpp \
    tests/test-hdr/map/hdr_splitlist_map_lazy_rcu_gpi.cpp \
    tests/test-hdr/map/hdr_splitlist_map_lazy_rcu_gpb.cpp \
    tests/test-hdr/map/hdr_splitlist_map_lazy_rcu_gpt.cpp \
    tests/test-hdr/map/hdr_splitlist_map_lazy_rcu_sht.cpp \
    tests/test-hdr/map/hdr_splitlist_map_lazy_rcu_shb.cpp \
    tests/test-hdr/map/hdr_striped_hashmap_hashmap_std.cpp \
    tests/test-hdr/map/hdr_striped_hashmap_boost_list.cpp \
    tests/test-hdr/map/hdr_striped_hashmap_list.cpp \
    tests/test-hdr/map/hdr_striped_hashmap_map.cpp \
    tests/test-hdr/map/hdr_striped_hashmap_boost_map.cpp \
    tests/test-hdr/map/hdr_striped_hashmap_boost_flat_map.cpp \
    tests/test-hdr/map/hdr_striped_hashmap_boost_unordered_map.cpp \
    tests/test-hdr/map/hdr_striped_hashmap_slist.cpp \
    tests/test-hdr/map/hdr_striped_map_reg.cpp

CDS_TESTHDR_DEQUE := \
    tests/test-hdr/deque/hdr_michael_deque_hp.cpp \
    tests/test-hdr/deque/hdr_michael_deque_ptb.cpp

CDS_TESTHDR_ORDLIST := \
    tests/test-hdr/ordered_list/hdr_intrusive_lazy_hrc.cpp \
    tests/test-hdr/ordered_list/hdr_lazy_hp.cpp \
    tests/test-hdr/ordered_list/hdr_lazy_hrc.cpp \
    tests/test-hdr/ordered_list/hdr_lazy_nogc.cpp \
    tests/test-hdr/ordered_list/hdr_lazy_ptb.cpp \
    tests/test-hdr/ordered_list/hdr_lazy_rcu_gpi.cpp \
    tests/test-hdr/ordered_list/hdr_lazy_rcu_gpb.cpp \
    tests/test-hdr/ordered_list/hdr_lazy_rcu_gpt.cpp \
    tests/test-hdr/ordered_list/hdr_lazy_rcu_shb.cpp \
    tests/test-hdr/ordered_list/hdr_lazy_rcu_sht.cpp \
    tests/test-hdr/ordered_list/hdr_lazy_kv_hp.cpp \
    tests/test-hdr/ordered_list/hdr_lazy_kv_hrc.cpp \
    tests/test-hdr/ordered_list/hdr_lazy_kv_nogc.cpp \
    tests/test-hdr/ordered_list/hdr_lazy_kv_ptb.cpp \
    tests/test-hdr/ordered_list/hdr_lazy_kv_rcu_gpb.cpp \
    tests/test-hdr/ordered_list/hdr_lazy_kv_rcu_gpi.cpp \
    tests/test-hdr/ordered_list/hdr_lazy_kv_rcu_gpt.cpp \
    tests/test-hdr/ordered_list/hdr_lazy_kv_rcu_shb.cpp \
    tests/test-hdr/ordered_list/hdr_lazy_kv_rcu_sht.cpp \
    tests/test-hdr/ordered_list/hdr_intrusive_michael_hrc.cpp \
    tests/test-hdr/ordered_list/hdr_michael_hp.cpp \
    tests/test-hdr/ordered_list/hdr_michael_hrc.cpp \
    tests/test-hdr/ordered_list/hdr_michael_nogc.cpp \
    tests/test-hdr/ordered_list/hdr_michael_ptb.cpp \
    tests/test-hdr/ordered_list/hdr_michael_rcu_gpi.cpp \
    tests/test-hdr/ordered_list/hdr_michael_rcu_gpb.cpp \
    tests/test-hdr/ordered_list/hdr_michael_rcu_gpt.cpp \
    tests/test-hdr/ordered_list/hdr_michael_rcu_shb.cpp \
    tests/test-hdr/ordered_list/hdr_michael_rcu_sht.cpp \
    tests/test-hdr/ordered_list/hdr_michael_kv_hp.cpp \
    tests/test-hdr/ordered_list/hdr_michael_kv_hrc.cpp \
    tests/test-hdr/ordered_list/hdr_michael_kv_nogc.cpp \
    tests/test-hdr/ordered_list/hdr_michael_kv_ptb.cpp \
    tests/test-hdr/ordered_list/hdr_michael_kv_rcu_gpi.cpp \
    tests/test-hdr/ordered_list/hdr_michael_kv_rcu_gpb.cpp \
    tests/test-hdr/ordered_list/hdr_michael_kv_rcu_gpt.cpp \
    tests/test-hdr/ordered_list/hdr_michael_kv_rcu_shb.cpp \
    tests/test-hdr/ordered_list/hdr_michael_kv_rcu_sht.cpp

CDS_TESTHDR_PQUEUE := \
    tests/test-hdr/priority_queue/hdr_intrusive_mspqueue_dyn.cpp \
    tests/test-hdr/priority_queue/hdr_intrusive_mspqueue_static.cpp \
    tests/test-hdr/priority_queue/hdr_mspqueue_dyn.cpp \
    tests/test-hdr/priority_queue/hdr_mspqueue_static.cpp \
    tests/test-hdr/priority_queue/hdr_priority_queue_reg.cpp

CDS_TESTHDR_QUEUE := \
    tests/test-hdr/queue/hdr_intrusive_basketqueue_hrc.cpp \
    tests/test-hdr/queue/hdr_intrusive_moirqueue_hrc.cpp \
    tests/test-hdr/queue/hdr_intrusive_msqueue_hrc.cpp \
    tests/test-hdr/queue/hdr_intrusive_tsigas_cycle_queue.cpp \
    tests/test-hdr/queue/hdr_intrusive_vyukovmpmc_cycle_queue.cpp \
    tests/test-hdr/queue/hdr_basketqueue_hrc.cpp \
    tests/test-hdr/queue/hdr_basketqueue_hzp.cpp \
    tests/test-hdr/queue/hdr_basketqueue_ptb.cpp \
    tests/test-hdr/queue/hdr_moirqueue_hrc.cpp \
    tests/test-hdr/queue/hdr_moirqueue_hzp.cpp \
    tests/test-hdr/queue/hdr_moirqueue_ptb.cpp \
    tests/test-hdr/queue/hdr_msqueue_hrc.cpp \
    tests/test-hdr/queue/hdr_msqueue_hzp.cpp \
    tests/test-hdr/queue/hdr_msqueue_ptb.cpp \
    tests/test-hdr/queue/hdr_optimistic_hzp.cpp \
    tests/test-hdr/queue/hdr_optimistic_ptb.cpp \
    tests/test-hdr/queue/hdr_rwqueue.cpp \
    tests/test-hdr/queue/hdr_vyukov_mpmc_cyclic.cpp \
    tests/test-hdr/queue/queue_test_header.cpp 

CDS_TESTHDR_SET := \
    tests/test-hdr/set/hdr_intrusive_michael_set_hrc.cpp \
    tests/test-hdr/set/hdr_intrusive_michael_set_hrc_lazy.cpp \
    tests/test-hdr/set/hdr_intrusive_refinable_hashset_avlset.cpp \
    tests/test-hdr/set/hdr_intrusive_refinable_hashset_list.cpp \
    tests/test-hdr/set/hdr_intrusive_refinable_hashset_set.cpp \
    tests/test-hdr/set/hdr_intrusive_refinable_hashset_sgset.cpp \
    tests/test-hdr/set/hdr_intrusive_refinable_hashset_slist.cpp \
    tests/test-hdr/set/hdr_intrusive_refinable_hashset_splayset.cpp \
    tests/test-hdr/set/hdr_intrusive_refinable_hashset_treapset.cpp \
    tests/test-hdr/set/hdr_intrusive_refinable_hashset_uset.cpp \
    tests/test-hdr/set/hdr_intrusive_skiplist_hp.cpp \
    tests/test-hdr/set/hdr_intrusive_skiplist_hrc.cpp \
    tests/test-hdr/set/hdr_intrusive_skiplist_ptb.cpp \
    tests/test-hdr/set/hdr_intrusive_skiplist_rcu_gpb.cpp \
    tests/test-hdr/set/hdr_intrusive_skiplist_rcu_gpi.cpp \
    tests/test-hdr/set/hdr_intrusive_skiplist_rcu_gpt.cpp \
    tests/test-hdr/set/hdr_intrusive_skiplist_rcu_shb.cpp \
    tests/test-hdr/set/hdr_intrusive_skiplist_rcu_sht.cpp \
    tests/test-hdr/set/hdr_intrusive_skiplist_nogc.cpp \
    tests/test-hdr/set/hdr_intrusive_splitlist_set_hrc.cpp \
    tests/test-hdr/set/hdr_intrusive_splitlist_set_hrc_lazy.cpp \
    tests/test-hdr/set/hdr_intrusive_striped_hashset_avlset.cpp \
    tests/test-hdr/set/hdr_intrusive_striped_hashset_list.cpp \
    tests/test-hdr/set/hdr_intrusive_striped_hashset_set.cpp \
    tests/test-hdr/set/hdr_intrusive_striped_hashset_sgset.cpp \
    tests/test-hdr/set/hdr_intrusive_striped_hashset_slist.cpp \
    tests/test-hdr/set/hdr_intrusive_striped_hashset_splayset.cpp \
    tests/test-hdr/set/hdr_intrusive_striped_hashset_treapset.cpp \
    tests/test-hdr/set/hdr_intrusive_striped_hashset_uset.cpp \
    tests/test-hdr/set/hdr_intrusive_striped_set.cpp \
    tests/test-hdr/set/hdr_michael_set_hp.cpp \
    tests/test-hdr/set/hdr_michael_set_hrc.cpp \
    tests/test-hdr/set/hdr_michael_set_ptb.cpp \
    tests/test-hdr/set/hdr_michael_set_rcu_gpi.cpp \
    tests/test-hdr/set/hdr_michael_set_rcu_gpb.cpp \
    tests/test-hdr/set/hdr_michael_set_rcu_gpt.cpp \
    tests/test-hdr/set/hdr_michael_set_rcu_shb.cpp \
    tests/test-hdr/set/hdr_michael_set_rcu_sht.cpp \
    tests/test-hdr/set/hdr_michael_set_nogc.cpp \
    tests/test-hdr/set/hdr_michael_set_lazy_hp.cpp \
    tests/test-hdr/set/hdr_michael_set_lazy_hrc.cpp \
    tests/test-hdr/set/hdr_michael_set_lazy_ptb.cpp \
    tests/test-hdr/set/hdr_michael_set_lazy_rcu_gpi.cpp \
    tests/test-hdr/set/hdr_michael_set_lazy_rcu_gpb.cpp \
    tests/test-hdr/set/hdr_michael_set_lazy_rcu_gpt.cpp \
    tests/test-hdr/set/hdr_michael_set_lazy_rcu_shb.cpp \
    tests/test-hdr/set/hdr_michael_set_lazy_rcu_sht.cpp \
    tests/test-hdr/set/hdr_michael_set_lazy_nogc.cpp \
    tests/test-hdr/set/hdr_refinable_hashset_hashset_std.cpp \
    tests/test-hdr/set/hdr_refinable_hashset_boost_flat_set.cpp \
    tests/test-hdr/set/hdr_refinable_hashset_boost_list.cpp \
    tests/test-hdr/set/hdr_refinable_hashset_boost_set.cpp \
    tests/test-hdr/set/hdr_refinable_hashset_boost_stable_vector.cpp \
    tests/test-hdr/set/hdr_refinable_hashset_boost_unordered_set.cpp \
    tests/test-hdr/set/hdr_refinable_hashset_boost_vector.cpp \
    tests/test-hdr/set/hdr_refinable_hashset_list.cpp \
    tests/test-hdr/set/hdr_refinable_hashset_set.cpp \
    tests/test-hdr/set/hdr_refinable_hashset_slist.cpp \
    tests/test-hdr/set/hdr_refinable_hashset_vector.cpp \
    tests/test-hdr/set/hdr_skiplist_set_hp.cpp \
    tests/test-hdr/set/hdr_skiplist_set_hrc.cpp \
    tests/test-hdr/set/hdr_skiplist_set_ptb.cpp \
    tests/test-hdr/set/hdr_skiplist_set_rcu_gpi.cpp \
    tests/test-hdr/set/hdr_skiplist_set_rcu_gpb.cpp \
    tests/test-hdr/set/hdr_skiplist_set_rcu_gpt.cpp \
    tests/test-hdr/set/hdr_skiplist_set_rcu_shb.cpp \
    tests/test-hdr/set/hdr_skiplist_set_rcu_sht.cpp \
    tests/test-hdr/set/hdr_skiplist_set_nogc.cpp \
    tests/test-hdr/set/hdr_splitlist_set_hp.cpp \
    tests/test-hdr/set/hdr_splitlist_set_hrc.cpp \
    tests/test-hdr/set/hdr_splitlist_set_nogc.cpp \
    tests/test-hdr/set/hdr_splitlist_set_ptb.cpp \
    tests/test-hdr/set/hdr_splitlist_set_rcu_gpi.cpp \
    tests/test-hdr/set/hdr_splitlist_set_rcu_gpb.cpp \
    tests/test-hdr/set/hdr_splitlist_set_rcu_gpt.cpp \
    tests/test-hdr/set/hdr_splitlist_set_rcu_shb.cpp \
    tests/test-hdr/set/hdr_splitlist_set_rcu_sht.cpp \
    tests/test-hdr/set/hdr_splitlist_set_lazy_hp.cpp \
    tests/test-hdr/set/hdr_splitlist_set_lazy_hrc.cpp \
    tests/test-hdr/set/hdr_splitlist_set_lazy_nogc.cpp \
    tests/test-hdr/set/hdr_splitlist_set_lazy_ptb.cpp \
    tests/test-hdr/set/hdr_splitlist_set_lazy_rcu_gpi.cpp \
    tests/test-hdr/set/hdr_splitlist_set_lazy_rcu_gpb.cpp \
    tests/test-hdr/set/hdr_splitlist_set_lazy_rcu_gpt.cpp \
    tests/test-hdr/set/hdr_splitlist_set_lazy_rcu_shb.cpp \
    tests/test-hdr/set/hdr_splitlist_set_lazy_rcu_sht.cpp \
    tests/test-hdr/set/hdr_striped_hashset_hashset_std.cpp \
    tests/test-hdr/set/hdr_striped_hashset_boost_flat_set.cpp \
    tests/test-hdr/set/hdr_striped_hashset_boost_list.cpp \
    tests/test-hdr/set/hdr_striped_hashset_boost_set.cpp \
    tests/test-hdr/set/hdr_striped_hashset_boost_stable_vector.cpp \
    tests/test-hdr/set/hdr_striped_hashset_boost_unordered_set.cpp \
    tests/test-hdr/set/hdr_striped_hashset_boost_vector.cpp \
    tests/test-hdr/set/hdr_striped_hashset_list.cpp \
    tests/test-hdr/set/hdr_striped_hashset_set.cpp \
    tests/test-hdr/set/hdr_striped_hashset_slist.cpp \
    tests/test-hdr/set/hdr_striped_hashset_vector.cpp 

CDS_TESTHDR_STACK := \
    tests/test-hdr/stack/hdr_intrusive_treiber_stack_hrc.cpp \
    tests/test-hdr/stack/hdr_treiber_stack_hp.cpp \
    tests/test-hdr/stack/hdr_treiber_stack_hrc.cpp \
    tests/test-hdr/stack/hdr_treiber_stack_ptb.cpp

CDS_TESTHDR_TREE := \
    tests/test-hdr/tree/hdr_tree_reg.cpp \
    tests/test-hdr/tree/hdr_intrusive_ellen_bintree_hp.cpp \
    tests/test-hdr/tree/hdr_intrusive_ellen_bintree_ptb.cpp \
    tests/test-hdr/tree/hdr_intrusive_ellen_bintree_rcu_gpb.cpp \
    tests/test-hdr/tree/hdr_intrusive_ellen_bintree_rcu_gpi.cpp \
    tests/test-hdr/tree/hdr_intrusive_ellen_bintree_rcu_gpt.cpp \
    tests/test-hdr/tree/hdr_intrusive_ellen_bintree_rcu_shb.cpp \
    tests/test-hdr/tree/hdr_intrusive_ellen_bintree_rcu_sht.cpp \
    tests/test-hdr/tree/hdr_ellenbintree_map_hp.cpp \
    tests/test-hdr/tree/hdr_ellenbintree_map_ptb.cpp \
    tests/test-hdr/tree/hdr_ellenbintree_map_rcu_gpb.cpp \
    tests/test-hdr/tree/hdr_ellenbintree_map_rcu_gpi.cpp \
    tests/test-hdr/tree/hdr_ellenbintree_map_rcu_gpt.cpp \
    tests/test-hdr/tree/hdr_ellenbintree_map_rcu_shb.cpp \
    tests/test-hdr/tree/hdr_ellenbintree_map_rcu_sht.cpp \
    tests/test-hdr/tree/hdr_ellenbintree_set_hp.cpp \
    tests/test-hdr/tree/hdr_ellenbintree_set_ptb.cpp \
    tests/test-hdr/tree/hdr_ellenbintree_set_rcu_gpb.cpp \
    tests/test-hdr/tree/hdr_ellenbintree_set_rcu_gpi.cpp \
    tests/test-hdr/tree/hdr_ellenbintree_set_rcu_gpt.cpp \
    tests/test-hdr/tree/hdr_ellenbintree_set_rcu_shb.cpp \
    tests/test-hdr/tree/hdr_ellenbintree_set_rcu_sht.cpp

CDS_TESTHDR_MISC := \
    tests/test-hdr/misc/cxx11_atomic_class.cpp \
    tests/test-hdr/misc/cxx11_atomic_func.cpp \
    tests/test-hdr/misc/find_option.cpp \
    tests/test-hdr/misc/allocator_test.cpp \
    tests/test-hdr/misc/michael_allocator.cpp \
    tests/test-hdr/misc/hash_tuple.cpp \
    tests/test-hdr/misc/bitop_st.cpp \
    tests/test-hdr/misc/thread_init_fini.cpp

CDS_TESTHDR_SOURCES := \
    $(CDS_TESTHDR_MAP) \
    $(CDS_TESTHDR_DEQUE) \
    $(CDS_TESTHDR_ORDLIST) \
    $(CDS_TESTHDR_PQUEUE) \
    $(CDS_TESTHDR_QUEUE) \
    $(CDS_TESTHDR_SET) \
    $(CDS_TESTHDR_STACK) \
    $(CDS_TESTHDR_TREE) \
    $(CDS_TESTHDR_MISC)
    