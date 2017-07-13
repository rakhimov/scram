find_package(Qt5Test REQUIRED)

macro(TEST NAME FILE)
  add_executable(${NAME} ${FILE})
  target_link_libraries(${NAME} Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Test)
  add_test(NAME ${NAME} COMMAND  $<TARGET_FILE:${NAME}>)
endmacro()

enable_testing()

TEST(TestDummy testdummy.cpp)

add_custom_target(gui_tests COMMAND ${CMAKE_CTEST_COMMAND}
  DEPENDS
  TestDummy
)
