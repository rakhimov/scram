find_package(Qt5Test REQUIRED)

macro(TEST NAME FILE)
  add_executable(${NAME} ${FILE})
  target_link_libraries(${NAME} scram-gui Qt5::Test)
  add_test(NAME ${NAME} COMMAND  $<TARGET_FILE:${NAME}>)
endmacro()

enable_testing()

set(SCRAM_GUI_TEST_SRC
  testdummy.cpp
  testvalidator.cpp
  )

foreach(testfile ${SCRAM_GUI_TEST_SRC})
  get_filename_component(testname "${testfile}" NAME_WE)
  TEST("${testname}" "${testfile}")
endforeach()
