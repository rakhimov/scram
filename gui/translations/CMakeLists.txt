set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM 1)  # prevent 'make clean'

find_package(Qt5LinguistTools)

if(Qt5LinguistTools_FOUND)
  message(STATUS "Found Qt5 Linguist")
  set(SCRAM_GUI_TS "${CMAKE_CURRENT_SOURCE_DIR}/scram_untranslated.ts")

  qt5_add_translation(SCRAM_GUI_QM ${SCRAM_GUI_TS})
  set(TRANSLATIONS ${SCRAM_GUI_QM} PARENT_SCOPE)
  add_custom_target(translations ALL DEPENDS ${TRANSLATIONS})

  install(FILES ${TRANSLATIONS} DESTINATION share/scram/translations)

  qt5_create_translation(TRANSLATION_MESSAGES "${SCRAM_GUI_DIR}" ${SCRAM_GUI_TS})
  add_custom_target(translations_update DEPENDS ${TRANSLATION_MESSAGES})
endif()
