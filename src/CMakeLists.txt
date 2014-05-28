add_executable(scram
    scram.cc
    fault_tree.cc
    superset.cc
    error.cc
    top_event.cc
    inter_event.cc
    primary_event.cc)
target_link_libraries(scram ${Boost_LIBRARIES})
