################### File Configurations #######################
EXECUTE_PROCESS(COMMAND git describe --tags OUTPUT_VARIABLE core_version
  OUTPUT_STRIP_TRAILING_WHITESPACE)
CONFIGURE_FILE(version.cc.in "${CMAKE_CURRENT_SOURCE_DIR}/version.cc" @ONLY)

SET(scram_install_dir "${CMAKE_INSTALL_PREFIX}")
CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/env.cc.in"
  "${CMAKE_CURRENT_SOURCE_DIR}/env.cc"
  @ONLY
  )
################### End File Configurations ####################

set(SCRAM_CORE_SRC ${SCRAM_CORE_SRC}
  "${CMAKE_CURRENT_SOURCE_DIR}/env.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/version.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/error.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/settings.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/config.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/element.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/expression.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/event.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/ccf_group.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/fault_tree.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/indexed_gate.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/indexed_fault_tree.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/random.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/fault_tree_analysis.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/probability_analysis.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/uncertainty_analysis.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/xml_parser.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/relax_ng_validator.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/grapher.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/reporter.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/risk_analysis.cc"
  )
add_library(scramcore ${SCRAM_CORE_SRC})

set_target_properties(scramcore
  PROPERTIES
  INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib"
  INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib"
  )

target_link_libraries(scramcore dl ${LIBS})

if(NOT BUILD_SHARED_LIBS)
  install(
    TARGETS scramcore
    DESTINATION lib
    COMPONENT scramcore
    )
else()
  install(
    TARGETS scramcore
    LIBRARY DESTINATION lib
    COMPONENT scramcore
    )
endif()

add_executable(scram
  scram.cc
  )
target_link_libraries(scram scramcore ${Boost_LIBRARIES})

install(TARGETS scram
  RUNTIME DESTINATION bin
  COMPONENT scram
  )
