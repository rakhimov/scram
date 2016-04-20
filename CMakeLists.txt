cmake_minimum_required(VERSION 2.8)

# In-source build prevention.
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

####################### Begin compiler configurations ###################

# Default to C++11.
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
if(NOT COMPILER_SUPPORTS_CXX11)
  message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support.")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(CMAKE_CXX_FLAGS_DEBUG
  "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -Werror -Wpedantic -Wno-sign-compare -Wnon-virtual-dtor -Wno-missing-field-initializers")

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_FLAGS_DEBUG} -Wno-deprecated-register -Wno-missing-braces -Wshadow -Wunused-exception-parameter")
endif()

if(WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-deprecated-declarations")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wno-deprecated-declarations")
endif()

######################## End compiler configurations ####################

##################### Begin cmake configuration ###################

# Set project directories.
set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(SCRAM_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src")
set(SCRAM_GUI_DIR "${PROJECT_SOURCE_DIR}/gui")
set(SCRAM_SHARE_DIR "${PROJECT_SOURCE_DIR}/share")
set(SCRAM_TEST_DIR "${PROJECT_SOURCE_DIR}/tests")
set(SCRAM_INPUT_DIR "${PROJECT_SOURCE_DIR}/input")

set(BUILD_SHARED_LIBS TRUE)  # Change this for static building.

# Linking of external libraries, such as BOOST.
set(USE_STATIC_LIBS OFF)  # Change this for static building

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

# Use, i.e. don't skip the full RPATH for the build tree.
set(CMAKE_SKIP_BUILD_RPATH FALSE)

# When building,
# don't use the install RPATH already
# (but later on when installing).
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# Add the automatically determined parts of the RPATH,
# which point to directories outside the build tree
# to the install RPATH.
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Tell CMake where the modules are.
if(CMAKE_DIR)
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_DIR}/share/cmake-2.8/Modules")
endif()
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Set configuration for all library builds.
if(WIN32)
  set(suffix ".dll")
elseif(APPLE)
  set(CMAKE_SHARED_LINKER_FLAGS "-undefined dynamic_lookup")
  set(suffix ".dylib")
else()
  set(suffix ".so")
endif()

######################## End cmake configuration ###################

######################## Begin find libraries ######################

set(LIBS "")

# Optional TCMalloc for optimization.
if(NOT NTCMALLOC)
  find_package(Tcmalloc)
  set(LIBS ${LIBS} ${Tcmalloc_LIBRARIES})
endif()

# Debian installs useful LibXML2 files to /usr/include/libxml2/libxml.
# libxml2 is required for relaxng schema validation.
find_package(LibXml2 REQUIRED)
add_definitions(${LIBXML2_DEFINITIONS})
set(LIBS ${LIBS} ${LIBXML2_LIBRARIES})

# Find LibXML++ and dependencies.
find_package(LibXML++ REQUIRED)
set(LIBS ${LIBS} ${LibXML++_LIBRARIES})
message(STATUS ${LIBS})

# Include the boost header files and the program_options library.
# Please be sure to use Boost rather than BOOST.
# Capitalization matters on some platforms.
set(BOOST_MIN_VERSION "1.54.0")
set(Boost_USE_STATIC_LIBS ${USE_STATIC_LIBS})
set(Boost_USE_STATIC_RUNTIME ${USE_STATIC_LIBS})

if(NOT WIN32)
  set(Boost_USE_MULTITHREADED OFF)
endif()

unset(Boost_INCLUDE_DIR CACHE)
unset(Boost_LIBRARY_DIRS CACHE)
find_package(Boost ${BOOST_MIN_VERSION} COMPONENTS
  program_options filesystem system
  REQUIRED)

if(USE_STATIC_LIBS)
  message(STATUS "Linking against boost static libraries")
else()
  message(STATUS "Linking against boost dynamic libraries")
endif()

if(Boost_ROOT)
  message(STATUS "    Boost Root: ${Boost_ROOT}")
endif()
message(STATUS "    Boost Include directory: ${Boost_INCLUDE_DIR}")
message(STATUS "    Boost Library directories: ${Boost_LIBRARY_DIRS}")

set(LIBS ${LIBS} ${Boost_PROGRAM_OPTIONS_LIBRARY})
message(STATUS "    Boost Program Options location: ${Boost_PROGRAM_OPTIONS_LIBRARY}")
set(LIBS ${LIBS} ${Boost_SYSTEM_LIBRARY})
message(STATUS "    Boost System location: ${Boost_SYSTEM_LIBRARY}")
set(LIBS ${LIBS} ${Boost_FILESYSTEM_LIBRARY})
message(STATUS "    Boost Filesystem location: ${Boost_FILESYSTEM_LIBRARY}")

# Find the QtWidgets library.
if(APPLE)
  # Qt5 with Homebrew.
  set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "/usr/local/opt/qt5")
endif()
find_package(Qt5Widgets)
if(Qt5Widgets_FOUND)
  message(STATUS "Found Qt5")
  message(STATUS "Qt5 Version: ${Qt5Widgets_VERSION_STRING}")
endif()

########################## End of find libraries ########################

########################## Begin includes ###############################

# Include all the directories we just found.
include_directories("${Boost_INCLUDE_DIR}")
include_directories("${LIBXML2_INCLUDE_DIR}")
include_directories("${LibXML++_INCLUDE_DIR}")
include_directories("${LibXML++Config_INCLUDE_DIR}")
include_directories(${Glibmm_INCLUDE_DIRS})
include_directories("${SCRAM_SOURCE_DIR}")

add_subdirectory("${SCRAM_SOURCE_DIR}")
add_subdirectory("${SCRAM_SHARE_DIR}")
add_subdirectory("${SCRAM_TEST_DIR}")
add_subdirectory("${SCRAM_INPUT_DIR}")

if(Qt5Widgets_FOUND)
  add_subdirectory("${SCRAM_GUI_DIR}")
endif()

####################### End includes ####################################

###################### Begin uninstall target ###########################

configure_file(
  "${PROJECT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY
  )

add_custom_target(uninstall
  COMMAND "${CMAKE_COMMAND}" -P "\"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake\"")

###################### End uninstall target #############################
