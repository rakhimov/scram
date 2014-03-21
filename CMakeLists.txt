# This project name is scram.
PROJECT( SCRAM )
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

##############################################################################################
################################# begin cmake configuration ##################################
##############################################################################################

# set project directories
SET(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
SET(SCRAM_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src")
SET(SCRAM_TEST_DIR "${PROJECT_SOURCE_DIR}/tests")

# This makes all the libraries build as SHARED
SET(BUILD_SHARED_LIBS true)

# Setup build locations.
IF(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${SCRAM_BINARY_DIR}/bin")
endif()
IF(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${SCRAM_BINARY_DIR}/lib")
endif()
IF(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
  SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${SCRAM_BINARY_DIR}/lib")
ENDIF()

SET(SCRAM_EXECUTABLE_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

# use, i.e. don't skip the full RPATH for the build tree
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)

# when building, don't use the install RPATH already
# (but later on when installing)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# the RPATH to be used when installing, but only if it's not a system directory
LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
IF("${isSystemDir}" STREQUAL "-1")
   SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
ENDIF("${isSystemDir}" STREQUAL "-1")

# set configuration for all library builds
IF (WIN32)
  set(suffix ".dll")
  set(dynamicloadlib "windows_helper_functions.h")
ELSEIF(APPLE)
  set(CMAKE_SHARED_LINKER_FLAGS "-undefined dynamic_lookup")
  set(suffix ".dylib")
  set(dynamicloadlib "unix_helper_functions.h")
ELSE()
  set(suffix ".so")
  set(dynamicloadlib "unix_helper_functions.h")
ENDIF()

# enable testing, must be at top-level cmake file
OPTION( USE_TESTING "Build testing" ON )
IF( USE_TESTING )
  enable_testing()
  INCLUDE(CTest)
ENDIF()

##############################################################################################
################################## end cmake configuration ###################################
##############################################################################################

##############################################################################################
################################### begin find libraries #####################################
##############################################################################################

# Include the boost header files and the program_options library
# Please be sure to use Boost rather than BOOST.
# Capitalization matters on some platforms
SET(Boost_USE_STATIC_LIBS       OFF)
SET(Boost_USE_STATIC_RUNTIME    OFF)
# SET(Boost_USE_MULTITHREADED    OFF)
FIND_PACKAGE( Boost COMPONENTS program_options filesystem system REQUIRED)
MESSAGE("--    Boost Root: ${Boost_ROOT}")
MESSAGE("--    Boost Include directory: ${Boost_INCLUDE_DIR}")
MESSAGE("--    Boost Library directories: ${Boost_LIBRARY_DIRS}")
SET(LIBS ${LIBS} ${Boost_PROGRAM_OPTIONS_LIBRARY})
MESSAGE("--    Boost Program Options location: ${Boost_PROGRAM_OPTIONS_LIBRARY}")
SET(LIBS ${LIBS} ${Boost_SYSTEM_LIBRARY})
MESSAGE("--    Boost System location: ${Boost_SYSTEM_LIBRARY}")
SET(LIBS ${LIBS} ${Boost_FILESYSTEM_LIBRARY})
MESSAGE("--    Boost Filesystem location: ${Boost_FILESYSTEM_LIBRARY}")

##############################################################################################
#################################### end find libraries ######################################
##############################################################################################

##############################################################################################
###################################### begin includes ########################################
##############################################################################################

# include all the directories we just found
INCLUDE_DIRECTORIES("${Boost_INCLUDE_DIR}")

ADD_SUBDIRECTORY("${SCRAM_SOURCE_DIR}")
ADD_SUBDIRECTORY("${SCRAM_TEST_DIR}")

##############################################################################################
####################################### end includes #########################################
##############################################################################################

##############################################################################################
################################## begin uninstall target ####################################
##############################################################################################

# configure_file(
#     "${PROJECT_SOURCE_DIR}/CMake/cmake_uninstall.cmake.in"
#     "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
#     IMMEDIATE @ONLY
#     )
#
# add_custom_target(uninstall
#     COMMAND "${CMAKE_COMMAND}" -P "\"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake\"")

##############################################################################################
################################### end uninstall target #####################################
##############################################################################################

##############################################################################################
####################################### begin cpack ##########################################
##############################################################################################

SET(CPACK_COMPONENTS_ALL scram testing libraries data core)

# Here's where we package it with CPack
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "SCRAM : Simplified Command-line Risk Analysis Machine.")

SET(CPACK_GENERATOR "DEB")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Olzhas Rakhimov") #required
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/README.rst")
SET(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
SET(CPACK_PACKAGE_VERSION_MAJOR "0")
SET(CPACK_PACKAGE_VERSION_MINOR "2")
#SET(CPACK_PACKAGE_VERSION_PATCH "2") # should use commit number here  ?
SET(CPACK_PACKAGE_NAME "${CMAKE_PROJECT_NAME}")
SET(CPACK_INCLUDE_TOPLEVEL_DIRECTORY "1")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "scram${CPACK_VERSION_MAJOR}.${CPACK_VERSION_MINOR}")
#SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libboost-all-dev  (>= 1.46.0-1), libxml++2.6-dev (>= 2.6.0), libsqlite3-dev (>= 3.7.9), libhdf5-serial-dev (>= 1.8.0)")
set (CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

INCLUDE(CPack)

##############################################################################################
######################################## end cpack ###########################################
##############################################################################################
