# vim: set foldmarker={{{,}}} foldlevel=0 foldmethod=marker:
################### File Configurations ####################### {{{
execute_process(COMMAND git describe --tags OUTPUT_VARIABLE SCRAM_GIT_REVISION
  OUTPUT_STRIP_TRAILING_WHITESPACE)
configure_file(version.h.in version.h @ONLY)
################### End File Configurations ################### }}}

set(CMAKE_INCLUDE_CURRENT_DIR ON)

####################### Begin SCRAM core lib config ##################### {{{
### Begin SCRAM core source list ### {{{
set(SCRAM_CORE_SRC
  ext/version.cc
  env.cc
  logger.cc
  settings.cc
  xml.cc
  project.cc
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
  cycle.cc
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
### End SCRAM core source list ### }}}
add_library(scram SHARED ${SCRAM_CORE_SRC})
target_link_libraries(scram ${LIBS})
target_compile_options(scram PRIVATE $<$<CONFIG:DEBUG>:${SCRAM_CXX_FLAGS_DEBUG}>)

install(
  TARGETS scram
  LIBRARY DESTINATION lib/scram
  ARCHIVE DESTINATION lib/scram/static
  RUNTIME DESTINATION bin
  COMPONENT scram
  )
####################### End SCRAM core lib config ##################### }}}

####################### Begin SCRAM CLI config ##################### {{{
add_executable(scram-cli scram.cc)
set_target_properties(scram-cli PROPERTIES OUTPUT_NAME scram)
target_link_libraries(scram-cli scram ${Boost_LIBRARIES})
target_compile_options(scram-cli PRIVATE $<$<CONFIG:DEBUG>:${SCRAM_CXX_FLAGS_DEBUG}>)

install(
  TARGETS scram-cli
  RUNTIME DESTINATION bin
  COMPONENT scram
  )
####################### End SCRAM CLI config ##################### }}}
