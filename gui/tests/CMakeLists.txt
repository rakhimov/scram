find_package(Qt5Test REQUIRED)

# There are no tr() strings in test, so no need to be strict with string construction.
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -UQT_NO_CAST_FROM_ASCII")

macro(TEST NAME FILE)
  add_executable(${NAME} ${FILE})
  target_link_libraries(${NAME} scram-gui Qt5::Test)
  add_test(NAME ${NAME} COMMAND  $<TARGET_FILE:${NAME}>)
endmacro()

enable_testing()

set(SCRAM_GUI_TEST_SRC
  testvalidator.cpp
  testmodel.cpp
  )

foreach(testfile ${SCRAM_GUI_TEST_SRC})
  get_filename_component(testname "${testfile}" NAME_WE)
  TEST("${testname}" "${testfile}")
endforeach()
