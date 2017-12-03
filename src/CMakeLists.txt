################### File Configurations #######################
execute_process(COMMAND git describe --tags OUTPUT_VARIABLE core_version
  OUTPUT_STRIP_TRAILING_WHITESPACE)
configure_file(version.cc.in version.cc @ONLY)
configure_file(env.cc.in env.cc @ONLY)
################### End File Configurations ####################

if(WITH_COVERAGE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
endif()

if(WITH_PROFILE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SCRAM_CORE_SRC
  ${CMAKE_CURRENT_BINARY_DIR}/env.cc
  ${CMAKE_CURRENT_BINARY_DIR}/version.cc
  logger.cc
  random.cc
  settings.cc
  xml.cc
  config.cc
  element.cc
  expression.cc
  parameter.cc
  expression/conditional.cc
  expression/constant.cc
  expression/numerical.cc
  expression/exponential.cc
  expression/random_deviate.cc
  expression/test_event.cc
  expression/extern.cc
  event.cc
  substitution.cc
  ccf_group.cc
  fault_tree.cc
  event_tree.cc
  alignment.cc
  model.cc
  pdag.cc
  preprocessor.cc
  mocus.cc
  bdd.cc
  zbdd.cc
  analysis.cc
  fault_tree_analysis.cc
  probability_analysis.cc
  importance_analysis.cc
  uncertainty_analysis.cc
  event_tree_analysis.cc
  reporter.cc
  serialization.cc
  initializer.cc
  risk_analysis.cc
  )
add_library(scramcore ${SCRAM_CORE_SRC})

if(INSTALL_LIBS)
  if(WIN32)
    install(TARGETS scramcore RUNTIME DESTINATION bin COMPONENT scram)
  else()
    set_target_properties(scramcore
      PROPERTIES
      INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib/scram"
      INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib/scram"
      )
    install(
      TARGETS scramcore
      DESTINATION lib/scram
      COMPONENT scram
      )
  endif()
endif()

target_link_libraries(scramcore ${LIBS})

add_executable(scram scram.cc)
target_link_libraries(scram scramcore ${Boost_LIBRARIES})

install(
  TARGETS scram
  RUNTIME DESTINATION bin
  COMPONENT scram
  )
