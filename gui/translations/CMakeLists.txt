set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM 1)  # prevent 'make clean'

find_package(Qt5LinguistTools REQUIRED)
message(STATUS "Found Qt5 Linguist")

set(SCRAM_GUI_TS_ORIGIN scramgui_en.ts)
set(SCRAM_GUI_TS
  scramgui_ru_RU.ts
  scramgui_de_DE.ts
  )

qt5_add_translation(SCRAM_GUI_QM ${SCRAM_GUI_TS})
message(STATUS "GUI translations: ${SCRAM_GUI_QM}")
add_custom_target(translations ALL DEPENDS ${SCRAM_GUI_QM})
install(FILES ${SCRAM_GUI_QM} DESTINATION share/scram/translations COMPONENT gui)

qt5_create_translation(TRANSLATION_MESSAGES .. ${SCRAM_GUI_TS_ORIGIN})
add_custom_target(translations_update DEPENDS ${TRANSLATION_MESSAGES})
