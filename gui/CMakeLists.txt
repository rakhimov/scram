# Find the QtWidgets library.
if(APPLE)
  # Qt5 with Homebrew.
  set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "/usr/local/opt/qt5")
endif()
find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Svg REQUIRED)
find_package(Qt5OpenGL REQUIRED)
find_package(Qt5PrintSupport REQUIRED)
find_package(Qt5Concurrent REQUIRED)
message(STATUS "Found Qt5")
message(STATUS "Qt5 Version: ${Qt5Widgets_VERSION_STRING}")

# Prevent implicit QString(const char*), string concat with "+", and other anti-patterns.
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII -DQT_NO_CAST_FROM_BYTEARRAY -DQT_NO_URL_CAST_FROM_STRING -DQT_USE_QSTRINGBUILDER")

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  # TODO: MOC generated code has some useless casts.
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-useless-cast")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
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
  "${CMAKE_CURRENT_SOURCE_DIR}/validator.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/model.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/modeltree.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/elementcontainermodel.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/preferencesdialog.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/settingsdialog.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/eventdialog.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/printable.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/zoomableview.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/diagram.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
  )
set(SCRAM_GUI_TS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/translations")

add_subdirectory("${SCRAM_GUI_TS_DIR}")

if(WIN32)
  list(APPEND SCRAM_GUI_SRC "${CMAKE_CURRENT_SOURCE_DIR}/scram.rc")
endif()
add_executable(scram-gui WIN32 ${SCRAM_GUI_SRC} ${SCRAM_GUI_RES})

target_link_libraries(scram-gui
  ${LIBS} scramcore
  Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Svg Qt5::OpenGL Qt5::PrintSupport Qt5::Concurrent)

install(TARGETS scram-gui
  RUNTIME DESTINATION bin
  COMPONENT gui
  )

if(UNIX AND NOT APPLE)
  foreach (size IN ITEMS 32 128)
    install(
      FILES       "${CMAKE_CURRENT_SOURCE_DIR}/images/scram_solid${size}x${size}.png"
      DESTINATION "share/icons/hicolor/${size}x${size}/apps"
      COMPONENT gui
      RENAME      "scram.png")
  endforeach ()
  install(
    FILES       "${CMAKE_CURRENT_SOURCE_DIR}/images/scram_solid.svg"
    DESTINATION "share/icons/hicolor/scalable/apps"
    COMPONENT gui
    RENAME      "scram.svg")

  # Install a desktop file
  # so that SCRAM appears in the application start menu with an icon.
  install(FILES scram-gui.desktop
    DESTINATION "share/applications"
    COMPONENT gui)
endif()

if(PACKAGE)
  if(WIN32)
    # Helper functions to bundle the dependencies.
    macro(install_qt5_plugin _qt_plugin_name _qt_plugins_var)
      get_target_property(_qt_plugin_path "${_qt_plugin_name}" LOCATION)
      if(EXISTS "${_qt_plugin_path}")
        get_filename_component(_qt_plugin_file "${_qt_plugin_path}" NAME)
        get_filename_component(_qt_plugin_type "${_qt_plugin_path}" PATH)
        get_filename_component(_qt_plugin_type "${_qt_plugin_type}" NAME)
        if(APPLE)
          set(_qt_plugin_dir "PlugIns")
        elseif(WIN32)
          set(_qt_plugin_dir "plugins")
        endif()
        set(_qt_plugin_dest "${_qt_plugin_dir}/${_qt_plugin_type}")
        install(FILES "${_qt_plugin_path}"
          DESTINATION "${_qt_plugin_dest}"
          COMPONENT gui)
        set(${_qt_plugins_var}
          "${${_qt_plugins_var}};\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${_qt_plugin_dest}/${_qt_plugin_file}")
      else()
        message(FATAL_ERROR "QT plugin ${_qt_plugin_name} not found")
      endif()
    endmacro()
    install_qt5_plugin("Qt5::QWindowsIntegrationPlugin" QT_PLUGINS)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/qt.conf"
      "[Paths]\nPlugins = ../${_qt_plugin_dir}\n")
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/qt.conf"
      DESTINATION bin
      COMPONENT gui)
  endif()
endif()

set(SCRAM_GUI_TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")

if(BUILD_TESTS)
  add_subdirectory("${SCRAM_GUI_TEST_DIR}")
endif()
