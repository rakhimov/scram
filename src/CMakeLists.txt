cmake_minimum_required (VERSION 2.8)

project( SCRAM )

set(Boost_USE_STATIC_LIBS OFF)
# set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost COMPONENTS program_options filesystem system REQUIRED)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    add_executable(scram scram.cc fault_tree.cc error.cc)
    target_link_libraries(scram ${Boost_LIBRARIES})
endif()
