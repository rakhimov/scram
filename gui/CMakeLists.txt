set(BUILD_SHARED_LIBS FALSE)  # GUI code should never be used by the core.

# Prevent implicit QString(const char*), string concat with "+", and other anti-patterns.
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII -DQT_NO_CAST_FROM_BYTEARRAY -DQT_NO_URL_CAST_FROM_STRING -DQT_USE_QSTRINGBUILDER")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  # TODO: Qt moc offsetof applied to non-POD types is nonstandard.
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -diag-disable=1875")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(SCRAM_GUI_RES "${CMAKE_CURRENT_SOURCE_DIR}/res.qrc")
set(SCRAM_GUI_SRC
  "${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/event.cpp"
  )
set(SCRAM_GUI_BIN "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
set(SCRAM_GUI_TS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/translations")

add_subdirectory("${SCRAM_GUI_TS_DIR}")

add_library(scramgui ${SCRAM_GUI_SRC} ${SCRAM_GUI_RES})

target_link_libraries(scramgui Qt5::Core Qt5::Gui Qt5::Widgets)

target_link_libraries(scramgui scramcore ${LIBS})

add_executable(scram-gui ${SCRAM_GUI_BIN})
target_link_libraries(scram-gui scramgui scramcore ${Boost_LIBRARIES})

install(TARGETS scram-gui
  RUNTIME DESTINATION bin
  COMPONENT gui
  )

set(SCRAM_GUI_TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")

if(BUILD_TESTS)
  add_subdirectory("${SCRAM_GUI_TEST_DIR}")
endif()
