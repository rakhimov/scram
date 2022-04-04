macro(get_scram_prerequisites  _output _executable _search_path)
  include(GetPrerequisites)
  get_prerequisites("${_executable}" _all_prerequisites 1 1 "" "")
  message(STATUS "${_executable} dependency libs:\n${_all_prerequisites}")
  foreach(item ${_all_prerequisites})
    string(REGEX MATCH "scram" is_scram "${item}")
	string(REGEX MATCH "libboost" is_boost "${item}")
	string(REGEX MATCH "libxml2" is_libxml2 "${item}")
    if(NOT is_scram AND NOT is_boost AND NOT is_libxml2)
	  gp_resolve_item("${_search_path}" "${item}" "" "" item)
      list(APPEND ${_output} ${item})
    endif()
  endforeach()
  message(STATUS "${_executable} dependency libs resolution:\n${${_output}}")
endmacro()
