cmake_minimum_required(VERSION 3.5)
cmake_policy(SET CMP0048 NEW)  # Project version specification.

# In-source build prevention.
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

project(SCRAM VERSION 0.16.0 LANGUAGES CXX)

####################### Begin Options ###################

option(WITH_TCMALLOC "Use TCMalloc if available (#1 preference)" ON)
option(WITH_JEMALLOC "Use JEMalloc if available (#2 preference)" ON)

option(WITH_COVERAGE "Instrument for coverage analysis" OFF)
option(WITH_PROFILE "Instrument for performance profiling" OFF)

option(HAS_TANGO "The system provides the fall-back Tango icons" OFF)

option(BUILD_GUI "Build the GUI front-end" ON)
option(BUILD_TESTS "Build the tests" ON)

option(PACKAGE "Package for distribution" OFF)

####################### End Options ###################

####################### Begin compiler configurations ###################

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # Needed for Clang Tooling.

# Default to C++14.
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

function(CHECK_COMPILER_VERSION MIN_VERSION)
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS MIN_VERSION)
    message(FATAL_ERROR "${CMAKE_CXX_COMPILER_ID} Compiler version too old. Required minimum version: ${MIN_VERSION}")
  endif()
endfunction()

set(CMAKE_CXX_FLAGS_DEBUG
  "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -Werror -Wno-sign-compare -Wnon-virtual-dtor -Wno-missing-field-initializers -Wold-style-cast")

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  CHECK_COMPILER_VERSION("4.9")  # TODO: Wpedantic with gcc 4.9 errors with default arg lambdas.
  set(CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_FLAGS_DEBUG} -Wredundant-decls -Wcast-align -Wlogical-op -Wvla -Wuseless-cast -Wunreachable-code")

  if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 5)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wshadow -Wpedantic -Wmissing-declarations")
  endif()

elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  CHECK_COMPILER_VERSION("3.6")
  set(CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_FLAGS_DEBUG} -Wno-missing-braces -Wshadow -Wunused-exception-parameter")

elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
  CHECK_COMPILER_VERSION("6.1")

elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  CHECK_COMPILER_VERSION("17.0.2")
  # TODO: CMAKE_CXX_STANDARD has no effect on icc.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
  # TODO: Warning with overload of private override.
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -diag-disable=1125")
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

# Setup build locations.
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${SCRAM_BINARY_DIR}/bin")
endif()
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${SCRAM_BINARY_DIR}/lib/scram")
endif()
if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${SCRAM_BINARY_DIR}/lib/scram")
endif()

# Use, i.e. don't skip the full RPATH for the build tree.
set(CMAKE_SKIP_BUILD_RPATH FALSE)

# When building,
# don't use the install RPATH already
# (but later on when installing).
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib/scram")
set(CMAKE_INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib/scram")

# Add the automatically determined parts of the RPATH,
# which point to directories outside the build tree
# to the install RPATH.
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Tell CMake where the modules are.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

######################## End cmake configuration ###################

######################## Begin find libraries ######################

set(MALLOC "System Malloc")
# Optional alternative Malloc for optimization.
if(NOT WITH_PROFILE)
  find_package(Tcmalloc)
  find_package(JeMalloc)
  if(WITH_TCMALLOC AND Tcmalloc_FOUND)
    list(APPEND LIBS ${Tcmalloc_LIBRARIES})
    set(MALLOC "TCMalloc")
  elseif(WITH_JEMALLOC AND JEMALLOC_FOUND)
    list(APPEND LIBS ${JEMALLOC_LIBRARIES})
    set(MALLOC "JEMalloc")
  endif()
endif()
message(STATUS "The memory allocator: ${MALLOC}")

# Find LibXML2 and dependencies.
find_package(LibXml2 REQUIRED)
list(APPEND LIBS ${LIBXML2_LIBRARIES})

# Include the boost header files and the program_options library.
# Please be sure to use Boost rather than BOOST.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(BOOST_MIN_VERSION "1.58.0")
  list(APPEND LIBS ${CMAKE_DL_LIBS})
else()
  set(BOOST_MIN_VERSION "1.61.0")
endif()

if(NOT WIN32)
  set(Boost_USE_MULTITHREADED OFF)
endif()

find_package(Boost ${BOOST_MIN_VERSION} REQUIRED COMPONENTS
  program_options filesystem system date_time random
  )
message(STATUS "Boost Include directory: ${Boost_INCLUDE_DIR}")
message(STATUS "Boost Library directories: ${Boost_LIBRARY_DIRS}")
list(APPEND LIBS ${Boost_LIBRARIES})

message(STATUS "Libraries: ${LIBS}")

########################## End of find libraries ########################

########################## Begin includes ###############################

# Include all the discovered system directories.
include_directories(SYSTEM "${Boost_INCLUDE_DIR}")
include_directories(SYSTEM "${LIBXML2_INCLUDE_DIR}")

include_directories("${CMAKE_SOURCE_DIR}")  # Include the core headers via "src".

add_subdirectory(src)
add_subdirectory(share)
add_subdirectory(input)

if(BUILD_GUI)
  add_subdirectory(gui)
endif()

if(BUILD_TESTS)
  add_subdirectory(tests)
endif()

####################### End includes ####################################

###################### Begin uninstall target ###########################

configure_file(cmake/cmake_uninstall.cmake.in cmake_uninstall.cmake @ONLY)
add_custom_target(uninstall
  COMMAND "${CMAKE_COMMAND}" -P "\"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake\"")

###################### End uninstall target #############################

###################### Begin CPACK configurations ###########################

if(PACKAGE)
  if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    # set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}\\\\gui\\\\images\\\\scram_logo.bmp")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\scram-gui.exe")
    set(CPACK_NSIS_MODIFY_PATH ON)
    set(CPACK_NSIS_HELP_LINK "https://scram-pra.org")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://scram-pra.org")
    set(CPACK_NSIS_CONTACT "scram-users@googlegroups.com")
    set(CPACK_PACKAGE_EXECUTABLES "scram-gui" "SCRAM")
    set(CPACK_CREATE_DESKTOP_LINKS "scram-gui")
    set(CPACK_COMPONENT_GUI_REQUIRED ON)
    # The following configuration assumes MinGW64 is setup specifically for this project.
    get_filename_component(MINGW_DLL_DIR "${CMAKE_CXX_COMPILER}" PATH)
    message(STATUS "MinGW bin location: ${MINGW_DLL_DIR}")
    include(ScramBundle)
    get_scram_prerequisites(SCRAM_PREREQUISITES
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/scram.exe" "${MINGW_DLL_DIR}")
    install(FILES
      ${SCRAM_PREREQUISITES}
      DESTINATION bin COMPONENT libraries
    )
    get_scram_prerequisites(SCRAM_GUI_PREREQUISITES
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/scram-gui.exe" "${MINGW_DLL_DIR}")
    install(FILES
      ${SCRAM_GUI_PREREQUISITES}
      DESTINATION bin COMPONENT libraries
    )
  endif()
endif()

set(CPACK_PACKAGE_VENDOR "Olzhas Rakhimov")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Probabilistic risk analysis tool")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

set(CPACK_STRIP_FILES ON)

set(CPACK_COMPONENTS_ALL scram libraries gui examples)
set(CPACK_COMPONENT_SCRAM_REQUIRED ON)
set(CPACK_COMPONENT_LIBRARIES_REQUIRED ON)

include(CPack)

###################### End CPACK configurations #############################
