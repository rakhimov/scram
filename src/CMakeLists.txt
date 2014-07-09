file(GLOB cc_core "${CMAKE_CURRENT_SOURCE_DIR}/*.cc")
set(SCRAM_CORE_SRC ${SCRAM_CORE_SRC}
  "${cc_core}"
  )
add_library(scramcore ${SCRAM_CORE_SRC})

set_target_properties(scramcore
  PROPERTIES
  INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib"
  INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib"
  )

target_link_libraries(scramcore dl ${LIBS})

install(
  TARGETS scramcore
  LIBRARY DESTINATION lib
  COMPONENT cycluscore
  )

add_executable(scram
  scram.cc
  )
target_link_libraries(scram scramcore ${Boost_LIBRARIES})
