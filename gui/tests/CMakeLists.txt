# vim: set foldmarker={{{,}}} foldlevel=0 foldmethod=marker:
find_package(Qt5Test REQUIRED)

### Begin SCRAM GUI test source list ### {{{
set(SCRAM_GUI_TEST_SRC
  testvalidator.cpp
  testlanguage.cpp
  testmodel.cpp
  )
### End SCRAM GUI test source list ### }}}

################# Begin Generate scramgui_test bins ########### {{{
macro(TEST NAME FILE)
  add_executable(scramgui_${NAME} ${FILE})
  target_link_libraries(scramgui_${NAME} scram-gui Qt5::Test)
  install(
    TARGETS scramgui_${NAME}
    RUNTIME DESTINATION bin
    COMPONENT testing
    )
  add_test(${NAME} scramgui_${NAME})
endmacro()

foreach(testfile ${SCRAM_GUI_TEST_SRC})
  get_filename_component(testname "${testfile}" NAME_WE)
  TEST("${testname}" "${testfile}")
endforeach()
################# End Generate scramgui_test bins ########### }}}
