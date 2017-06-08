# Find the QtWidgets library.
if(APPLE)
  # Qt5 with Homebrew.
  set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "/usr/local/opt/qt5")
endif()
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Svg REQUIRED)
find_package(Qt5OpenGL REQUIRED)
find_package(Qt5PrintSupport REQUIRED)
if(Qt5Widgets_FOUND)
  message(STATUS "Found Qt5")
  message(STATUS "Qt5 Version: ${Qt5Widgets_VERSION_STRING}")
endif()
if(Qt5Svg_FOUND)
  message(STATUS "Found Qt5 SVG")
endif()
if(Qt5OpenGL_FOUND)
  message(STATUS "Found Qt5 OpenGL")
endif()
if(Qt5PrintSupport_FOUND)
  message(STATUS "Found Qt5 PrintSupport")
endif()

# Prevent implicit QString(const char*), string concat with "+", and other anti-patterns.
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII -DQT_NO_CAST_FROM_BYTEARRAY -DQT_NO_URL_CAST_FROM_STRING -DQT_USE_QSTRINGBUILDER")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  # TODO: Qt moc offsetof applied to non-POD types is nonstandard.
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -diag-disable=1875")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  if(Qt5Widgets_VERSION_STRING VERSION_LESS 5.4)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-inconsistent-missing-override")
  endif()
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# TODO: Use the autogeneration for static Qt plugin linking by CMake (buggy as of Qt 5.8).
# if(WIN32)
#   set(CMAKE_AUTOSTATICPLUGINS ON)  # Special mingw64 cmake extension.
# endif()

set(SCRAM_GUI_RES "${CMAKE_CURRENT_SOURCE_DIR}/res.qrc")

if(NOT HAS_TANGO AND (WIN32 OR APPLE))
  set(TANGO_QRC "${CMAKE_CURRENT_SOURCE_DIR}/qtango/icons/tango/tango.qrc")
  execute_process(
    COMMAND python generate_rcc.py icons/tango/index.theme
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/qtango"
    RESULT_VARIABLE qtango_return
    OUTPUT_VARIABLE qtango_out
    ERROR_VARIABLE qtango_out
    )
  if(qtango_return)
    message(FATAL_ERROR "Tango icon generation failed: ${qtango_out}")
  endif()
  list(APPEND SCRAM_GUI_RES "${TANGO_QRC}")
endif()

set(SCRAM_GUI_SRC
  "${CMAKE_CURRENT_SOURCE_DIR}/zoomableview.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/event.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/scramgui.cpp"
  )
set(SCRAM_GUI_TS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/translations")

add_subdirectory("${SCRAM_GUI_TS_DIR}")

add_library(scramgui ${SCRAM_GUI_SRC} ${SCRAM_GUI_RES})
target_link_libraries(scramgui PRIVATE
  ${LIBS} scramcore
  Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Svg Qt5::OpenGL Qt5::PrintSupport)

if(INSTALL_LIBS)
  if(WIN32)
    install(TARGETS scramgui RUNTIME DESTINATION bin COMPONENT gui)
  else()
    set_target_properties(scramgui
      PROPERTIES
      INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib/scram"
      INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib/scram"
      )
    install(
      TARGETS scramgui
      DESTINATION lib/scram
      COMPONENT gui
      )
  endif()
endif()

add_executable(scram-gui WIN32 "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
target_link_libraries(scram-gui scramgui)

install(TARGETS scram-gui
  RUNTIME DESTINATION bin
  COMPONENT gui
  )

set(SCRAM_GUI_TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")

if(BUILD_TESTS AND NOT WIN32)
  add_subdirectory("${SCRAM_GUI_TEST_DIR}")
endif()
