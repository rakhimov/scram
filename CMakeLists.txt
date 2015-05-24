cmake_minimum_required(VERSION 2.8)
# in-source build prevention
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(STATUS "SCRAM requires an out-of-source build.")
  message(STATUS "Please remove these files from ${CMAKE_BINARY_DIR} first:")
  message(STATUS "CMakeCache.txt")
  message(STATUS "CMakeFiles")
  message(STATUS "Once these files are removed, create a separate directory")
  message(STATUS "and run CMake from there")
  message(FATAL_ERROR "in-source build detected")
endif()

project(SCRAM)

############################ begin cmake configuration ###################

# set project directories
set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(SCRAM_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src")
set(SCRAM_SHARE_DIR "${PROJECT_SOURCE_DIR}/share")
set(SCRAM_TEST_DIR "${PROJECT_SOURCE_DIR}/tests")

set(BUILD_SHARED_LIBS TRUE)  # Change this for static building.

# Linking of external libraries, such as BOOST.
set(USE_STATIC_LIBS OFF)  # Change this for static building

# Pre-uploading the schema for static distribution.
# The tests will need to be run in the test directory.
if(USE_STATIC_LIBS)
  add_definitions(-DEMBED_SCHEMA)
endif()

# Setup build locations.
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${SCRAM_BINARY_DIR}/bin")
endif()
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${SCRAM_BINARY_DIR}/lib")
endif()
if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${SCRAM_BINARY_DIR}/lib")
endif()

set(SCRAM_EXECUTABLE_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

# use, i.e. don't skip the full RPATH for the build tree
set(CMAKE_SKIP_BUILD_RPATH FALSE)

# when building, don't use the install RPATH already
# (but later on when installing)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Tell CMake where the modules are
list(APPEND CMAKE_MODULE_PATH
  "${CMAKE_DIR}/share/cmake-2.8/Modules" "${PROJECT_SOURCE_DIR}/cmake")

# set configuration for all library builds
if(WIN32)
  set(suffix ".dll")
elseif(APPLE)
  set(CMAKE_SHARED_LINKER_FLAGS "-undefined dynamic_lookup")
  set(suffix ".dylib")
else()
  set(suffix ".so")
endif()

# enable testing, must be at top-level cmake file
option(USE_TESTING "Build testing" ON)
if(USE_TESTING)
  enable_testing()
  include(CTest)
endif()

############################ end cmake configuration ###################

############################ begin find libraries ######################

# Debian installs useful LibXML2 files to /usr/include/libxml2/libxml
# libxml2 is required for relaxng schema validation
find_package(LibXml2 REQUIRED)
add_definitions(${LIBXML2_DEFINITIONS})
set(LIBS ${LIBS} ${LIBXML2_LIBRARIES})

# Find LibXML++ and dependencies
find_package(LibXML++ REQUIRED)
set(LIBS ${LIBS} ${LibXML++_LIBRARIES})
message(${LIBS})

# Include the boost header files and the program_options library
# Please be sure to use Boost rather than BOOST.
# Capitalization matters on some platforms
set(BOOST_MIN_VERSION "1.47.0")
set(Boost_USE_STATIC_LIBS ${USE_STATIC_LIBS})
set(Boost_USE_STATIC_RUNTIME ${USE_STATIC_LIBS})
set(Boost_USE_MULTITHREADED OFF)
unset(Boost_INCLUDE_DIR CACHE)
unset(Boost_LIBRARY_DIRS CACHE)
find_package(Boost ${BOOST_MIN_VERSION} COMPONENTS
  program_options filesystem system random
  REQUIRED)

if(USE_STATIC_LIBS)
  message(STATUS "Linking against boost static libraries")
else()
  message(STATUS "Linking against boost dynamic libraries")
endif()

message("--    Boost Root: ${Boost_ROOT}")
message("--    Boost Include directory: ${Boost_INCLUDE_DIR}")
message("--    Boost Library directories: ${Boost_LIBRARY_DIRS}")
set(LIBS ${LIBS} ${Boost_PROGRAM_OPTIONS_LIBRARY})
message("--    Boost Program Options location: ${Boost_PROGRAM_OPTIONS_LIBRARY}")
set(LIBS ${LIBS} ${Boost_SYSTEM_LIBRARY})
message("--    Boost System location: ${Boost_SYSTEM_LIBRARY}")
set(LIBS ${LIBS} ${Boost_FILESYSTEM_LIBRARY})
message("--    Boost Filesystem location: ${Boost_FILESYSTEM_LIBRARY}")

########################## end of find libraries ########################

########################## begin includes ###############################

# include all the directories we just found
include_directories("${Boost_INCLUDE_DIR}")
include_directories("${LIBXML2_INCLUDE_DIR}")
include_directories("${LibXML++_INCLUDE_DIR}")
include_directories("${LibXML++Config_INCLUDE_DIR}")
include_directories(${Glibmm_INCLUDE_DIRS})
include_directories("${SCRAM_SOURCE_DIR}")

add_subdirectory("${SCRAM_SOURCE_DIR}")
add_subdirectory("${SCRAM_SHARE_DIR}")
add_subdirectory("${SCRAM_TEST_DIR}")

####################### end includes ####################################

###################### begin uninstall target ###########################

configure_file(
  "${PROJECT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY
  )

add_custom_target(uninstall
  COMMAND "${CMAKE_COMMAND}" -P "\"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake\"")

###################### end uninstall target #############################
