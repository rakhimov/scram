set(BUILD_SHARED_LIBS FALSE)  # GUI code should never be used by the core.

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII -DQT_NO_CAST_FROM_BYTEARRAY -DQT_NO_URL_CAST_FROM_STRING -DQT_USE_QSTRINGBUILDER")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories("${PROJECT_SOURCE_DIR}")  # Include core headers via "src".

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(SCRAM_GUI_RES "${CMAKE_CURRENT_SOURCE_DIR}/res.qrc")
set(SCRAM_GUI_SRC "${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.cpp")
set(SCRAM_GUI_BIN "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

add_library(scramgui ${SCRAM_GUI_SRC} ${SCRAM_GUI_RES})

target_link_libraries(scramgui Qt5::Core Qt5::Gui Qt5::Widgets)

target_link_libraries(scramgui scramcore ${LIBS})

add_executable(scram-gui ${SCRAM_GUI_BIN})
target_link_libraries(scram-gui scramgui scramcore ${Boost_LIBRARIES})

install(TARGETS scram-gui
  RUNTIME DESTINATION bin
  COMPONENT gui
  )
