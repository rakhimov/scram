#################################################################
######### build libscram #######################################
#################################################################

INCLUDE_DIRECTORIES(
  "${CMAKE_CURRENT_SOURCE_DIR}"
  )

SET(
  SCRAM_CORE_INCLUDE_DIRS ${SCRAM_CORE_INCLUDE_DIRS}
  "${CMAKE_CURRENT_SOURCE_DIR}"
  PARENT_SCOPE
  )

SET(SCRAM_CORE_SRC ${SCRAM_CORE_SRC} 
  )

INSTALL(FILES
  )

ADD_LIBRARY( scramcore ${SCRAM_CORE_SRC} )

SET_TARGET_PROPERTIES( scramcore 
  PROPERTIES 
  INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib"
  INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib"
  )

TARGET_LINK_LIBRARIES( scramcore dl ${LIBS} )

INSTALL(
  TARGETS scramcore
  LIBRARY DESTINATION lib
  COMPONENT scramcore
  )

#################################################################
######### end build libscram ###################################
#################################################################
