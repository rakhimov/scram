################### File Configurations #######################
execute_process(COMMAND git describe --tags OUTPUT_VARIABLE core_version
  OUTPUT_STRIP_TRAILING_WHITESPACE)
configure_file(version.cc.in version.cc @ONLY)
################### End File Configurations ####################

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SCRAM_CORE_SRC
  env.cc
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
add_library(scram SHARED ${SCRAM_CORE_SRC})
target_link_libraries(scram ${LIBS})

install(
  TARGETS scram
  LIBRARY DESTINATION lib/scram
  ARCHIVE DESTINATION lib/scram/static
  RUNTIME DESTINATION bin
  COMPONENT scram
  )

add_executable(scram-cli scram.cc)
set_target_properties(scram-cli PROPERTIES OUTPUT_NAME scram)
target_link_libraries(scram-cli scram ${Boost_LIBRARIES})

install(
  TARGETS scram-cli
  RUNTIME DESTINATION bin
  COMPONENT scram
  )
