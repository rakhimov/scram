# vim: set foldmarker={{{,}}} foldlevel=0 foldmethod=marker:
######################## Begin find Qt ###################### {{{
# Find Qt5 libraries.
if(APPLE)
  # Qt5 with Homebrew.
  list(APPEND CMAKE_PREFIX_PATH /usr/local/opt/qt5)
endif()
find_package(Qt5 "5.9.1" REQUIRED COMPONENTS
  Core Widgets Svg OpenGL PrintSupport Concurrent
  )
message(STATUS "Found Qt5")
message(STATUS "Qt5 Version: ${Qt5_VERSION}")
######################## End find Qt ###################### }}}

##################### Begin cmake configuration ################### {{{
set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories("${CMAKE_BINARY_DIR}")  # Access the generated core version header.

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
##################### End cmake configuration ################### }}}

##################### Begin resource configuration ################### {{{
set(SCRAM_GUI_RES res.qrc)

if(WIN32 OR APPLE)
  set(TANGO_QRC qtango/icons/tango/tango.qrc)
  execute_process(
    COMMAND python generate_rcc.py icons/tango/index.theme
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/qtango
    RESULT_VARIABLE qtango_return
    OUTPUT_VARIABLE qtango_out
    ERROR_VARIABLE qtango_out
    )
  if(qtango_return)
    message(FATAL_ERROR "Tango icon generation failed: ${qtango_out}")
  endif()
  list(APPEND SCRAM_GUI_RES "${TANGO_QRC}")
endif()

if(UNIX AND NOT APPLE)
  foreach(size IN ITEMS 32 64 128)
    install(
      FILES       images/scram${size}x${size}.png
      DESTINATION share/icons/hicolor/${size}x${size}/apps
      COMPONENT   gui
      RENAME      scram.png)
  endforeach()

  install(
    FILES       images/scram_motif.svg
    DESTINATION share/icons/hicolor/scalable/apps
    COMPONENT   gui
    RENAME      scram.svg)

  # Install a desktop file
  # so that SCRAM appears in the application start menu with an icon.
  install(
    FILES       scram-gui.desktop
    DESTINATION share/applications
    COMPONENT   gui)
  # Install appdata for software center.
  install(
    FILES       scram-gui.appdata.xml
    DESTINATION share/metainfo
    COMPONENT   gui
    )
endif()

add_subdirectory(translations)
##################### End resource configuration ################### }}}

####################### Begin SCRAM GUI lib config ##################### {{{
### Begin SCRAM GUI source list ### {{{
set(SCRAM_GUI_SRC
  language.cpp
  validator.cpp
  model.cpp
  modeltree.cpp
  reporttree.cpp
  elementcontainermodel.cpp
  importancetablemodel.cpp
  producttablemodel.cpp
  preferencesdialog.cpp
  settingsdialog.cpp
  eventdialog.cpp
  printable.cpp
  zoomableview.cpp
  diagram.cpp
  mainwindow.cpp
  )
### End SCRAM GUI source list ### }}}
add_library(scram-gui STATIC ${SCRAM_GUI_SRC})
target_link_libraries(scram-gui
  ${LIBS} scram
  Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Svg Qt5::OpenGL Qt5::PrintSupport Qt5::Concurrent)
target_compile_options(scram-gui PUBLIC $<$<CONFIG:DEBUG>:${SCRAM_CXX_FLAGS_DEBUG} -Wno-useless-cast>) # TODO: MOC failure.
# Prevent implicit QString(const char*), string concat with "+", and other anti-patterns.
target_compile_definitions(scram-gui PRIVATE
  $<$<CONFIG:DEBUG>:QT_NO_CAST_FROM_ASCII QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_BYTEARRAY QT_NO_URL_CAST_FROM_STRING QT_USE_QSTRINGBUILDER>
  )
####################### End SCRAM GUI lib config ##################### }}}

####################### Begin SCRAM GUI bin config ##################### {{{
add_executable(scram-gui-bin WIN32 main.cpp scram.rc ${SCRAM_GUI_RES})
set_target_properties(scram-gui-bin PROPERTIES OUTPUT_NAME scram-gui)
target_link_libraries(scram-gui-bin scram-gui scram ${Boost_LIBRARIES})
install(
  TARGETS scram-gui-bin
  RUNTIME DESTINATION bin
  COMPONENT gui
  )
####################### End SCRAM GUI bin config ##################### }}}

###################### Begin CPACK configurations ########################### {{{
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
        install(
          FILES "${_qt_plugin_path}"
          DESTINATION "${_qt_plugin_dest}"
          COMPONENT gui)
        set(${_qt_plugins_var}
          "${${_qt_plugins_var}};\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${_qt_plugin_dest}/${_qt_plugin_file}")
      else()
        message(FATAL_ERROR "QT plugin ${_qt_plugin_name} not found")
      endif()
    endmacro()
    install_qt5_plugin("Qt5::QWindowsIntegrationPlugin" QT_PLUGINS)

    execute_process(COMMAND qmake -query QT_INSTALL_TRANSLATIONS
      OUTPUT_VARIABLE QT_TRANSLATIONS_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
    install(
      DIRECTORY "${QT_TRANSLATIONS_DIR}/"
      DESTINATION translations
      COMPONENT gui
      FILES_MATCHING PATTERN "qtbase_*" PATTERN "qt_*" PATTERN "qt_help_*" EXCLUDE
      )

    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/qt.conf"
      "[Paths]\nPlugins = ../${_qt_plugin_dir}\nTranslations = ../translations\n")
    install(
      FILES "${CMAKE_CURRENT_BINARY_DIR}/qt.conf"
      DESTINATION bin
      COMPONENT gui)
  endif()
endif()
###################### End CPACK configurations ############################# }}}

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
