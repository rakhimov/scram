cmake_minimum_required(VERSION 2.8.12)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SCRAM_GUI_SRC "${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.cpp")
set(SCRAM_GUI_HDR "${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.h")
set(SCRAM_GUI_UI "${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.ui")
set(SCRAM_GUI_BIN "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

qt5_wrap_cpp(SCRAM_GUI_LIB_HDR_MOC "${SCRAM_GUI_HDR}")
qt5_wrap_ui(SCRAM_GUI_LIB_UI_MOC "${SCRAM_GUI_UI}")

add_library(scramgui
  ${SCRAM_GUI_SRC}
  ${SCRAM_GUI_LIB_HDR_MOC}
  ${SCRAM_GUI_LIB_UI_MOC}
  )

qt5_use_modules(scramgui Core Gui Widgets)

set_target_properties(scramgui
  PROPERTIES
  INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib"
  INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib"
  )

install(
  TARGETS scramgui
  LIBRARY DESTINATION lib
  COMPONENT scram-gui
  )

add_executable(scram-gui ${SCRAM_GUI_BIN})
target_link_libraries(scram-gui scramgui)

install(TARGETS scram-gui
  RUNTIME DESTINATION bin
  COMPONENT scram-gui
  )
