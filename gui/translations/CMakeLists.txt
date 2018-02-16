find_package(Qt5LinguistTools REQUIRED)
message(STATUS "Found Qt5 Linguist")

# The translation is setup with Crowdin.
# The English locale is a reference for all the other locales.
# Use 'lupdate' directly to update the English locale only.
# The other locales will be updated by Crowdin automatically.
#
# To lupdate the reference - en - translation file,
# the GUI source files require preprocessing
# to replace gettext-like `_()` w/ `QObject::tr()`.
#
#   $ sed -i -r 's/\b_\b\(/QObject::tr\(/g' ./*.{cpp,h}
#   $ lupdate . -ts translations/scramgui_en.ts
#
# This could have been automated w/ CMake;
# however, it does not play well with CMake out-of-source builds.

# The En locale is default reference,
# so TS only includes non-En locales.
# Ideally, 90% minimum translation coverage is desired for inclusion.
qt5_add_translation(SCRAM_GUI_QM
  scramgui_ru_RU.ts
  scramgui_de_DE.ts
  scramgui_es_ES.ts
  scramgui_tr_TR.ts
  scramgui_nl_NL.ts
  scramgui_id_ID.ts
  scramgui_pl_PL.ts
  scramgui_it_IT.ts
  )
message(STATUS "GUI translations: ${SCRAM_GUI_QM}")
add_custom_target(translations ALL DEPENDS ${SCRAM_GUI_QM})
install(FILES ${SCRAM_GUI_QM} DESTINATION share/scram/translations COMPONENT gui)
