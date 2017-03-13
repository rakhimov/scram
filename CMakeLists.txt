cmake_minimum_required(VERSION 2.8.12)

# In-source build prevention.
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(STATUS "SCRAM requires an out-of-source build.")
  message(STATUS "Please remove these files from ${CMAKE_BINARY_DIR} first:")
  message(STATUS "CMakeCache.txt")
  message(STATUS "CMakeFiles")
  message(STATUS "Once these files are removed, create a separate directory")
  message(STATUS "and run CMake from there.")
  message(FATAL_ERROR "in-source build detected")
endif()

project(SCRAM)
set(SCRAM_VERSION_MAJOR 0)  # Indicate major API change after 1.0.0
set(SCRAM_VERSION_MINOR 12)  # New features and improvements.
set(SCRAM_VERSION_MICRO 0)  # Bug fixes.

####################### Begin Options ###################

option(BUILD_SHARED_LIBS OFF)

option(INSTALL_LIBS "Install the generated libraries" ON)

# Linking of external libraries, such as BOOST.
option(WITH_STATIC_LIBS "Try to link against static libraries" OFF)

option(WITH_TCMALLOC "Use TCMalloc if available (#1 preference)" ON)
option(WITH_JEMALLOC "Use JEMalloc if available (#2 preference)" ON)

option(WITH_COVERAGE "Instrument for coverage analysis" OFF)
option(WITH_PROFILE "Instrument for performance profiling" OFF)

option(BUILD_GUI "Build the GUI front-end" ON)
option(BUILD_TESTS "Build the tests" ON)

option(PACKAGE "Package for distribution" OFF)

####################### End Options ###################

####################### Begin compiler configurations ###################

# Default to C++14.
if(CMAKE_VERSION VERSION_LESS 3.1)
  include(CheckCXXCompilerFlag)
  CHECK_CXX_COMPILER_FLAG("-std=c++1y" COMPILER_SUPPORTS_CXX14)
  if(NOT COMPILER_SUPPORTS_CXX14)
    message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++14 support.")
  endif()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")
else()
  set(CMAKE_CXX_STANDARD 14)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()

function(CHECK_COMPILER_VERSION MIN_VERSION)
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS MIN_VERSION)
    message(FATAL_ERROR "${CMAKE_CXX_COMPILER_ID} Compiler version too old. Required minimum version: ${MIN_VERSION}")
  endif()
endfunction()

set(CMAKE_CXX_FLAGS_DEBUG
  "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -Werror -Wno-sign-compare -Wnon-virtual-dtor -Wno-missing-field-initializers")

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  CHECK_COMPILER_VERSION("4.9")  # TODO: Wpedantic with gcc 4.9 errors with default arg lambdas.
  if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 5)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wshadow -Wpedantic")
  endif()
  if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 6)
    # TODO: Boost sign bit shift fails to compile on GCC 6.1
    add_definitions("-fpermissive")
  endif()

elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  CHECK_COMPILER_VERSION("3.6")
  set(CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_FLAGS_DEBUG} -Wno-missing-braces -Wshadow -Wunused-exception-parameter")

elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
  CHECK_COMPILER_VERSION("6.1")

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  CHECK_COMPILER_VERSION("17.0.1")
  # TODO: CMAKE_CXX_STANDARD has no effect on icc.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
  # TODO: Failure to deduce lambda return type with Boost 1.56.
  add_definitions("-DBOOST_RESULT_OF_USE_DECLTYPE=1")
  # TODO: Bug in Intel compiler with multi-initialization with auto and final-is-not-override.
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -diag-disable=3373,1125")
endif()

if(WIN32)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-error")
endif()

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
  message(STATUS "Using CCache for builds")
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)

  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR
     "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    add_definitions("-Qunused-arguments")  # TODO: CCache bug with not splitting args.
  endif()
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

if(INSTALL_LIBS)
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
endif()

# Add the automatically determined parts of the RPATH,
# which point to directories outside the build tree
# to the install RPATH.
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Tell CMake where the modules are.
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

######################## End cmake configuration ###################

######################## Begin find libraries ######################

set(MALLOC "System Malloc")
# Optional alternative Malloc for optimization.
if(NOT WITH_PROFILE)
  find_package(Tcmalloc)
  find_package(JeMalloc)
  if(WITH_TCMALLOC AND Tcmalloc_FOUND)
    set(LIBS ${LIBS} ${Tcmalloc_LIBRARIES})
    set(MALLOC "TCMalloc")
  elseif(WITH_JEMALLOC AND JEMALLOC_FOUND)
    set(LIBS ${LIBS} ${JEMALLOC_LIBRARIES})
    set(MALLOC "JEMalloc")
  endif()
