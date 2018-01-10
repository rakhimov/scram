find_package(Qt5Test REQUIRED)

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

set(SCRAM_GUI_TEST_SRC
  testvalidator.cpp
  testlanguage.cpp
  testmodel.cpp
  )

foreach(testfile ${SCRAM_GUI_TEST_SRC})
  get_filename_component(testname "${testfile}" NAME_WE)
  TEST("${testname}" "${testfile}")
endforeach()
