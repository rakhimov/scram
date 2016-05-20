set(BUILD_SHARED_LIBS FALSE)  # GUI code should never be used by the core.

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories("${PROJECT_SOURCE_DIR}")  # Include core headers via "src".

set(SCRAM_GUI_RES "${CMAKE_CURRENT_SOURCE_DIR}/res.qrc")
set(SCRAM_GUI_SRC "${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.cpp")
set(SCRAM_GUI_HDR "${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.h")
set(SCRAM_GUI_UI "${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.ui")
set(SCRAM_GUI_BIN "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

qt5_wrap_cpp(SCRAM_GUI_LIB_HDR_MOC "${SCRAM_GUI_HDR}")
qt5_wrap_ui(SCRAM_GUI_LIB_UI_MOC "${SCRAM_GUI_UI}")
qt5_add_resources(SCRAM_GUI_LIB_RES ${SCRAM_GUI_RES})

add_library(scramgui
  ${SCRAM_GUI_SRC}
  ${SCRAM_GUI_LIB_HDR_MOC}
  ${SCRAM_GUI_LIB_UI_MOC}
  ${SCRAM_GUI_LIB_RES}
  )

target_link_libraries(scramgui Qt5::Core Qt5::Gui Qt5::Widgets)

target_link_libraries(scramgui scramcore ${LIBS})

add_executable(scram-gui ${SCRAM_GUI_BIN})
target_link_libraries(scram-gui scramgui scramcore ${Boost_LIBRARIES})

install(TARGETS scram-gui
  RUNTIME DESTINATION bin
  COMPONENT gui
  )