endif()
message(STATUS "The memory allocator: ${MALLOC}")

# Find LibXML++ and dependencies.
set(LIBXML++_MIN_VERSION "2.38.1")  # TODO: Fix the minimum version check.
find_package(LibXML++ "${LIBXML++_MIN_VERSION}" REQUIRED)
set(LIBS ${LIBS} ${LibXML++_LIBRARIES})

message(STATUS ${LIBS})

# Include the boost header files and the program_options library.
# Please be sure to use Boost rather than BOOST.
# Capitalization matters on some platforms.
set(BOOST_MIN_VERSION "1.56.0")
set(Boost_USE_STATIC_LIBS ${WITH_STATIC_LIBS})
set(Boost_USE_STATIC_RUNTIME ${WITH_STATIC_LIBS})

if(NOT WIN32)
  set(Boost_USE_MULTITHREADED OFF)
endif()

unset(Boost_INCLUDE_DIR CACHE)
unset(Boost_LIBRARY_DIRS CACHE)
find_package(Boost ${BOOST_MIN_VERSION} COMPONENTS
  program_options filesystem system date_time random
  REQUIRED)

if(Boost_USE_STATIC_LIBS)
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
set(LIBS ${LIBS} ${Boost_DATE_TIME_LIBRARY})
message(STATUS "    Boost Date-Time location: ${Boost_DATE_TIME_LIBRARY}")

if(BUILD_GUI)
  # Find the QtWidgets library.
  if(APPLE)
    # Qt5 with Homebrew.
    set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "/usr/local/opt/qt5")
  endif()
  find_package(Qt5Widgets REQUIRED)
  if(Qt5Widgets_FOUND)
    message(STATUS "Found Qt5")
    message(STATUS "Qt5 Version: ${Qt5Widgets_VERSION_STRING}")
  endif()
endif()

########################## End of find libraries ########################

########################## Begin includes ###############################

# Include all the discovered system directories.
include_directories(SYSTEM "${Boost_INCLUDE_DIR}")
include_directories(SYSTEM "${LibXML++_INCLUDE_DIR}")
include_directories(SYSTEM "${LibXML++Config_INCLUDE_DIR}")
include_directories(SYSTEM ${Glibmm_INCLUDE_DIRS})

include_directories("${PROJECT_SOURCE_DIR}")  # Include the core headers via "src".

add_subdirectory("${SCRAM_SOURCE_DIR}")
add_subdirectory("${SCRAM_SHARE_DIR}")
add_subdirectory("${SCRAM_INPUT_DIR}")

if(BUILD_GUI)
  add_subdirectory("${SCRAM_GUI_DIR}")
endif()

if(BUILD_TESTS)
  add_subdirectory("${SCRAM_TEST_DIR}")
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

###################### Begin CPACK configurations ###########################

if(PACKAGE)
  if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    # TODO: Find a way to programmatically detect shared libraries and add to installation.
    # The following configuration assumes MinGW64 is setup specifically for this project.
    get_filename_component(MINGW_DLL_DIR "${CMAKE_CXX_COMPILER}" PATH)
    message(STATUS "MinGW bin location: ${MINGW_DLL_DIR}")
    file(GLOB all_libs "${MINGW_DLL_DIR}/lib*.dll")
    install(FILES
      ${all_libs}
      ${MINGW_DLL_DIR}/zlib1.dll
      DESTINATION bin COMPONENT libraries
    )

    # Qt Libraries
    install(FILES
      ${MINGW_DLL_DIR}/Qt5Core.dll
      ${MINGW_DLL_DIR}/Qt5Gui.dll
      ${MINGW_DLL_DIR}/Qt5Widgets.dll
      DESTINATION bin COMPONENT libraries
    )
  endif()
endif()

set(CPACK_PACKAGE_VENDOR "Olzhas Rakhimov")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Probabilistic risk analysis tool")
set(CPACK_PACKAGE_VERSION_MAJOR "${SCRAM_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${SCRAM_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${SCRAM_VERSION_MICRO}")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")

set(CPACK_COMPONENTS_ALL scram libraries gui examples)
set(CPACK_COMPONENT_SCRAM_REQUIRED ON)
set(CPACK_COMPONENT_LIBRARIES_REQUIRED ON)

include(CPack)

###################### End CPACK configurations #############################
