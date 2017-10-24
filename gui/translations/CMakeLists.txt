set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM 1)  # prevent 'make clean'

find_package(Qt5LinguistTools REQUIRED)
message(STATUS "Found Qt5 Linguist")

set(SCRAM_GUI_TS_ORIGIN "${CMAKE_CURRENT_SOURCE_DIR}/scramgui_en.ts")
set(SCRAM_GUI_TS
  "${CMAKE_CURRENT_SOURCE_DIR}/scramgui_ru_RU.ts"
  "${CMAKE_CURRENT_SOURCE_DIR}/scramgui_de_DE.ts"
  )

qt5_add_translation(SCRAM_GUI_QM ${SCRAM_GUI_TS})
message(STATUS "GUI translations: ${SCRAM_GUI_QM}")
add_custom_target(translations ALL DEPENDS ${SCRAM_GUI_QM})
install(FILES ${SCRAM_GUI_QM} DESTINATION share/scram/translations COMPONENT gui)

qt5_create_translation(TRANSLATION_MESSAGES "${SCRAM_GUI_DIR}" ${SCRAM_GUI_TS_ORIGIN})
add_custom_target(translations_update DEPENDS ${TRANSLATION_MESSAGES})
