################### File Configurations #######################
execute_process(COMMAND git describe --tags OUTPUT_VARIABLE core_version
  OUTPUT_STRIP_TRAILING_WHITESPACE)
configure_file(version.cc.in "${CMAKE_CURRENT_SOURCE_DIR}/version.cc" @ONLY)

set(scram_install_dir "${CMAKE_INSTALL_PREFIX}")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/env.cc.in"
  "${CMAKE_CURRENT_SOURCE_DIR}/env.cc"
  @ONLY
  )
################### End File Configurations ####################

set(SCRAM_CORE_SRC
  "${CMAKE_CURRENT_SOURCE_DIR}/env.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/version.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/error.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/logger.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/xml_parser.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/xml_stream.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/relax_ng_validator.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/random.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/settings.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/config.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/cycle.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/element.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/expression.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/event.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/ccf_group.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/fault_tree.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/model.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/boolean_graph.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/preprocessor.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/mocus.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/bdd.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/zbdd.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/analysis.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/fault_tree_analysis.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/probability_analysis.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/importance_analysis.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/uncertainty_analysis.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/reporter.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/initializer.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/risk_analysis.cc"
  )
add_library(scramcore ${SCRAM_CORE_SRC})

set_target_properties(scramcore
  PROPERTIES
  INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib"
  INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib"
  )

target_link_libraries(scramcore ${LIBS})

if(WIN32)
  install(
    TARGETS scramcore
    RUNTIME DESTINATION bin COMPONENT scram
    LIBRARY DESTINATION lib COMPONENT scram
    ARCHIVE DESTINATION lib COMPONENT scram
    )
else()
  install(
    TARGETS scramcore
    DESTINATION lib
    COMPONENT scram
    )
endif()

add_executable(scram scram.cc)
target_link_libraries(scram scramcore ${Boost_LIBRARIES})

install(
  TARGETS scram
  RUNTIME DESTINATION bin
  COMPONENT scram
  )
